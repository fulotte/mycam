# MyCam 部署指南

本文档详细说明如何部署 MyCam 监控系统。

## 前置要求

### 必需软件

- **PlatformIO** - 用于固件编译和烧录
  ```bash
  pip install platformio
  ```

- **Node.js 18+** - 用于前端构建
  ```bash
  # macOS
  brew install node

  # Linux
  # 使用 nvm 安装
  curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.0/install.sh | bash
  nvm install 18
  ```

- **Terraform** - 用于基础设施部署
  ```bash
  # macOS
  brew install terraform

  # Linux
  wget https://releases.hashicorp.com/terraform/1.5.0/terraform_1.5.0_linux_amd64.zip
  unzip terraform_1.5.0_linux_amd64.zip
  sudo mv terraform /usr/local/bin/
  ```

- **阿里云 CLI** - 用于 OSS 上传
  ```bash
  pip install aliyun-cli
  aliyun configure
  ```

### 必需账号

- 阿里云账号（需开通以下服务）：
  - 函数计算
  - 对象存储 OSS
  - Tablestore
  - API 网关
  - RAM（访问控制）

## 第一步：部署云基础设施

### 1.1 配置阿里云凭证

```bash
export ALICLOUD_ACCESS_KEY="your-access-key-id"
export ALICLOUD_SECRET_KEY="your-access-key-secret"
export ALICLOUD_REGION="cn-hangzhou"
```

### 1.2 初始化 Terraform

```bash
cd infrastructure/terraform
terraform init
```

### 1.3 查看部署计划

```bash
terraform plan
```

### 1.4 执行部署

```bash
terraform apply
# 输入 yes 确认
```

部署完成后，Terraform 会输出以下信息：
- OSS Bucket 名称
- Tablestore 实例名称
- 函数计算服务名称
- API 网关地址

### 1.5 配置云函数环境变量

在阿里云函数计算控制台，为每个函数配置环境变量：

**upload-handler:**
```bash
OTS_INSTANCE=mycam-ots-xxxxx
OTS_ENDPOINT=https://mycam-ots-xxxxx.cn-hangzhou.ots.aliyuncs.com
OTS_ACCESS_KEY_ID=your-access-key
OTS_SECRET_ACCESS_KEY=your-secret-key
NOTIFY_FUNCTION_URL=https://xxxxx.cn-hangzhou.fc.aliyuncs.com/2016-08-15/proxy/mycam/notify-sender/
```

**notify-sender:**
```bash
OTS_INSTANCE=mycam-ots-xxxxx
OTS_ENDPOINT=https://mycam-ots-xxxxx.cn-hangzhou.ots.aliyuncs.com
OTS_ACCESS_KEY_ID=your-access-key
OTS_SECRET_ACCESS_KEY=your-secret-key
```

**device-manager:**
```bash
OTS_INSTANCE=mycam-ots-xxxxx
OTS_ENDPOINT=https://mycam-ots-xxxxx.cn-hangzhou.ots.aliyuncs.com
OTS_ACCESS_KEY_ID=your-access-key
OTS_SECRET_ACCESS_KEY=your-secret-key
STS_ARN=acs:ram::xxxxx:role/device_upload_role
```

**image-query:**
```bash
OTS_INSTANCE=mycam-ots-xxxxx
OTS_ENDPOINT=https://mycam-ots-xxxxx.cn-hangzhou.ots.aliyuncs.com
OTS_ACCESS_KEY_ID=your-access-key
OTS_SECRET_ACCESS_KEY=your-secret-key
OSS_ENDPOINT=oss-cn-hangzhou.aliyuncs.com
OSS_BUCKET=mycam-bucket-xxxxx
```

**device-config:**
```bash
OTS_INSTANCE=mycam-ots-xxxxx
OTS_ENDPOINT=https://mycam-ots-xxxxx.cn-hangzhou.ots.aliyuncs.com
OTS_ACCESS_KEY_ID=your-access-key
OTS_SECRET_ACCESS_KEY=your-secret-key
```

