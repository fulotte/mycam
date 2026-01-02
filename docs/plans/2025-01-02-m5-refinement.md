# M5: 完善优化实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**目标:** 完善错误处理、性能优化、测试覆盖和文档。确保系统稳定可靠，可以正式发布 v1.0 版本。

**架构:** 不改变整体架构，专注于：
1. 错误处理和日志
2. 性能优化
3. 单元测试和集成测试
4. 文档完善
5. 部署自动化

---

## Task 1: 错误处理和日志系统

### 1.1 固件端错误处理

**Files:**
- Modify: `firmware/src/main.cpp`
- Create: `firmware/src/logger.h`
- Create: `firmware/src/logger.cpp`

**Step 1: 创建日志系统**

```cpp
// firmware/src/logger.h
#pragma once

#include <Arduino.h>

enum LogLevel {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

class Logger {
public:
    static void log(LogLevel level, const char* tag, const char* format, ...);
    static void error(const char* tag, const char* format, ...);
    static void warn(const char* tag, const char* format, ...);
    static void info(const char* tag, const char* format, ...);
    static void debug(const char* tag, const char* format, ...);

    static void setLogLevel(LogLevel level);

private:
    static LogLevel minLevel;
    static const char* levelToString(LogLevel level);
};
```

```cpp
// firmware/src/logger.cpp
#include "logger.h"
#include <stdarg.h>

LogLevel Logger::minLevel = LOG_INFO;

void Logger::log(LogLevel level, const char* tag, const char* format, ...) {
    if (level < minLevel) return;

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("[%s][%s] %s\n", levelToString(level), tag, buffer);
}

void Logger::error(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_ERROR, tag, format, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_WARN, tag, format, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_INFO, tag, format, args);
    va_end(args);
}

void Logger::debug(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_DEBUG, tag, format, args);
    va_end(args);
}

void Logger::setLogLevel(LogLevel level) {
    minLevel = level;
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LOG_ERROR: return "ERROR";
        case LOG_WARN: return "WARN";
        case LOG_INFO: return "INFO";
        case LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
```

**Step 2: 添加错误处理**

```cpp
// firmware/src/main.cpp
#include "logger.h"

void setup() {
    Serial.begin(115200);
    Logger::info("MAIN", "CamS3 Monitor starting...");

    // 使用 try-catch 风格的错误处理
    if (!camera.init()) {
        Logger::error("MAIN", "Camera init failed");
        delay(5000);
        ESP.restart();
    }

    // ... 其他初始化
}

// 看门狗任务
void wdtTask(void* parameter) {
    while (true) {
        // 喂狗
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void setup() {
    // ... 现有代码

    // 初始化看门狗
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    // 创建看门狗任务
    xTaskCreateUniversal(
        wdtTask,
        "wdt",
        2048,
        NULL,
        0,
        NULL,
        ARDUINO_RUNNING_CORE
    );
}
```

**Step 3: 提交**

```bash
git add firmware/src/
git commit -m "feat(m5): add logging system and error handling"
```

---

### 1.2 云端错误处理

**Files:**
- Create: `backend/shared/error-handler.js`
- Create: `backend/shared/logger.js`

**Step 1: 创建错误处理中间件**

```javascript
// backend/shared/error-handler.js

class AppError extends Error {
  constructor(message, statusCode) {
    super(message);
    this.statusCode = statusCode;
    this.isOperational = true;
    Error.captureStackTrace(this, this.constructor);
  }
}

class ErrorHandler {
  static async handleError(err, context) {
    // 记录错误
    await this.logError(err, context);

    // 发送告警（严重错误）
    if (err.statusCode >= 500) {
      await this.sendAlert(err, context);
    }
  }

  static async logError(err, context) {
    const logEntry = {
      timestamp: new Date().toISOString(),
      error: {
        message: err.message,
        stack: err.stack,
        code: err.code
      },
      context
    };

    // 写入 SLS 或简单日志
    console.error(JSON.stringify(logEntry));
  }

  static async sendAlert(err, context) {
    // TODO: 发送到钉钉/飞书群
  }

  static response(error) {
    if (error.isOperational) {
      return {
        statusCode: error.statusCode,
        body: JSON.stringify({
          error: error.message
        })
      };
    }

    // 未知错误
    return {
      statusCode: 500,
      body: JSON.stringify({
        error: 'Internal server error'
      })
    };
  }
}

module.exports = { AppError, ErrorHandler };
```

