// firmware/src/main.cpp
#include <Arduino.h>
#include "config.h"

void setup() {
    Serial.begin(115200);
    Serial.println("CamS3 Monitor starting...");

    // TODO: 初始化各个模块
}

void loop() {
    // FreeRTOS 任务，loop 空置
    vTaskDelay(portMAX_DELAY);
}
