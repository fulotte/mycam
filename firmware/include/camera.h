// firmware/include/camera.h
#pragma once

#include <esp_camera.h>
#include <Arduino.h>

class Camera {
public:
    bool init();
    bool capture();
    camera_fb_t* getBuffer();
    size_t getImageSize();
    uint8_t* getImageData();

    // 设置
    void setFrameSize(framesize_t size);
    void setQuality(uint8_t quality);

private:
    camera_fb_t* fb = nullptr;
    bool initialized = false;
};