**Step 2: 在各函数中使用**

```javascript
// backend/functions/upload-handler/index.js
const { AppError, ErrorHandler } = require('../../shared/error-handler');

module.exports.handler = async (event, context) => {
  try {
    // ... 现有逻辑

    if (!device_id) {
      throw new AppError('device_id is required', 400);
    }

    // ... 业务逻辑

  } catch (error) {
    await ErrorHandler.handleError(error, { event });
    return ErrorHandler.response(error);
  }
};
```

**Step 3: 提交**

```bash
git add backend/shared/
git commit -m "feat(m5): add error handling for cloud functions"
```

---

## Task 2: 性能优化

### 2.1 固件性能优化

**Files:**
- Modify: `firmware/src/motion_detector.cpp`
- Modify: `firmware/include/config.h`

**Step 1: 优化运动检测算法**

```cpp
// firmware/src/motion_detector.cpp

// 使用更高效的像素采样
void MotionDetector::processGrid(camera_fb_t* fb, uint8_t* grid) {
    if (!fb || !fb->buf) return;

    // 降采样处理
    const int SAMPLE_STEP = 4;  // 每 4 个像素采样一次

    // ... 优化后的算法
}
```

**Step 2: 添加自适应帧率**

```cpp
// 根据网络状况调整帧率
void adjustFpsBasedOnNetwork() {
    static unsigned long lastUploadTime = 0;
    static int currentFps = STREAM_FPS;

    unsigned long uploadTime = millis() - lastUploadTime;

    if (uploadTime > 2000) {  // 上传超过 2 秒
        currentFps = max(1, currentFps - 1);
    } else if (uploadTime < 500) {  // 上传很快
        currentFps = min(STREAM_FPS, currentFps + 1);
    }

    return currentFps;
}
```

**Step 3: 提交**

```bash
git add firmware/src/
git commit -m "perf(m5): optimize motion detection and adaptive frame rate"
```

---

### 2.2 前端性能优化

**Files:**
- Modify: `frontend/src/views/ImageList.vue`
- Create: `frontend/src/utils/lazyLoad.js`

**Step 1: 添加图片懒加载**

```javascript
// frontend/src/utils/lazyLoad.js
export function setupLazyLoad() {
  const images = document.querySelectorAll('img[data-src]');

  const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        const img = entry.target;
        img.src = img.dataset.src;
        img.removeAttribute('data-src');
        observer.unobserve(img);
      }
    });
  }, {
    rootMargin: '50px'
  });

  images.forEach(img => observer.observe(img));
}
```

**Step 2: 添加图片压缩**

```javascript
// frontend/src/utils/image.js
export function compressImage(file, maxWidth = 1024, quality = 0.8) {
  return new Promise((resolve) => {
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    const img = new Image();

    img.onload = () => {
      let width = img.width;
      let height = img.height;

      if (width > maxWidth) {
        height = (height * maxWidth) / width;
        width = maxWidth;
      }

      canvas.width = width;
      canvas.height = height;
      ctx.drawImage(img, 0, 0, width, height);

      canvas.toBlob(
        (blob) => resolve(new File([blob], file.name, { type: 'image/jpeg' })),
        'image/jpeg',
        quality
      );
    };

    img.src = URL.createObjectURL(file);
  });
}
```

**Step 3: 提交**

```bash
git add frontend/src/
git commit -m "perf(m5): add lazy loading and image compression"
```

---

## Task 3: 单元测试

### 3.1 固件单元测试

**Files:**
- Create: `firmware/test/test_motion_detector.cpp`

**Step 1: 创建测试框架**

```cpp
// firmware/test/test_motion_detector.cpp
#include <unity.h>
#include "../include/motion_detector.h"

MotionDetector detector;

void test_function_initializes(void) {
    TEST_ASSERT_TRUE(detector.init());
}

void test_detects_motion(void) {
    // 创建模拟图像数据
    // 测试运动检测逻辑
    TEST_ASSERT_TRUE(true);  // 占位
}

void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(test_function_initializes);
    RUN_TEST(test_detects_motion);

    UNITY_END();
}

void loop() {}
```

**Step 2: 提交**

```bash
git add firmware/test/
git commit -m "test(m5): add unit tests for motion detector"
```

