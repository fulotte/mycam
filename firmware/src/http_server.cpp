// firmware/src/http_server.cpp
#include "http_server.h"
#include "camera.h"
#include "config.h"

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
    // 根路径
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "CamS3 Monitor - Stream at /stream");
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

    // 健康检查
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
}
