# MyCam API 文档

本文档描述 MyCam 系统的 API 接口。

## 基础信息

- **Base URL**: `https://your-api-gateway-id.cn-hangzhou.aliyuncs.com/prod`
- **Content-Type**: `application/json`
- **字符编码**: UTF-8

## 认证

当前版本不需要认证。未来版本将添加 API Key 或 JWT 认证。

## API 端点

### 1. 图片上传

#### POST /api/v1/images/upload

上传图片元数据到云端。

**请求头:**
```
Content-Type: application/json
```

**请求体:**
```json
{
  "device_id": "cams3-001",
  "oss_path_original": "devices/cams3-001/20250102_123456_original.jpg",
  "oss_path_thumbnail": "devices/cams3-001/20250102_123456_thumb.jpg",
  "has_motion": true,
  "image_size": 102400
}
```

**字段说明:**
- `device_id` (string, 必需): 设备 ID
- `oss_path_original` (string, 必需): OSS 原图路径
- `oss_path_thumbnail` (string, 可选): OSS 缩略图路径
- `has_motion` (boolean, 可选): 是否检测到运动
- `image_size` (number, 可选): 图片大小（字节）

**成功响应 (200):**
```json
{
  "message": "success",
  "row_key": "1704192356000-abc123",
  "created_at": 1704192356000
}
```

**错误响应 (400):**
```json
{
  "error": "Missing required fields: device_id or oss_path_original"
}
```

---

### 2. 图片列表

#### GET /api/v1/images/list

查询图片列表。

**查询参数:**
- `device_id` (string, 可选): 设备 ID
- `start_time` (number, 可选): 开始时间戳（毫秒）
- `end_time` (number, 可选): 结束时间戳（毫秒）
- `limit` (number, 可选): 返回数量限制，默认 50，最大 100

**请求示例:**
```
GET /api/v1/images/list?device_id=cams3-001&limit=20
```

**成功响应 (200):**
```json
{
  "images": [
    {
      "partition_key": "cams3-001",
      "row_key": "1704192356000-abc123",
      "device_id": "cams3-001",
      "has_motion": true,
      "oss_path_original": "https://mycam-bucket.oss-cn-hangzhou.aliyuncs.com/devices/cams3-001/...",
      "oss_path_thumbnail": "https://mycam-bucket.oss-cn-hangzhou.aliyuncs.com/devices/cams3-001/...",
      "created_at": 1704192356000,
      "image_size": 102400
    }
  ],
  "next_token": "eyJwYXJ0aXRpb25fa2V5IjoiY2FtczMtMDAxIn0="
}
```

**错误响应 (400):**
```json
{
  "error": "device_id is required"
}
```

---

### 3. 获取设备 Token

#### POST /api/v1/device/token

获取用于直接上传 OSS 的 STS Token。

**请求体:**
```json
{
  "device_id": "cams3-001"
}
```

**成功响应 (200):**
```json
{
  "access_key_id": "STS.xxx",
  "access_key_secret": "xxx",
  "security_token": "xxx",
  "expiration": "2025-01-02T13:00:00Z",
  "allowed_prefix": "devices/cams3-001/"
}
```

**字段说明:**
- `access_key_id`: STS Access Key ID
- `access_key_secret`: STS Access Key Secret
- `security_token`: STS Security Token
- `expiration`: Token 过期时间（ISO 8601 格式）
- `allowed_prefix`: 允许上传的 OSS 前缀

---

### 4. 设备注册

#### POST /api/v1/device/register

注册新设备到系统。

**请求体:**
```json
{
  "device_id": "cams3-001",
  "device_name": "客厅摄像头",
  "location": "客厅"
}
```

**成功响应 (200):**
```json
{
  "message": "Device registered successfully",
  "device_id": "cams3-001"
}
```

---

### 5. 获取设备配置

#### GET /api/v1/device/{device_id}/config

获取设备配置。

**路径参数:**
- `device_id`: 设备 ID

**成功响应 (200):**
```json
{
  "device_id": "cams3-001",
  "device_name": "客厅摄像头",
  "location": "客厅",
  "notify_enabled": "true",
  "feishu_webhook": "https://open.feishu.cn/open-apis/bot/v2/hook/xxx",
  "dingtalk_webhook": "https://oapi.dingtalk.com/robot/send?access_token=xxx",
  "notify_feishu": "true",
  "notify_dingtalk": "false",
  "updated_at": 1704192356000
}
```

**错误响应 (404):**
```json
{
  "error": "Device not found"
}
```

---

### 6. 更新设备配置

#### POST /api/v1/device/{device_id}/config

创建或替换设备配置。

**路径参数:**
- `device_id`: 设备 ID

**请求体:**
```json
{
  "device_name": "客厅摄像头",
  "location": "客厅",
  "notify_enabled": true,
  "feishu_webhook": "https://open.feishu.cn/open-apis/bot/v2/hook/xxx",
  "dingtalk_webhook": "https://oapi.dingtalk.com/robot/send?access_token=xxx"
}
```

**成功响应 (200):**
```json
{
  "message": "Device config saved",
  "config": {
    "device_id": "cams3-001",
    "device_name": "客厅摄像头",
    "notify_enabled": true,
    "feishu_webhook": "https://open.feishu.cn/open-apis/bot/v2/hook/xxx"
  }
}
```

---

### 7. 部分更新设备配置

#### PATCH /api/v1/device/{device_id}/config

部分更新设备配置。

**路径参数:**
- `device_id`: 设备 ID

**请求体:**
```json
{
  "notify_enabled": false
}
```

**成功响应 (200):**
```json
{
  "message": "Device config updated",
  "config": {
    "device_id": "cams3-001",
    "device_name": "客厅摄像头",
    "notify_enabled": false,
    "feishu_webhook": "https://open.feishu.cn/open-apis/bot/v2/hook/xxx"
  }
}
```

---

## 设备端 HTTP 接口

### 1. 实时视频流

#### GET /stream

获取实时视频流（BMP 格式）。

**响应:**
- Content-Type: `image/bmp`
- 每秒更新 3 次（可配置）

**示例:**
```html
<img src="http://cams3.local/stream" />
```

---

### 2. 运动状态

#### GET /motion

获取当前运动检测状态。

**成功响应 (200):**
```json
{
  "motion": true,
  "timestamp": 1704192356000
}
```

---

### 3. 设备信息

#### GET /info

获取设备信息。

**成功响应 (200):**
```json
{
  "device_id": "cams3-001",
  "version": "1.0.0",
  "uptime": 3600,
  "free_heap": 180000,
  "wifi_rssi": -45,
  "ip_address": "192.168.1.100"
}
```

---

## 错误码

| HTTP 状态码 | 说明 |
|------------|------|
| 200 | 请求成功 |
| 400 | 请求参数错误 |
| 404 | 资源不存在 |
| 500 | 服务器内部错误 |

**错误响应格式:**
```json
{
  "error": "错误描述信息"
}
```

---

## 速率限制

- API Gateway 限制：每秒 100 请求
- 设备注册限制：每小时 10 次

---

## 更新日志

### v1.0.0 (2025-01-02)
- 初始版本发布
- 支持图片上传、查询、设备管理
- 支持飞书/钉钉通知
