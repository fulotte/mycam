# mycam - CamS3 智能视频监控系统

基于 M5Stack CamS3（ESP32-S3）的低成本智能视频监控系统。

## 项目结构

```
mycam-repo/
├── docs/           # 文档（PRD、设计文档、API 文档）
├── firmware/       # CamS3 固件（PlatformIO + ESP32-Arduino）
├── backend/        # 云端函数（阿里云 FC + Node.js）
├── frontend/       # Web 前端（Vue 3 + Vite）
└── infrastructure/ # 基础设施代码（Terraform）
```

## 快速开始

### WiFi 配置

1. 给 CamS3 设备上电
2. 手机连接 WiFi "MyCam-xxxxxx"
3. 浏览器自动打开配置页面（或访问 http://192.164.4.1）
4. 选择目标 WiFi 并输入密码
5. 等待连接成功

详细说明请查看 [WiFi Provisioning 指南](docs/WIFI_PROVISIONING.md)

## 开发状态

- [ ] M1: 固件基础
- [ ] M2: 云端基础
- [ ] M3: 通知集成
- [ ] M4: 前端基础
- [ ] M5: 完善优化

## License

MIT
