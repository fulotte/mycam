// firmware/include/http_server.h
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "camera.h"
#include "provisioning_manager.h"
#include "wifi_scanner.h"

class HTTPServer {
private:
    AsyncWebServer server = AsyncWebServer(HTTP_PORT);
    camera_fb_t* currentFb = nullptr;

    // 新增：依赖注入
    ProvisioningManager* provManager = nullptr;
    WiFiScanner* wifiScanner = nullptr;

public:
    void begin();
    void setBuffer(camera_fb_t* fb);

    // 新增：设置依赖
    void setProvisioningManager(ProvisioningManager* pm) { provManager = pm; }
    void setWiFiScanner(WiFiScanner* scanner) { wifiScanner = scanner; }

private:
    void setupRoutes();
    void setupProvisioningRoutes();
};
