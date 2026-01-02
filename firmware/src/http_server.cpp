// firmware/src/http_server.cpp
#include "http_server.h"
#include "camera.h"
#include "config.h"
#include "provisioning_manager.h"
#include "wifi_scanner.h"
#include "logger.h"
#include <ArduinoJson.h>

// 外部引用运动检测状态
extern bool g_motionDetected;

void HTTPServer::begin() {
    setupRoutes();
    server.begin();
    Serial.println("HTTP Server started");
    Serial.printf("Stream URL: http://%s/stream\n", WiFi.localIP().toString().c_str());
}

void HTTPServer::setBuffer(camera_fb_t* fb) {
    currentFb = fb;
}

void HTTPServer::setupRoutes() {
    // 根路径 - 返回网页界面
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // 返回嵌入的 HTML
        const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CamS3 Monitor</title>
    <style>
        body { font-family: Arial; max-width: 640px; margin: 0 auto; padding: 20px; text-align: center; }
        #stream { width: 100%; max-width: 640px; border: 2px solid #333; background: #000; }
    </style>
</head>
<body>
    <h1>CamS3 Monitor</h1>
    <img id="stream" src="/stream" alt="Stream">
    <script>
        const img = document.getElementById('stream');
        setInterval(() => { img.src = '/stream?' + Date.now(); }, 333);
    </script>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });

    // 实时视频流端点 (RGB565 格式，浏览器兼容)
    server.on("/stream", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!currentFb) {
            request->send(503, "text/plain", "No image available");
            return;
        }

        // RGB565 格式：width * height * 2 字节
        size_t bmpDataSize = currentFb->width * currentFb->height * 2 + 54;  // +54 for BMP header
        uint8_t* bmpBuffer = (uint8_t*)malloc(bmpDataSize);
        if (!bmpBuffer) {
            request->send(503, "text/plain", "Memory allocation failed");
            return;
        }

        // 构建 BMP 文件头
        uint8_t header[54] = {0};
        header[0] = 'B';
        header[1] = 'M';
        uint32_t fileSize = bmpDataSize;
        memcpy(header + 2, &fileSize, 4);
        header[10] = 54;  // offset to pixel data
        header[14] = 40;  // BITMAPINFOHEADER size
        int32_t width = currentFb->width;
        int32_t height = -currentFb->height;  // negative for top-down
        memcpy(header + 18, &width, 4);
        memcpy(header + 22, &height, 4);
        header[26] = 1;  // planes
        header[28] = 16;  // bits per pixel (RGB565)

        memcpy(bmpBuffer, header, 54);
        memcpy(bmpBuffer + 54, currentFb->buf, currentFb->len);

        AsyncWebServerResponse* response = request->beginResponse(
            "image/bmp",
            bmpDataSize,
            [bmpBuffer](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
                if (index >= bmpDataSize) {
                    free(bmpBuffer);
                    return 0;
                }
                size_t toSend = bmpDataSize - index;
                if (toSend > maxLen) {
                    toSend = maxLen;
                }
                memcpy(buffer, bmpBuffer + index, toSend);
                if (index + toSend >= bmpDataSize) {
                    free(bmpBuffer);
                }
                return toSend;
            }
        );
        response->addHeader("Content-Type", "image/bmp");
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
        request->send(response);
    });

    // 运动状态端点
    server.on("/motion", HTTP_GET, [](AsyncWebServerRequest* request) {
        char json[64];
        snprintf(json, sizeof(json), "{\"motion\":%s}", g_motionDetected ? "true" : "false");
        request->send(200, "application/json", json);
    });

    // 健康检查
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    setupProvisioningRoutes();
}

void HTTPServer::setupProvisioningRoutes() {
    if (!provManager || !wifiScanner) return;

    // 扫描 WiFi
    server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        NetworkInfo networks[20];
        int count = wifiScanner->scan(networks, 20);

        StaticJsonDocument<2048> doc;
        JsonArray arr = doc.createNestedArray("networks");

        for (int i = 0; i < count; i++) {
            JsonObject net = arr.createNestedObject();
            net["ssid"] = networks[i].ssid;
            net["rssi"] = networks[i].rssi;
            net["encrypted"] = networks[i].encrypted;
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // 获取当前配置
    server.on("/api/wifi/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        WiFiConfig config;
        bool hasConfig = false;

        // 从当前连接获取
        if (WiFi.isConnected()) {
            StaticJsonDocument<256> doc;
            doc["ssid"] = WiFi.SSID();
            doc["hasConfig"] = true;
            doc["mode"] = "STA";

            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else {
            request->send(200, "application/json", "{\"hasConfig\":false,\"mode\":\"AP\"}");
        }
    });

    // 保存配置并连接
    server.on("/api/wifi/config", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400, "application/json", "{\"success\":false,\"error\":\"MISSING_PARAMS\"}");
            return;
        }

        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();

        if (provManager->saveAndConnect(ssid.c_str(), password.c_str())) {
            request->send(200, "application/json", "{\"success\":true,\"message\":\"Connecting...\"}");
        } else {
            request->send(500, "application/json", "{\"success\":false,\"error\":\"STORAGE_ERROR\"}");
        }
    });

    // 重置配置
    server.on("/api/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (provManager->resetConfig()) {
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(500, "application/json", "{\"success\":false,\"error\":\"RESET_FAILED\"}");
        }
    });

    // 获取状态
    server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        StaticJsonDocument<512> doc;
        doc["connected"] = WiFi.isConnected();
        doc["mode"] = (WiFi.getMode() == WIFI_STA) ? "STA" : "AP";

        if (WiFi.isConnected()) {
            doc["ip"] = WiFi.localIP().toString();
        } else {
            doc["ip"] = WiFi.softAPIP().toString();
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    Logger::info("HTTP", "Provisioning routes registered");
}
