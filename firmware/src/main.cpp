// firmware/src/main.cpp
#include <Arduino.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include "config.h"
#include "camera.h"
#include "wifi_manager.h"
#include "http_server.h"
#include "motion_detector.h"
#include "logger.h"
#include "config_storage.h"
#include "wifi_scanner.h"
#include "provisioning_manager.h"
#include "captive_portal.h"

Camera camera;
WiFiManager wifiManager;
LittleFSConfigStorage storage;
WiFiScanner wifiScanner;
ProvisioningManager* provManager = nullptr;
HTTPServer httpServer;
MotionDetector motionDetector;

camera_fb_t* g_currentFb = nullptr;
bool g_motionDetected = false;

// 看门狗任务
void wdtTask(void* parameter) {
    while (true) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void captureTask(void* parameter) {
    while (true) {
        if (camera.capture()) {
            if (g_currentFb != nullptr) {
                esp_camera_fb_return(g_currentFb);
            }
            g_currentFb = camera.getBuffer();
            httpServer.setBuffer(g_currentFb);

            // 运动检测
            if (motionDetector.detect(g_currentFb)) {
                if (!g_motionDetected) {
                    g_motionDetected = true;
                    Logger::info("MOTION", "Motion detected!");
                }
            } else {
                g_motionDetected = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000 / STREAM_FPS));
    }
}

void setup() {
    Serial.begin(115200);
    Logger::info("MAIN", "CamS3 Monitor starting...");

    // 初始化 LittleFS 文件系统
    if (!LittleFS.begin(true)) {
        Logger::error("MAIN", "LittleFS mount failed, restarting...");
        delay(1000);
        ESP.restart();
    }
    Logger::info("MAIN", "LittleFS mounted");

    // 初始化看门狗
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    if (!camera.init()) {
        Logger::error("MAIN", "Camera init failed!");
        delay(1000);
        ESP.restart();
    }
    Logger::info("MAIN", "Camera initialized");

    motionDetector.init();
    Logger::info("MAIN", "Motion detector initialized");

    // Provisioning 管理
    provManager = new ProvisioningManager(&storage, wifiManager);
    if (!provManager->begin()) {
        Logger::error("MAIN", "Provisioning failed, restarting...");
        delay(5000);
        ESP.restart();
    }

    // 设置 HTTP 服务依赖
    httpServer.setProvisioningManager(provManager);
    httpServer.setWiFiScanner(&wifiScanner);

    httpServer.begin();
    Logger::info("MAIN", "HTTP server started");

    // 启动 mDNS 服务
    // mDNS 只在 STA 模式下启动
    if (provManager->getState() == STA_CONNECTED && MDNS.begin(MDNS_NAME)) {
        Logger::info("MAIN", "mDNS responder started: http://%s.local/", MDNS_NAME);
        MDNS.addService("http", "tcp", HTTP_PORT);
    } else if (provManager->getState() == AP_PROVISIONING) {
        Logger::info("MAIN", "AP mode: connect to WiFi and open browser");
    }

    // 创建采集任务
    xTaskCreateUniversal(
        captureTask,
        "capture",
        8192,
        NULL,
        TASK_CAPTURE_PRIORITY,
        NULL,
        ARDUINO_RUNNING_CORE
    );

    // 创建看门狗任务
    xTaskCreateUniversal(
        wdtTask,
        "wdt",
        2048,
        NULL,
        0,
        NULL,
        ARDUINO_RUNNING_CORE
    );

    Logger::info("MAIN", "Setup complete");
}

void loop() {
    wifiManager.update();
    if (provManager) {
        provManager->update();
    }
    vTaskDelay(100);
}
