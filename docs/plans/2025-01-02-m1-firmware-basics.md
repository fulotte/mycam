# M1: CamS3 固件基础实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**目标:** 实现 CamS3 固件的基础功能，包括图像采集、HTTP Server（局域网实时预览）、WiFi 连接管理和运动检测。

**架构:** 基于 PlatformIO + ESP32-Arduino 框架，使用 FreeRTOS 多任务分离图像采集、运动检测和网络通信。HTTP Server 提供 `/stream` 端点返回最新 JPEG 帧，前端定时轮询实现伪视频流。

**技术栈:** PlatformIO, ESP32-Arduino, ESP32 Camera, AsyncWebServer_ESP32, ArduinoJson, FreeRTOS

---

## Task 1: PlatformIO 项目初始化

**Files:**
- Create: `firmware/platformio.ini`
- Create: `firmware/src/main.cpp`
- Create: `firmware/include/config.h`

**Step 1: 创建 platformio.ini 配置**

```ini
; firmware/platformio.ini
[env:camS3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

; 串口配置
monitor_speed = 115200
monitor_filters = esp32_exception_decoder

; 编译选项
build_flags =
    -D BOARD_HAS_PSRAM
    -D CONFIG_CAMERA_MODEL_ESP32S3_EYE
    -D CORE_DEBUG_LEVEL=3

; 依赖库
lib_deps =
    esp32camera
    me-no-dev/AsyncTCP @^1.1.1
    mathieucarbou/AsyncWebServer_ESP32 @^2.7.0
    bblanchon/ArduinoJson @^6.21.3

; 分区表
board_build.partitions = huge_app.csv
```

**Step 2: 创建配置头文件**

```cpp
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
```

**Step 3: 创建主程序入口**

```cpp
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
```

**Step 4: 初始化 Git 仓库并提交**

```bash
cd E:\myapp\mycam-repo
git add firmware/
git commit -m "feat(m1): initialize PlatformIO project structure"
```

---

## Task 2: 摄像头初始化和图像采集

**Files:**
- Create: `firmware/src/camera.cpp`
- Create: `firmware/include/camera.h`
- Modify: `firmware/src/main.cpp`

**Step 1: 编写摄像头头文件**

```cpp
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
```

**Step 2: 编写摄像头实现**

```cpp
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
    config.pixel_format = PIXFORMAT_JPEG;

    // 初始化为高分辨率用于运动检测
    config.frame_size = CAMERA_FRAME_SIZE;
    config.jpeg_quality = CAMERA_JPEG_QUALITY;
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
```

**Step 3: 在 main.cpp 中初始化摄像头**

```cpp
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
```

**Step 4: 编译并烧录测试**

```bash
cd E:\myapp\mycam-repo\firmware
pio run --target upload
pio device monitor
```

预期输出: "Camera initialized successfully"

**Step 5: 提交**

```bash
cd E:\myapp\mycam-repo
git add firmware/
git commit -m "feat(m1): implement camera initialization and capture"
```

---

## Task 3: WiFi 连接管理

**Files:**
- Create: `firmware/src/wifi_manager.cpp`
- Create: `firmware/include/wifi_manager.h`
- Modify: `firmware/src/main.cpp`

**Step 1: 编写 WiFi 管理器头文件**

```cpp
// firmware/include/wifi_manager.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    bool begin(const char* ssid, const char* password);
    bool isConnected();
    void update();
    String getIP();
    String getMAC();

private:
    unsigned long lastReconnectAttempt = 0;
    void reconnect();
};
```

**Step 2: 编写 WiFi 管理器实现**

```cpp
// firmware/src/wifi_manager.cpp
#include "wifi_manager.h"
#include "config.h"

bool WiFiManager::begin(const char* ssid, const char* password) {
    Serial.printf("Connecting to WiFi: %s\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);  // 保持活跃以降低延迟
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.printf("IP address: %s\n", getIP().c_str());
        return true;
    }

    Serial.println("\nWiFi connection failed!");
    return false;
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::update() {
    if (!isConnected() &&
        millis() - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS) {
        reconnect();
    }
}

void WiFiManager::reconnect() {
    Serial.println("Attempting WiFi reconnection...");
    lastReconnectAttempt = millis();
    WiFi.reconnect();
}

String WiFiManager::getIP() {
    return WiFi.localIP().toString();
}

String WiFiManager::getMAC() {
    return WiFi.macAddress();
}
```

