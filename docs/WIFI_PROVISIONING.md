# WiFi Provisioning 使用指南

## 概述

MyCam 设备支持通过手机 web 页面配置 WiFi，无需修改代码重新编译固件。

## 工作流程

### 首次配置

1. **给设备上电**
2. **设备进入 AP 模式**，创建名为 "MyCam-xxxxxx" 的 WiFi 热点（xxxxxx 是 MAC 地址后 6 位）
3. **手机连接设备热点**，密码无需（开放网络）
4. **浏览器自动打开配置页面** (或手动访问 `http://192.164.4.1`)
5. **选择目标 WiFi**，输入密码
6. **点击连接**，设备保存配置并切换到 STA 模式
7. **连接成功后**，设备 AP 关闭，手机自动切换回原 WiFi

### 后续使用

- 设备启动时自动连接已保存的 WiFi
- 如果连接失败（30秒超时），自动进入 AP 模式重新配置

### 重置配置

通过前端设置页面的"忘记 WiFi"按钮，或在设备 API 调用:

```bash
POST /api/wifi/reset
```

## 技术细节

### 配置存储

- **位置**: SPIFFS 文件系统 `/wifi_config.json`
- **格式**: JSON
- **内容**:
  ```json
  {
    "ssid": "YourWiFi",
    "password": "yourpassword",
    "saved": true,
    "lastSeen": 1704207600000
  }
  ```

### AP 模式参数

- **SSID**: MyCam-<MAC后6位>
- **IP 地址**: 192.164.4.1
- **子网掩码**: 255.255.255.0
- **Captive Portal**: 启用（DNS 劫持所有域名）

### API 端点

| 端点 | 方法 | 功能 |
|------|------|------|
| `/provision` | GET | WiFi 配置页面 |
| `/api/wifi/scan` | GET | 扫描附近 WiFi |
| `/api/wifi/config` | GET | 获取当前配置 |
| `/api/wifi/config` | POST | 保存并连接 |
| `/api/wifi/reset` | POST | 清除配置并重启 |
| `/api/wifi/status` | GET | 获取连接状态 |

## 故障排查

### 无法扫描到 WiFi

- 检查设备是否正常启动
- 查看 Serial Monitor 日志
- 确认 WiFi 模块工作正常

### 连接后立即掉线

- 检查 WiFi 密码是否正确
- 检查路由器是否支持 2.4GHz（ESP32 不支持 5GHz）
- 检查路由器是否启用了 MAC 地址过滤

### 配置页面无法打开

- 确认手机已连接到 "MyCam-xxxxxx" 热点
- 尝试手动访问 `http://192.164.4.1`
- 清除浏览器缓存后重试

### 设备频繁重启

- 检查电源供电是否稳定
- 查看 Serial Monitor 错误日志
- 检查看门狗是否触发

## 开发相关

### 编译和上传

```bash
cd firmware
pio run --target upload
```

### 查看日志

```bash
pio device monitor
```

### 清除配置

```bash
# 通过串口
pio device monitor
# 设备启动后，调用 POST /api/wifi/reset

# 或重新烧录固件
pio run --target upload
```
