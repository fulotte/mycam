// firmware/src/main.cpp
#include <Arduino.h>
#include "config.h"
#include "camera.h"
#include "wifi_manager.h"
#include "http_server.h"
#include "motion_detector.h"
#include "wifi_config.h"

Camera camera;
WiFiManager wifiManager;
HTTPServer httpServer;
MotionDetector motionDetector;

camera_fb_t* g_currentFb = nullptr;
bool g_motionDetected = false;

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
                    Serial.println("Motion detected!");
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
    Serial.println("CamS3 Monitor starting...");

    if (!camera.init()) {
        Serial.println("Camera init failed!");
        delay(1000);
        ESP.restart();
    }

    motionDetector.init();

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
