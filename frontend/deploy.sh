#!/bin/bash
# frontend/deploy.sh

echo "Building frontend..."

# 安装依赖
npm install

# 构建
npm run build

# 上传到 OSS
echo "Uploading to OSS..."

BUCKET_NAME="your-bucket-name"
OSS_ENDPOINT="oss-cn-hangzhou.aliyuncs.com"

# 使用 aliyun CLI 上传
aliyun oss cp dist/ oss://$BUCKET_NAME/frontend/ -r -f --update

# 刷新 CDN
echo "Refreshing CDN..."
aliyun cdn RefreshObjectCaches --ObjectPath "http://$BUCKET_NAME.$OSS_ENDPOINT/frontend/*"

echo "Deploy complete!"
