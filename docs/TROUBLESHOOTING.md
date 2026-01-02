# MyCam 故障排查指南

本文档列出常见问题和解决方案。

## 固件问题

### 1. 摄像头无法初始化

**症状:** 串口日志显示 `Camera init failed!`

**可能原因:**
- 摄像头模块未正确连接
- PSRAM 未启用
- 摄像头引脚配置错误

**解决方案:**
1. 检查硬件连接
2. 确认 `platformio.ini` 中启用了 PSRAM:
   ```ini
   build_flags =
       -D BOARD_HAS_PSRAM
   ```
3. 检查 `firmware/src/camera.cpp` 中的引脚配置

### 2. WiFi 连接失败

**症状:** 日志显示 `WiFi connection failed`

**可能原因:**
- WiFi SSID 或密码错误
- 路由器不支持 2.4GHz
- 信号强度太弱

**解决方案:**
1. 检查 `firmware/include/wifi_config.h` 中的 WiFi 凭证
2. 确认路由器支持 2.4GHz（ESP32 不支持 5GHz）
3. 靠近路由器或使用 WiFi 中继器
4. 检查路由器的 MAC 过滤设置

### 3. 设备频繁重启

**症状:** 设备每隔几分钟重启

**可能原因:**
- 看门狗超时
- 内存不足
- 电源不稳定

**解决方案:**
1. 检查串口日志确认重启原因
2. 增加看门狗超时时间（`main.cpp` 中的 `esp_task_wdt_init`）
3. 检查电源供应（建议 5V 2A）
4. 优化内存使用（降低摄像头分辨率）

### 4. mDNS 无法访问

**症状:** 无法通过 `http://camS3.local/` 访问设备

**可能原因:**
- mDNS 服务未启动
- 防火墙阻止 mDNS
- 设备和设备不在同一网段

**解决方案:**
1. 使用 IP 地址直接访问（如 `http://192.168.1.100/`）
2. 检查路由器是否启用 mDNS/Bonjour
3. Windows 用户需安装 Bonjour 服务
4. 确认设备和电脑在同一子网

## 云端问题

### 1. 图片上传失败

**症状:** 设备端显示上传错误

**可能原因:**
- STS Token 过期
- OSS Bucket 配置错误
- 网络连接问题

**解决方案:**
1. 检查 Token 是否过期（有效期 15 分钟）
2. 重新请求 Token:
   ```bash
   curl -X POST https://your-api.com/api/v1/device/token \
     -d '{"device_id": "cams3-001"}'
   ```
3. 检查 OSS Bucket CORS 设置
4. 验证网络连接

### 2. Tablestore 写入失败

**症状:** 云函数日志显示 OTS 错误

**可能原因:**
- 表不存在
- 环境变量配置错误
- RAM 权限不足

**解决方案:**
1. 确认 Terraform 部署成功
2. 检查函数环境变量配置
3. 验证 RAM 角色权限:
   - OTS 实例读写权限
   - 正确的实例名称和表名

### 3. 通知未发送

**症状:** 运动检测后未收到通知

**可能原因:**
- Webhook URL 错误
- 设备配置中通知未启用
- Webhook 被限流

**解决方案:**
1. 检查设备配置:
   ```bash
   curl https://your-api.com/api/v1/device/cams3-001/config
   ```
2. 验证 Webhook URL 有效性
3. 在设置页面使用"测试发送"功能
4. 检查飞书/钉钉机器人的限流规则

### 4. API 请求超时

**症状:** 前端显示"网络错误"

**可能原因:**
- API 网关未正确配置
- 函数计算冷启动
- 网络问题

**解决方案:**
1. 检查 API 网关部署状态
2. 增加函数计算内存和超时时间
3. 配置函数计算预留实例避免冷启动
4. 检查前端 API 代理配置

## 前端问题

### 1. 无法连接到设备

**症状:** 预览页面显示"未连接"

**可能原因:**
- 设备 IP 变化
- 设备离线
- 跨域问题

**解决方案:**
1. 刷新设备列表获取最新 IP
2. 检查设备是否在线
3. 确认设备和前端在同一网络
4. 检查浏览器控制台的错误信息

### 2. 图片加载失败

**症状:** 图片列表显示空白或加载错误

**可能原因:**
- OSS 签名 URL 过期
- 网络问题
- 图片不存在

**解决方案:**
1. 刷新页面重新获取 URL
2. 检查网络连接
3. 验证图片是否存在于 OSS
4. 检查 OSS Bucket 访问权限

### 3. 设置保存失败

**症状:** 修改设置后提示"保存失败"

**可能原因:**
- API 请求失败
- 权限问题
- 数据验证失败

**解决方案:**
1. 检查浏览器控制台的错误
2. 验证 API 端点可用性
3. 检查输入格式是否正确
4. 确认设备已注册

## 性能问题

### 1. 视频流卡顿

**解决方案:**
1. 降低帧率：修改 `STREAM_FPS` 为 1 或 2
2. 降低分辨率：修改 `IMAGE_WIDTH` 和 `IMAGE_HEIGHT`
3. 使用有线网络代替 WiFi
4. 靠近路由器改善信号

### 2. 运动检测误报

**解决方案:**
1. 调整检测阈值：修改 `MOTION_THRESHOLD`
2. 调整触发网格数：修改 `MOTION_TRIGGER_COUNT`
3. 改善光线条件
4. 调整摄像头角度避免移动物体

### 3. 设备响应慢

**解决方案:**
1. 重启设备
2. 检查可用内存（`/info` 端点）
3. 降低日志级别
4. 优化代码性能

## 调试技巧

### 查看串口日志

```bash
# 使用 PlatformIO
pio device monitor

# 或使用串口工具（Linux/Mac）
screen /dev/ttyUSB0 115200

# 或使用串口工具（Windows）
# 使用 PuTTY 或 Tera Term
```

### 查看云函数日志

1. 登录阿里云控制台
2. 进入函数计算服务
3. 选择函数 → 日志查询
4. 查看实时日志和历史日志

### 网络调试

```bash
# 测试 API 连通性
curl -v https://your-api.com/api/v1/images/list?device_id=cams3-001

# 测试设备连接
curl -v http://cams3.local/info

# 测试 mDNS
nslookup camS3.local
```

### 浏览器调试

1. 打开开发者工具（F12）
2. 查看 Console 标签的错误信息
3. 查看 Network 标签的请求状态
4. 查看 Application 标签的本地存储

## 获取帮助

如果以上方案无法解决问题：

1. **查看 GitHub Issues**: [https://github.com/yourusername/mycam/issues](https://github.com/yourusername/mycam/issues)
2. **提交新 Issue**: 包含以下信息
   - 设备型号和固件版本
   - 完整的错误日志
   - 复现步骤
   - 已尝试的解决方案

3. **社区讨论**: 欢迎在 Discussions 中交流经验
