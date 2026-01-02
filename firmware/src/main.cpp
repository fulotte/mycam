// firmware/src/main.cpp
#include <Arduino.h>
#include "config.h"
#include "camera.h"
#include "wifi_manager.h"
#include "http_server.h"
#include "wifi_config.h"

Camera camera;
WiFiManager wifiManager;
HTTPServer httpServer;

// 全局缓冲区指针
camera_fb_t* g_currentFb = nullptr;

void captureTask(void* parameter) {
    while (true) {
        if (camera.capture()) {
            // 释放旧缓冲区
            if (g_currentFb != nullptr) {
                esp_camera_fb_return(g_currentFb);
            }
            g_currentFb = camera.getBuffer();
            httpServer.setBuffer(g_currentFb);
        }
        vTaskDelay(pdMS_TO_TICKS(1000 / STREAM_FPS));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("CamS3 Monitor starting...");

    if (!camera.init()) {
        Serial.println("Camera init failed!");
        delay(1000);
        ESP.restart();
    }

    if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("WiFi connection failed, restarting...");
        delay(5000);
        ESP.restart();
    }

    httpServer.begin();

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

    Serial.println("Setup complete");
}

void loop() {
    wifiManager.update();
    vTaskDelay(100);
}