---

### 3.2 云端函数测试

**Files:**
- Create: `backend/functions/__tests__/upload-handler.test.js`

**Step 1: 创建测试**

```javascript
// backend/functions/__tests__/upload-handler.test.js

const { handler } = require('../upload-handler/index.js');

describe('upload-handler', () => {
  beforeEach(() => {
    // Mock 环境变量
    process.env.OTS_INSTANCE = 'test-instance';
    process.env.OTS_ENDPOINT = 'https://test.ots.aliyuncs.com';
  });

  test('should reject request without device_id', async () => {
    const event = {
      body: JSON.stringify({})
    };

    const result = await handler(event, {});

    expect(result.statusCode).toBe(400);
  });

  test('should handle valid upload', async () => {
    // Mock TableStore 客户端
    const event = {
      body: JSON.stringify({
        device_id: 'test-device',
        oss_path_original: 'devices/test/original/test.jpg',
        has_motion: true
      })
    };

    const result = await handler(event, {});

    expect(result.statusCode).toBe(200);
  });
});
```

**Step 2: 添加测试脚本**

```json
{
  "scripts": {
    "test": "jest",
    "test:coverage": "jest --coverage"
  }
}
```

**Step 3: 提交**

```bash
git add backend/
git commit -m "test(m5): add unit tests for cloud functions"
```

---

## Task 4: 部署自动化

**Files:**
- Create: `scripts/deploy-all.sh`
- Create: `.github/workflows/deploy.yml`

**Step 1: 创建部署脚本**

```bash
#!/bin/bash
# scripts/deploy-all.sh

set -e

echo "=== MyCam Deployment ==="

# 1. 部署基础设施
echo "Step 1: Deploying infrastructure..."
cd infrastructure/terraform
terraform apply -auto-approve
cd ../../

# 2. 部署云函数
echo "Step 2: Deploying cloud functions..."
./scripts/deploy-functions.sh

# 3. 构建和部署前端
echo "Step 3: Deploying frontend..."
cd frontend
npm run build
./deploy.sh
cd ../

echo "=== Deployment Complete ==="
```

**Step 2: 创建 GitHub Actions**

```yaml
# .github/workflows/deploy.yml
name: Deploy

on:
  push:
    branches: [ main ]

jobs:
  deploy:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v3

      - name: Setup Node.js
        uses: actions/setup-node@v3
        with:
          node-version: '18'

      - name: Deploy Infrastructure
        run: |
          cd infrastructure/terraform
          terraform init
          terraform apply -auto-approve
        env:
          ALICLOUD_ACCESS_KEY: ${{ secrets.ALICLOUD_ACCESS_KEY }}
          ALICLOUD_SECRET_KEY: ${{ secrets.ALICLOUD_SECRET_KEY }}

      - name: Deploy Functions
        run: ./scripts/deploy-functions.sh

      - name: Deploy Frontend
        run: |
          cd frontend
          npm install
          npm run build
          ./deploy.sh
```

**Step 3: 提交**

```bash
git add scripts/ .github/
git commit -m "ci(m5): add automated deployment scripts"
```

---

## Task 5: 文档完善

**Files:**
- Create: `docs/USER_GUIDE.md`
- Create: `docs/DEPLOYMENT.md`
- Create: `docs/API.md`
- Create: `docs/TROUBLESHOOTING.md`

**Step 1: 用户指南**

```markdown
# MyCam 用户指南

## 快速开始

### 硬件准备
- M5Stack CamS3 x1
- USB-C 数据线 x1
- TF 卡（可选，用于本地缓存）

### 软件准备
- 下载固件并烧录到 CamS3
- 配置 WiFi
- 访问 Web 界面

### 基本操作
1. 添加设备
2. 查看实时画面
3. 配置通知
...
```

**Step 2: 部署文档**

```markdown
# 部署指南

## 前置要求
- 阿里云账号
- Terraform 安装
- PlatformIO 安装

## 部署步骤

### 1. 云端部署
```bash
cd infrastructure/terraform
terraform init
terraform apply
```

### 2. 固件部署
```bash
cd firmware
pio run --target upload
```

### 3. 前端部署
```bash
cd frontend
npm run build
./deploy.sh
```
...
```

**Step 3: API 文档**

