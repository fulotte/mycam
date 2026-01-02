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

详见 [PRD 文档](docs/plans/2025-01-02-cams3-monitor-prd.md)

## 开发状态

- [ ] M1: 固件基础
- [ ] M2: 云端基础
- [ ] M3: 通知集成
- [ ] M4: 前端基础
- [ ] M5: 完善优化

## License

MIT
