// firmware/src/main.cpp
#include <Arduino.h>
#include "config.h"
#include "camera.h"

Camera camera;

void setup() {
    Serial.begin(115200);
    Serial.println("CamS3 Monitor starting...");

    if (!camera.init()) {
        Serial.println("Camera init failed!");
        delay(1000);
        ESP.restart();
    }

    Serial.println("Setup complete, entering main loop...");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
