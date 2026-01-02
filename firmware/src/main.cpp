// firmware/src/main.cpp
#include <Arduino.h>
#include "config.h"
#include "camera.h"
#include "wifi_manager.h"
#include "wifi_config.h"

Camera camera;
WiFiManager wifiManager;

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

    Serial.println("Setup complete, entering main loop...");
}

void loop() {
    wifiManager.update();
    vTaskDelay(100);
}