**Step 3: 创建 WiFi 配置文件**

```cpp
// firmware/include/wifi_config.h
#pragma once

// TODO: 改为通过 SmartConfig 或 Web 配置
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"
```

**Step 4: 在 main.cpp 中集成 WiFi**

```cpp
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
```

**Step 5: 编译测试**

```bash
pio run --target upload
pio device monitor
```

预期输出: 显示 IP 地址

**Step 6: 提交**

```bash
git add firmware/
git commit -m "feat(m1): implement WiFi connection management"
```

---

## Task 4: HTTP Server 和实时视频流

**Files:**
- Create: `firmware/src/http_server.cpp`
- Create: `firmware/include/http_server.h`
- Modify: `firmware/src/main.cpp`

**Step 1: 编写 HTTP Server 头文件**

```cpp
// firmware/include/http_server.h
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class HTTPServer {
public:
    void begin();
    void setBuffer(camera_fb_t* fb);

private:
    AsyncWebServer server = AsyncWebServer(HTTP_PORT);
    camera_fb_t* currentFb = nullptr;

    void setupRoutes();
};
```

**Step 2: 编写 HTTP Server 实现**

```cpp
// firmware/src/http_server.cpp
#include "http_server.h"
#include "camera.h"
#include "config.h"

void HTTPServer::begin() {
    setupRoutes();
    server.begin();
    Serial.println("HTTP Server started");
    Serial.printf("Stream URL: http://%s/stream\n", WiFi.localIP().toString().c_str());
}

void HTTPServer::setBuffer(camera_fb_t* fb) {
    currentFb = fb;
}

void HTTPServer::setupRoutes() {
    // 根路径
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "CamS3 Monitor - Stream at /stream");
    });

    // 实时视频流端点
    server.on("/stream", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!currentFb) {
            request->send(503, "text/plain", "No image available");
            return;
        }

        AsyncWebServerResponse* response = request->beginResponse(
            "image/jpeg",
            currentFb->len,
            [this](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
                if (index >= currentFb->len) {
                    return 0;
                }
                size_t toSend = currentFb->len - index;
                if (toSend > maxLen) {
                    toSend = maxLen;
                }
                memcpy(buffer, currentFb->buf + index, toSend);
                return toSend;
            }
        );
        response->addHeader("Content-Type", "image/jpeg");
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
        request->send(response);
    });

    // 健康检查
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
}
```

**Step 3: 创建图像采集任务**

```cpp
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
```

**Step 4: 编译并测试**

```bash
pio run --target upload
pio device monitor
```

使用浏览器访问 `http://{camera_ip}/stream`，应该能看到实时画面。

**Step 5: 提交**

```bash
git add firmware/
git commit -m "feat(m1): implement HTTP server with live stream endpoint"
```

---

## Task 5: 运动检测模块

**Files:**
- Create: `firmware/src/motion_detector.cpp`
- Create: `firmware/include/motion_detector.h`
- Modify: `firmware/src/main.cpp`

**Step 1: 编写运动检测头文件**

```cpp
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
```

**Step 2: 编写运动检测实现**

```cpp
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
                            sum += (r + g + b) / 3;
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
```

**Step 3: 在 main.cpp 中集成运动检测**

```cpp
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
```

**Step 4: 编译测试**

```bash
pio run --target upload
pio device monitor
```

预期输出: 当画面变化时显示 "Motion detected!"

**Step 5: 提交**

```bash
git add firmware/
git commit -m "feat(m1): implement grid-based motion detection"
```

---

## Task 6: mDNS 服务发现

**Files:**
- Modify: `firmware/src/main.cpp`
- Modify: `firmware/include/config.h`

**Step 1: 添加 mDNS 初始化**

