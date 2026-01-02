// firmware/src/http_server.cpp
#include "http_server.h"
#include "camera.h"
#include "config.h"

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

    // 实时视频流端点
    server.on("/stream", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!currentFb) {
            request->send(503, "text/plain", "No image available");
            return;
        }

        AsyncWebServerResponse* response = request->beginResponse(
            "image/jpeg",
            currentFb->len,
            [this](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
                if (index >= currentFb->len) {
                    return 0;
                }
                size_t toSend = currentFb->len - index;
                if (toSend > maxLen) {
                    toSend = maxLen;
                }
                memcpy(buffer, currentFb->buf + index, toSend);
                return toSend;
            }
        );
        response->addHeader("Content-Type", "image/jpeg");
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
}