## 第二步：烧录固件

### 2.1 配置 WiFi

编辑 `firmware/include/wifi_config.h`:

```cpp
#pragma once

#define WIFI_SSID "Your-WiFi-Name"
#define WIFI_PASSWORD "Your-WiFi-Password"
```

### 2.2 编译并上传

```bash
cd firmware

# 首次编译（下载依赖）
pio run

# 上传到设备
pio run --target upload

# 查看串口日志
pio device monitor
```

### 2.3 验证固件

连接 CamS3 的 USB 到电脑，串口监视器应显示：

```
[INFO][MAIN] CamS3 Monitor starting...
[INFO][MAIN] Camera initialized
[INFO][MAIN] Motion detector initialized
[INFO][MAIN] WiFi connected
[INFO][MAIN] HTTP server started
[INFO][MAIN] mDNS responder started: http://camS3.local/
[INFO][MAIN] Setup complete
```

### 2.4 访问设备

在浏览器访问：`http://camS3.local/` 或设备的 IP 地址

## 第三步：部署前端

### 3.1 安装依赖

```bash
cd frontend
npm install
```

### 3.2 配置 API 地址

编辑 `frontend/vite.config.js`，设置代理目标：

```javascript
server: {
  port: 3000,
  proxy: {
    '/api': {
      target: 'https://your-api-gateway-id.execute-api.cn-hangzhou.aliyuncs.com',
      changeOrigin: true
    }
  }
}
```

### 3.3 本地开发

```bash
npm run dev
```

访问 `http://localhost:3000`

### 3.4 生产构建

```bash
npm run build
```

### 3.5 部署到 OSS

编辑 `frontend/deploy.sh`，设置 bucket 名称：

```bash
BUCKET_NAME="mycam-bucket-xxxxx"
OSS_ENDPOINT="oss-cn-hangzhou.aliyuncs.com"
```

执行部署：

```bash
chmod +x deploy.sh
./deploy.sh
```

### 3.6 配置 CDN（可选）

在阿里云 CDN 控制台：
1. 添加域名：`your-domain.com`
2. 源站设置：OSS Bucket
3. 缓存配置：静态文件缓存 7 天
4. HTTPS 配置：添加 SSL 证书

## 第四步：配置通知

### 4.1 获取飞书 Webhook

1. 在飞书群设置中添加"自定义机器人"
2. 选择"安全设置"中的"签名验证"或"IP 白名单"
3. 复制 Webhook URL

### 4.2 获取钉钉 Webhook

1. 在钉钉群设置中添加"自定义机器人"
2. 安全设置选择"加签"
3. 复制 Webhook URL

### 4.3 在前端配置

1. 访问前端设置页面
2. 选择"通知设置"标签
3. 启用飞书/钉钉通知
4. 输入 Webhook URL
5. 点击"测试发送"验证

## 第五步：注册设备

首次使用时，设备需要向云端注册：

1. 设备启动后自动发送注册请求
2. 或通过 API 手动注册：

```bash
curl -X POST https://your-api-gateway.com/api/v1/device/register \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "cams3-001",
    "device_name": "客厅摄像头",
    "location": "客厅"
  }'
```

## 故障排查

部署过程中遇到问题？查看 [故障排查文档](TROUBLESHOOTING.md)。

## 成本估算

| 服务 | 规格 | 月成本 |
|------|------|--------|
| 函数计算 | 1M 调用/月 | ¥1-5 |
| OSS 存储 | 10GB | ¥0.3 |
| OSS 流量 | 10GB/月 | ¥0.8 |
| Tablestore | 100 CU | ¥15 |
| API 网关 | 1M 调用/月 | ¥1 |
| CDN | 按流量 | ¥5-10 |
| **总计** | | **¥25-35/月** |

单设备运行成本约 **¥5-10/月**。

## 下一步

部署完成后，建议：
1. 配置定期备份
2. 设置告警规则
3. 优化运动检测参数
4. 配置 HTTPS 证书
