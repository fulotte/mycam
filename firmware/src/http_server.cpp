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
    // WiFi 配置页面
    server.on("/provision", HTTP_GET, [](AsyncWebServerRequest* request) {
        const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MyCam WiFi 配置</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
               background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
               min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; }
        .container { background: white; border-radius: 16px; box-shadow: 0 20px 60px rgba(0,0,0,0.3);
                     width: 100%; max-width: 400px; padding: 30px; }
        h1 { text-align: center; color: #333; margin-bottom: 20px; font-size: 24px; }
        .scanning { text-align: center; color: #666; padding: 40px 20px; }
        .spinner { display: inline-block; width: 40px; height: 40px;
                   border: 4px solid #f3f3f3; border-top: 4px solid #667eea;
                   border-radius: 50%; animation: spin 1s linear infinite; }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
        .network-list { margin-top: 20px; }
        .network-item { background: #f8f9fa; border: 1px solid #e9ecef; border-radius: 8px;
                       padding: 15px; margin-bottom: 10px; cursor: pointer; transition: all 0.2s;
                       display: flex; justify-content: space-between; align-items: center; }
        .network-item:hover { background: #e9ecef; transform: translateX(4px); }
        .network-item.selected { background: #667eea; color: white; border-color: #667eea; }
        .network-name { font-weight: 600; font-size: 16px; }
        .network-meta { font-size: 12px; opacity: 0.7; margin-top: 4px; }
        .password-form { display: none; margin-top: 20px; }
        .password-form.active { display: block; }
        input[type=password] { width: 100%; padding: 12px; border: 2px solid #e9ecef;
                               border-radius: 8px; font-size: 16px; margin-bottom: 15px; }
        input[type=password]:focus { outline: none; border-color: #667eea; }
        .btn-group { display: flex; gap: 10px; }
        button { flex: 1; padding: 12px; border: none; border-radius: 8px; font-size: 16px;
                 font-weight: 600; cursor: pointer; transition: all 0.2s; }
        .btn-primary { background: #667eea; color: white; }
        .btn-primary:hover { background: #5568d3; }
        .btn-secondary { background: #e9ecef; color: #333; }
        .btn-secondary:hover { background: #dee2e6; }
        .status { padding: 15px; border-radius: 8px; margin-top: 15px; display: none; text-align: center; }
        .status.success { background: #d4edda; color: #155724; }
        .status.error { background: #f8d7da; color: #721c24; }
        .status.active { display: block; }
        .refresh-btn { background: none; border: none; color: #667eea; cursor: pointer;
                       font-size: 14px; padding: 0; text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>MyCam WiFi 配置</h1>

        <div id="scanning" class="scanning">
            <div class="spinner"></div>
            <p style="margin-top: 15px;">正在扫描附近的 WiFi 网络...</p>
        </div>

        <div id="networkList" class="network-list" style="display:none;"></div>

        <div id="passwordForm" class="password-form">
            <h2 style="font-size: 18px; margin-bottom: 15px;">连接到: <span id="selectedSSID"></span></h2>
            <input type="password" id="password" placeholder="WiFi 密码">
            <div class="btn-group">
                <button class="btn-secondary" onclick="cancelSelection()">取消</button>
                <button class="btn-primary" onclick="connect()">连接</button>
            </div>
        </div>

        <div id="status" class="status"></div>
    </div>

    <script>
        let selectedNetwork = null;

        function scanNetworks() {
            fetch('/api/wifi/scan')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('scanning').style.display = 'none';
                    const list = document.getElementById('networkList');
                    list.style.display = 'block';
                    list.innerHTML = '';

                    if (data.networks.length === 0) {
                        list.innerHTML = '<p style="text-align:center;color:#666;">未找到 WiFi 网络 <button class="refresh-btn" onclick="scanNetworks()">重新扫描</button></p>';
                        return;
                    }

                    data.networks.forEach(net => {
                        const div = document.createElement('div');
                        div.className = 'network-item';
                        div.innerHTML = `
                            <div>
                                <div class="network-name">${net.ssid}</div>
                                <div class="network-meta">${net.encrypted ? '🔒' : '📶'} 信号: ${getSignalLabel(net.rssi)}</div>
                            </div>
                        `;
                        div.onclick = () => selectNetwork(net.ssid, div);
                        list.appendChild(div);
                    });
                })
                .catch(err => {
                    document.getElementById('scanning').innerHTML = '<p style="color:#dc3545;">扫描失败，<button class="refresh-btn" onclick="scanNetworks()">重试</button></p>';
                });
        }

        function getSignalLabel(rssi) {
            if (rssi > -50) return '强';
            if (rssi > -60) return '中等';
            return '弱';
        }

        function selectNetwork(ssid, element) {
            selectedNetwork = ssid;
            document.querySelectorAll('.network-item').forEach(el => el.classList.remove('selected'));
            element.classList.add('selected');
            document.getElementById('selectedSSID').textContent = ssid;
            document.getElementById('passwordForm').classList.add('active');
            document.getElementById('password').focus();
        }

        function cancelSelection() {
            selectedNetwork = null;
            document.querySelectorAll('.network-item').forEach(el => el.classList.remove('selected'));
            document.getElementById('passwordForm').classList.remove('active');
        }

        function connect() {
            const password = document.getElementById('password').value;
            if (!selectedNetwork) return;

            const status = document.getElementById('status');
            status.className = 'status active';
            status.textContent = '正在连接...';

            fetch('/api/wifi/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `ssid=${encodeURIComponent(selectedNetwork)}&password=${encodeURIComponent(password)}`
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    status.className = 'status success active';
                    status.innerHTML = '<strong>配置成功！</strong><br>设备正在连接 WiFi...<br>请稍后刷新此页面。';
                    document.getElementById('passwordForm').style.display = 'none';
                    document.getElementById('networkList').style.display = 'none';
                } else {
                    status.className = 'status error active';
                    status.textContent = '连接失败: ' + (data.error || '未知错误');
                }
            })
            .catch(err => {
                status.className = 'status error active';
                status.textContent = '请求失败: ' + err.message;
            });
        }

        // 页面加载时自动扫描
        scanNetworks();

        // 密码框回车连接
        document.getElementById('password')?.addEventListener('keypress', e => {
            if (e.key === 'Enter') connect();
        });
    </script>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });

    // AP 模式下重定向根路径到配置页
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (provManager && provManager->getState() == AP_PROVISIONING) {
            request->redirect("/provision");
        } else {
            // 原有的预览页面
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
        }
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
