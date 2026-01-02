// firmware/src/motion_detector.cpp
#include "motion_detector.h"
#include "config.h"
#include <string.h>

bool MotionDetector::init() {
    memset(prevGrid, 0, sizeof(prevGrid));
    initialized = true;
    Serial.println("Motion detector initialized");
    return true;
}

void MotionDetector::processGrid(camera_fb_t* fb, uint8_t* grid) {
    if (!fb || !fb->buf) return;

    // 计算 VMA (每个网格的平均亮度)
    int imgWidth = fb->width;
    int imgHeight = fb->height;
    int cellWidth = imgWidth / MOTION_GRID_COLS;
    int cellHeight = imgHeight / MOTION_GRID_ROWS;

    for (int row = 0; row < MOTION_GRID_ROWS; row++) {
        for (int col = 0; col < MOTION_GRID_COLS; col++) {
            // 计算当前网格的像素平均值
            unsigned long sum = 0;
            int count = 0;

            int startX = col * cellWidth;
            int startY = row * cellHeight;
            int endX = startX + cellWidth;
            int endY = startY + cellHeight;

            // 采样计算（降低计算量）
            for (int y = startY; y < endY; y += 2) {
                for (int x = startX; x < endX; x += 2) {
                    if (y < imgHeight && x < imgWidth) {
                        // JPEG 格式，需要解码像素
                        // 简化：使用灰度近似
                        int idx = (y * imgWidth + x) * 3;  // RGB
                        if (idx + 2 < fb->len) {
                            uint8_t r = fb->buf[idx];
                            uint8_t g = fb->buf[idx + 1];
                            uint8_t b = fb->buf[idx + 2];
                            sum += (r + g * 2 + b) / 4;
                            count++;
                        }
                    }
                }
            }

            grid[row * MOTION_GRID_COLS + col] = count > 0 ? sum / count : 0;
        }
    }
}

bool MotionDetector::compareGrids(const uint8_t* grid1, const uint8_t* grid2) {
    int changedCells = 0;

    for (int i = 0; i < MOTION_GRID_ROWS * MOTION_GRID_COLS; i++) {
        int diff = abs((int)grid1[i] - (int)grid2[i]);
        if (diff > threshold) {
            changedCells++;
        }
    }

    return changedCells >= triggerCount;
}

bool MotionDetector::detect(camera_fb_t* fb) {
    if (!initialized || !fb) return false;

    uint8_t currentGrid[MOTION_GRID_ROWS * MOTION_GRID_COLS];
    processGrid(fb, currentGrid);

    bool motion = compareGrids(prevGrid, currentGrid);

    // 更新前一帧
    memcpy(prevGrid, currentGrid, sizeof(prevGrid));

    return motion;
}

void MotionDetector::setThreshold(uint8_t th) {
    threshold = th;
}

void MotionDetector::setTriggerCount(uint8_t count) {
    triggerCount = count;
}
