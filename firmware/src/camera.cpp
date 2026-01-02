// firmware/src/camera.cpp
#include "camera.h"
#include "config.h"
#include <esp_camera.h>

// CamS3 (ESP32S3_EYE) 引脚配置
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       35
#define Y7_GPIO_NUM       17
#define Y6_GPIO_NUM      18
#define Y5_GPIO_NUM      12
#define Y4_GPIO_NUM      10
#define Y3_GPIO_NUM       8
#define Y2_GPIO_NUM       9
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     11

bool Camera::init() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    // 使用 RGB565 格式以便运动检测能直接访问像素数据
    // JPEG 格式会导致像素访问错误（压缩数据不能直接访问）
    config.pixel_format = PIXFORMAT_RGB565;

    // 初始化为高分辨率用于运动检测
    config.frame_size = CAMERA_FRAME_SIZE;
    // RGB565 不需要 jpeg_quality 设置
    config.fb_count = CAMERA_FB_COUNT;

    // 初始化摄像头
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    initialized = true;
    Serial.println("Camera initialized successfully");
    return true;
}

bool Camera::capture() {
    if (!initialized) return false;

    // 释放旧缓冲区
    if (fb != nullptr) {
        esp_camera_fb_return(fb);
    }

    fb = esp_camera_fb_get();
    return fb != nullptr;
}

camera_fb_t* Camera::getBuffer() {
    return fb;
}

size_t Camera::getImageSize() {
    return fb ? fb->len : 0;
}

uint8_t* Camera::getImageData() {
    return fb ? fb->buf : nullptr;
}

void Camera::setFrameSize(framesize_t size) {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, size);
    }
}

void Camera::setQuality(uint8_t quality) {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, quality);
    }
}
