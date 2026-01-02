// firmware/include/config.h
#pragma once

#include <Arduino.h>

// 摄像头配置
#define CAMERA_MODEL_ESP32S3_EYE
#define CAMERA_FRAME_SIZE FRAMESIZE_VGA  // 640x480
#define CAMERA_JPEG_QUALITY 12           // 1-63, 越低质量越高
#define CAMERA_FB_COUNT 2                // 双缓冲

// 网络配置
#define HTTP_PORT 80
#define MDNS_NAME "camS3"
#define STREAM_FPS 3                     // 实时预览 FPS

// 运动检测配置
#define MOTION_GRID_ROWS 8
#define MOTION_GRID_COLS 8
#define MOTION_THRESHOLD 30              // 像素变化阈值 (0-255)
#define MOTION_TRIGGER_COUNT 5           // 触发网格数量阈值
#define MOTION_CHECK_INTERVAL_MS 200     // 检测间隔

// WiFi 配置
#define WIFI_TIMEOUT_MS 30000
#define WIFI_RECONNECT_INTERVAL_MS 5000

// 任务优先级
#define TASK_CAPTURE_PRIORITY 2
#define TASK_DETECT_PRIORITY 2
#define TASK_SERVER_PRIORITY 1
