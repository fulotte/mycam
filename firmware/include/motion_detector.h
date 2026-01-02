// firmware/include/motion_detector.h
#pragma once

#include <Arduino.h>
#include <esp_camera.h>

class MotionDetector {
public:
    bool init();
    bool detect(camera_fb_t* fb);
    void setThreshold(uint8_t threshold);
    void setTriggerCount(uint8_t count);

private:
    uint8_t prevGrid[MOTION_GRID_ROWS * MOTION_GRID_COLS] = {0};
    bool initialized = false;
    uint8_t threshold = MOTION_THRESHOLD;
    uint8_t triggerCount = MOTION_TRIGGER_COUNT;

    void processGrid(camera_fb_t* fb, uint8_t* grid);
    bool compareGrids(const uint8_t* grid1, const uint8_t* grid2);
};