```cpp
// firmware/src/main.cpp (在 setup() 中添加)
#include <ESPmDNS>

// 在 httpServer.begin() 之后添加
if (MDNS.begin(MDNS_NAME)) {
    Serial.printf("mDNS responder started: http://%s.local/\n", MDNS_NAME);
} else {
    Serial.println("Error setting up MDNS responder!");
}

// 添加 HTTP 服务
MDNS.addService("http", "tcp", HTTP_PORT);
```

**Step 2: 编译测试**

```bash
pio run --target upload
```

在同一局域网的其他设备上，可以通过 `http://camS3.local/` 访问。

**Step 3: 提交**

```bash
git add firmware/
git commit -m "feat(m1): add mDNS service discovery"
```

---

## Task 7: 简单测试页面

**Files:**
- Modify: `firmware/src/http_server.cpp`
- Create: `firmware/data/index.html`

**Step 1: 创建测试页面**

```html
<!-- firmware/data/index.html -->
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CamS3 Monitor</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 640px;
            margin: 0 auto;
            padding: 20px;
            text-align: center;
        }
        #stream {
            width: 100%;
            max-width: 640px;
            border: 2px solid #333;
            background: #000;
        }
        .status {
            margin-top: 20px;
            padding: 10px;
            background: #f0f0f0;
            border-radius: 5px;
        }
        .motion {
            color: red;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <h1>CamS3 Monitor</h1>
    <img id="stream" alt="Loading stream...">
    <div class="status">
        <p>Image Size: <span id="size">-</span></p>
        <p>Last Update: <span id="time">-</span></p>
        <p class="motion">Motion: <span id="motion">No</span></p>
    </div>

    <script>
        const img = document.getElementById('stream');
        const sizeSpan = document.getElementById('size');
        const timeSpan = document.getElementById('time');
        const motionSpan = document.getElementById('motion');

        function updateStream() {
            img.src = '/stream?' + Date.now();
            sizeSpan.textContent = img.naturalWidth + 'x' + img.naturalHeight;
            timeSpan.textContent = new Date().toLocaleTimeString();
        }

        img.onload = updateStream;
        updateStream();

        // 轮询运动状态
        setInterval(() => {
            fetch('/motion')
                .then(r => r.json())
                .then(d => {
                    motionSpan.textContent = d.motion ? 'YES' : 'No';
                    motionSpan.style.color = d.motion ? 'red' : 'gray';
                })
                .catch(() => {});
        }, 500);
    </script>
</body>
</html>
```

**Step 2: 更新 HTTP Server**

```cpp
// firmware/src/http_server.cpp
// 添加新的路由
server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    // 返回嵌入的 HTML
    const char* html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CamS3 Monitor</title>
    <style>
        body { font-family: Arial; max-width: 640px; margin: 0 auto; padding: 20px; text-align: center; }
        #stream { width: 100%; max-width: 640px; border: 2px solid #333; background: #000; }
    </style>
</head>
<body>
    <h1>CamS3 Monitor</h1>
    <img id="stream" src="/stream" alt="Stream">
    <script>
        const img = document.getElementById('stream');
        setInterval(() => { img.src = '/stream?' + Date.now(); }, 333);
    </script>
</body>
</html>
    )";
    request->send(200, "text/html", html);
});

// 运动状态端点
extern bool g_motionDetected;

server.on("/motion", HTTP_GET, [](AsyncWebServerRequest* request) {
    char json[64];
    snprintf(json, sizeof(json), "{\"motion\":%s}", g_motionDetected ? "true" : "false");
    request->send(200, "application/json", json);
});
```

**Step 3: 编译测试**

```bash
pio run --target upload
```

访问 `http://{camera_ip}/` 应该看到实时视频流。

**Step 4: 提交**

```bash
git add firmware/
git commit -m "feat(m1): add web interface for stream viewing"
```

---

## 完成标准

- [ ] 摄像头能正常采集图像
- [ ] WiFi 自动连接，断线重连
- [ ] HTTP Server 响应 `/stream` 请求返回 JPEG
- [ ] Web 页面显示实时视频流
- [ ] 运动检测能识别画面变化
- [ ] mDNS 服务发现正常工作

---

## 下一步

M1 完成后，进入 M2: 云端基础实施计划