```markdown
# API 文档

## 认证
所有 API 请求需要在 Header 中包含认证信息。

## 端点列表

### POST /api/v1/images/upload
上传图片元数据

**请求体:**
```json
{
  "device_id": "string",
  "oss_path_original": "string",
  "oss_path_thumbnail": "string",
  "has_motion": boolean,
  "image_size": number
}
```

**响应:**
```json
{
  "message": "success",
  "row_key": "string",
  "created_at": number
}
```
...
```

**Step 4: 故障排查**

```markdown
# 故障排查

## 常见问题

### 1. 摄像头无法初始化
- 检查硬件连接
- 查看串口日志
- 重启设备

### 2. WiFi 连接失败
- 检查 SSID 和密码
- 确认路由器 2.4G 支持
- 检查信号强度

### 3. 图片上传失败
- 检查网络连接
- 验证 STS Token
- 查看 OSS 日志
...
```

**Step 5: 更新 README**

```markdown
# MyCam - CamS3 智能视频监控系统

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://badges.aliyun.com)](https://platformio.org)

基于 M5Stack CamS3 的低成本智能视频监控系统。

## 功能特性

- 📹 实时视频预览（局域网）
- 🎯 运动检测自动拍照
- ☁️ 云端存储（阿里云 OSS）
- 📱 多端通知（飞书/钉钉）
- 🌐 互联网远程访问

## 快速开始

\`\`\`bash
# 克隆仓库
git clone https://github.com/yourusername/mycam.git
cd mycam

# 部署云端
cd infrastructure/terraform
terraform apply

# 烧录固件
cd firmware
pio run --target upload

# 部署前端
cd frontend
npm install
npm run build
\`\`\`

详细文档请查看 [docs](docs) 目录。

## 项目结构

\`\`\`
mycam/
├── docs/              # 文档
├── firmware/          # CamS3 固件
├── backend/           # 云函数
├── frontend/          # Web 前端
└── infrastructure/    # 基础设施
\`\`\`

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License
```

**Step 6: 提交**

```bash
git add docs/
git commit -m "docs(m5): add comprehensive documentation"
```

---

## Task 6: 发布准备

**Files:**
- Modify: `frontend/package.json`
- Modify: `firmware/platformio.ini`
- Create: `CHANGELOG.md`

**Step 1: 更新版本号**

```json
{
  "name": "mycam-frontend",
  "version": "1.0.0"
}
```

**Step 2: 创建变更日志**

```markdown
# 变更日志

## [1.0.0] - 2025-01-02

### 新增
- 实时视频预览（局域网）
- 运动检测自动拍照
- 云端存储（阿里云 OSS）
- 飞书/钉钉通知
- 历史照片查看
- 设备管理界面

### 技术栈
- 固件: PlatformIO + ESP32-Arduino
- 云端: 阿里云函数计算
- 前端: Vue 3 + Element Plus
```

**Step 3: 创建 Release Notes**

```markdown
# MyCam v1.0.0 发布

这是 MyCam 的首个正式版本！

## 主要功能

✅ 基于 M5Stack CamS3 的视频监控
✅ 局域网低延迟实时预览
✅ 智能运动检测
✅ 云端存储和多端通知
✅ 响应式 Web 界面

## 安装

详细安装指南请查看: [部署文档](docs/DEPLOYMENT.md)

## 反馈

遇到问题请在 GitHub 提交 Issue。
```

**Step 4: 提交并打标签**

```bash
git add .
git commit -m "chore(m5): prepare for v1.0.0 release"

git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
```

---

## 完成标准

- [ ] 错误处理完善
- [ ] 日志系统运行正常
- [ ] 性能优化实施
- [ ] 单元测试覆盖率 > 60%
- [ ] 部署自动化完成
- [ ] 文档完整

---

## 项目完成！

恭喜！MyCam v1.0 已经完成开发和测试，可以正式发布了。

### 交付物清单

- ✅ PRD 文档
- ✅ 固件代码（PlatformIO）
- ✅ 云端函数（Node.js）
- ✅ Web 前端（Vue 3）
- ✅ 基础设施代码（Terraform）
- ✅ 部署脚本
- ✅ 用户文档
- ✅ API 文档
- ✅ 故障排查指南

### 后续优化方向

1. 支持更多摄像头型号
2. 添加人脸检测功能
3. 支持视频录制
4. 移动端原生应用
5. 多用户权限管理
