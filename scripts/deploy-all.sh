#!/bin/bash
# scripts/deploy-all.sh
# 完整部署脚本 - 部署所有组件

set -e

echo "======================================"
echo "  MyCam 完整部署脚本"
echo "======================================"

# 检查必需的工具
command -v terraform >/dev/null 2>&1 || { echo "错误: Terraform 未安装"; exit 1; }
command -v pio >/dev/null 2>&1 || { echo "错误: PlatformIO 未安装"; exit 1; }
command -v npm >/dev/null 2>&1 || { echo "错误: Node.js 未安装"; exit 1; }

# 1. 部署基础设施
echo ""
echo "[1/4] 部署云基础设施..."
cd infrastructure/terraform
terraform init
terraform apply -auto-approve
cd ../../

echo "✓ 基础设施部署完成"

# 2. 构建并部署前端
echo ""
echo "[2/4] 构建和部署前端..."
cd frontend
npm install
npm run build

# 检查是否配置了 OSS
if [ -z "$OSS_BUCKET" ]; then
    echo "警告: 未设置 OSS_BUCKET 环境变量，跳过前端部署"
    echo "请手动部署 frontend/dist/ 目录到 OSS"
else
    chmod +x deploy.sh
    ./deploy.sh
fi
cd ../

echo "✓ 前端部署完成"

# 3. 提示烧录固件
echo ""
echo "[3/4] 固件烧录..."
echo "请手动执行以下步骤："
echo "  1. 连接 CamS3 设备到电脑"
echo "  2. 编辑 firmware/include/wifi_config.h 配置 WiFi"
echo "  3. 运行: cd firmware && pio run --target upload"
echo ""

# 4. 配置说明
echo ""
echo "[4/4] 后续配置..."
echo "1. 在阿里云函数计算控制台配置环境变量"
echo "2. 在前端设置页面配置通知 Webhook"
echo "3. 注册设备到系统"

echo ""
echo "======================================"
echo "  部署完成！"
echo "======================================"
echo ""
echo "下一步："
echo "  - 查看部署日志: terraform outputs"
echo "  - 访问前端: https://your-domain.com"
echo "  - 阅读文档: docs/DEPLOYMENT.md"
echo ""
