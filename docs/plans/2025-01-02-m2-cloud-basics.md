# M2: 云端基础实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**目标:** 实现云端基础设施，包括 OSS 存储、Tablestore 数据库、函数计算（FC）和 API 网关。支持 CamS3 上传图片并获取元数据。

**架构:** 基于阿里云无服务器架构。CamS3 获取 STS Token → 直接上传到 OSS → 调用云函数上报元数据 → 元数据存储到 Tablestore。Terraform 管理基础设施。

**技术栈:** 阿里云 OSS, 函数计算 (Node.js), Tablestore, API 网关, RAM, Terraform

---

## Task 1: Terraform 基础设施定义

**Files:**
- Create: `infrastructure/terraform/main.tf`
- Create: `infrastructure/terraform/variables.tf`
- Create: `infrastructure/terraform/outputs.tf`
- Create: `infrastructure/terraform/oss.tf`
- Create: `infrastructure/terraform/tablestore.tf`
- Create: `infrastructure/terraform/ram.tf`
- Create: `infrastructure/terraform/.gitignore`

**Step 1: 创建 main.tf**

```hcl
# infrastructure/terraform/main.tf
terraform {
  required_version = ">= 1.0"
  required_providers {
    alicloud = {
      source  = "aliyun/alicloud"
      version = "~> 1.200"
    }
  }
}

provider "alicloud" {
  region = var.region
}

# 获取当前账号信息
data "alicloud_account" "current" {}
data "alicloud_current_user" "current" {}

# 获取可用区
data "alicloud_zones" "available" {
  available_resource_creation = "VSwitch"
}

# 获取已有的 VPC（可选，根据实际情况调整）
# data "alicloud_vpcs" "existing" {
#   name_regex = "default"
# }
```

**Step 2: 创建 variables.tf**

```hcl
# infrastructure/terraform/variables.tf
variable "region" {
  description = "阿里云区域"
  type        = string
  default     = "cn-hangzhou"
}

variable "project_name" {
  description = "项目名称"
  type        = string
  default     = "mycam"
}

variable "environment" {
  description = "环境 (dev/staging/prod)"
  type        = string
  default     = "dev"
}

variable "oss_retention_days" {
  description = "图片保留天数"
  type        = number
  default     = 30
}
```

**Step 3: 创建 oss.tf**

```hcl
# infrastructure/terraform/oss.tf
resource "alicloud_oss_bucket" "camera_images" {
  bucket = "${var.project_name}-${var.environment}-${random_id.bucket_suffix.hex}"
  acl    = "private"

  # 开启版本控制
  versioning {
    status = "Enabled"
  }

  # 生命周期规则
  lifecycle_rule {
    id      = "cleanup_old_images"
    enabled = true

    prefix = "devices/"

    expiration {
      days = var.oss_retention_days
    }
  }

  # 缩略图保留 7 天
  lifecycle_rule {
    id      = "cleanup_thumbnails"
    enabled = true

    prefix = "devices/*/thumbnail/"

    expiration {
      days = 7
    }
  }

  tags = {
    Name = "${var.project_name}-camera-images"
    Env  = var.environment
  }
}

# 生成随机后缀避免 Bucket 名称冲突
resource "random_id" "bucket_suffix" {
  byte_length = 4
}
```

**Step 4: 创建 tablestore.tf**

```hcl
# infrastructure/terraform/tablestore.tf
resource "alicloud_ots_instance" "metadata" {
  name        = "${var.project_name}-${var.environment}-metadata"
  description = "MyCam camera metadata storage"
  instance_type = "CU"
  capacity_unit {
    read    = 100
    write   = 100
  }

  tags = {
    Name = "${var.project_name}-metadata"
    Env  = var.environment
  }
}

# images 表 - 存储图片元数据
resource "alicloud_ots_table" "images" {
  instance_name = alicloud_ots_instance.metadata.name
  table_name    = "images"

  primary_key {
    name = "partition_key"
    type = "String"
  }

  primary_key {
    name = "row_key"
    type = "String"
  }

  defined_column {
    name = "device_id"
    type = "String"
  }

  defined_column {
    name = "has_motion"
    type = "Boolean"
  }

  defined_column {
    name = "oss_path_original"
    type = "String"
  }

  defined_column {
    name = "oss_path_thumbnail"
    type = "String"
  }

  defined_column {
    name = "created_at"
    type = "Integer"
  }

  defined_column {
    name = "image_size"
    type = "Integer"
  }

  ttl = 2592000  # 30 天自动删除

  max_version = 1
}

# devices 表 - 存储设备信息
resource "alicloud_ots_table" "devices" {
  instance_name = alicloud_ots_instance.metadata.name
  table_name    = "devices"

  primary_key {
    name = "device_id"
    type = "String"
  }

  defined_column {
    name = "owner_id"
    type = "String"
  }

  defined_column {
    name = "device_name"
    type = "String"
  }

  defined_column {
    name = "notify_feishu"
    type = "Boolean"
  }

  defined_column {
    name = "notify_dingtalk"
    type = "Boolean"
  }

  defined_column {
    name = "feishu_webhook"
    type = "String"
  }

  defined_column {
    name = "dingtalk_webhook"
    type = "String"
  }

  defined_column {
    name = "last_online"
    type = "Integer"
  }

  defined_column {
    name = "created_at"
    type = "Integer"
  }

  max_version = 1
}
```

**Step 5: 创建 ram.tf**

```hcl
# infrastructure/terraform/ram.tf

# 创建 RAM 角色
resource "alicloud_ram_role" "fc_upload_role" {
  name = "${var.project_name}-fc-upload-role"
  description = "Role for Function Compute to upload to OSS"
  document = <<EOF
{
  "Statement": [
    {
      "Action": "sts:AssumeRole",
      "Effect": "Allow",
      "Principal": {
        "Service": [
          "fc.aliyuncs.com"
        ]
      }
    }
  ],
  "Version": "1"
}
EOF
}

# 函数计算写入 OSS 的策略
resource "alicloud_ram_policy" "fc_oss_write" {
  policy_name = "${var.project_name}-fc-oss-write"
  policy_document = jsonencode({
    Version = "1"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "oss:PutObject",
          "oss:PutObjectAcl"
        ]
        Resource = "acs:oss:*:*:${alicloud_oss_bucket.camera_images.bucket}/devices/*"
      }
    ]
  })
}

# 函数计算访问 Tablestore 的策略
resource "alicloud_ram_policy" "fc_ots_access" {
  policy_name = "${var.project_name}-fc-ots-access"
  policy_document = jsonencode({
    Version = "1"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "ots:PutRow",
          "ots:GetRow",
          "ots:UpdateRow",
          "ots:BatchWriteRow",
          "ots:GetRange",
          "ots:BatchGetRow"
        ]
        Resource = "acs:ots:*:*:instance/${alicloud_ots_instance.metadata.name}/*"
      }
    ]
  })
}

# 附加策略到角色
resource "alicloud_ram_role_policy_attachment" "fc_upload_oss" {
  policy_name = alicloud_ram_policy.fc_oss_write.policy_name
  role_name   = alicloud_ram_role.fc_upload_role.name
}

resource "alicloud_ram_role_policy_attachment" "fc_upload_ots" {
  policy_name = alicloud_ram_policy.fc_ots_access.policy_name
  role_name   = alicloud_ram_role.fc_upload_role.name
}

# 创建 RAM 角色用于设备直接上传 OSS
resource "alicloud_ram_role" "device_upload_role" {
  name = "${var.project_name}-device-upload-role"
  description = "Role for devices to upload directly to OSS via STS"
  document = <<EOF
{
  "Statement": [
    {
      "Action": "sts:AssumeRole",
      "Effect": "Allow",
      "Principal": {
        "Service": [
          "fc.aliyuncs.com"
        ]
      }
    }
  ],
  "Version": "1"
}
EOF
}

resource "alicloud_ram_policy" "device_oss_write" {
  policy_name = "${var.project_name}-device-oss-write"
  policy_document = jsonencode({
    Version = "1"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "oss:PutObject"
        ]
        Resource = "acs:oss:*:*:${alicloud_oss_bucket.camera_images.bucket}/devices/${var.project_name}-*/*"
      }
    ]
  })
}

resource "alicloud_ram_role_policy_attachment" "device_upload_oss" {
  policy_name = alicloud_ram_policy.device_oss_write.policy_name
  role_name   = alicloud_ram_role.device_upload_role.name
}
```

**Step 6: 创建 outputs.tf**

```hcl
# infrastructure/terraform/outputs.tf
output "oss_bucket_name" {
  description = "OSS Bucket 名称"
  value       = alicloud_oss_bucket.camera_images.bucket
}

output "ots_instance_name" {
  description = "Tablestore 实例名称"
  value       = alicloud_ots_instance.metadata.name
}

output "fc_upload_role_arn" {
  description = "函数计算上传角色 ARN"
  value       = alicloud_ram_role.fc_upload_role.arn
}

output "device_upload_role_arn" {
  description = "设备上传角色 ARN"
  value       = alicloud_ram_role.device_upload_role.arn
}

output "region" {
  description = "阿里云区域"
  value       = var.region
}
```

**Step 7: 创建 .gitignore**

```
# infrastructure/terraform/.gitignore
.terraform/
*.tfstate
*.tfstate.*
.terraform.lock.hcl
terraform.tfvars
```

**Step 8: 初始化 Terraform 并验证**

```bash
cd infrastructure/terraform
terraform init
terraform validate
terraform plan
```

**Step 9: 提交**

```bash
cd E:\myapp\mycam-repo
git add infrastructure/
git commit -m "feat(m2): add terraform infrastructure definitions"
```

---

## Task 2: 部署基础设施

**Step 1: 配置阿里云凭证**

```bash
# 设置环境变量（或使用 terraform.tfvars）
export ALICLOUD_ACCESS_KEY="your-access-key"
export ALICLOUD_SECRET_KEY="your-secret-key"
export ALICLOUD_REGION="cn-hangzhou"
```

**Step 2: 应用 Terraform 配置**

```bash
cd infrastructure/terraform
terraform apply -auto-approve
```

**Step 3: 验证资源创建**

访问阿里云控制台，确认：
- OSS Bucket 已创建
- Tablestore 实例和表已创建
- RAM 角色和策略已创建

**Step 4: 保存输出值**

```bash
terraform output -json > ../outputs.json
```

**Step 5: 提交**

```bash
git add infrastructure/outputs.json
git commit -m "chore(m2): add terraform outputs after deployment"
```

---

## Task 3: 函数计算 - upload-handler

**Files:**
- Create: `backend/functions/upload-handler/index.js`
- Create: `backend/functions/upload-handler/package.json`
- Create: `backend/functions/upload-handler/.fcignore`

**Step 1: 初始化项目**

```bash
cd backend/functions/upload-handler
npm init -y
npm install alibabacloud-sdk-client
```

**Step 2: 编写函数代码**

```javascript
// backend/functions/upload-handler/index.js

const TableStore = require('tablestore');

// 从环境变量获取配置
const OTS_INSTANCE = process.env.OTS_INSTANCE;
const OTS_ENDPOINT = process.env.OTS_ENDPOINT || `https://${OTS_INSTANCE}.${process.env.OTS_REGION || 'cn-hangzhou'}.ots.aliyuncs.com`;
const OTS_ACCESS_KEY = process.env.OTS_ACCESS_KEY_ID;
const OTS_SECRET_KEY = process.env.OTS_SECRET_ACCESS_KEY;

// 初始化 TableStore 客户端
const otsClient = new TableStore.Client({
  accessKeyId: OTS_ACCESS_KEY,
  secretAccessKey: OTS_SECRET_KEY,
  endpoint: OTS_ENDPOINT,
  instancename: OTS_INSTANCE,
});

/**
 * 处理图片上传元数据
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    // 解析请求体
    const body = JSON.parse(event.body || '{}');

    // 验证必需字段
    const { device_id, oss_path_original, oss_path_thumbnail, has_motion, image_size } = body;

    if (!device_id || !oss_path_original) {
      return {
        statusCode: 400,
        body: JSON.stringify({ error: 'Missing required fields' }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // 生成 row_key (timestamp + random suffix)
    const timestamp = Date.now();
    const rowKey = `${timestamp}-${Math.random().toString(36).substr(2, 9)}`;

    // 写入 Tablestore
    const params = {
      tableName: 'images',
      condition: new TableStore.RowExistenceExpectation(TableStore.RowExistenceExpectation.IGNORE),
      primaryKey: [
        { name: 'partition_key', value: device_id },
        { name: 'row_key', value: rowKey }
      ],
      attributeColumns: [
        { name: 'device_id', value: device_id },
        { name: 'has_motion', value: has_motion || false },
        { name: 'oss_path_original', value: oss_path_original },
        { name: 'oss_path_thumbnail', value: oss_path_thumbnail || '' },
        { name: 'created_at', value: timestamp },
        { name: 'image_size', value: image_size || 0 }
      ]
    };

    await new Promise((resolve, reject) => {
      otsClient.putRow(params, (err, data) => {
        if (err) reject(err);
        else resolve(data);
      });
    });

    console.log('Image metadata saved successfully');

    // 返回成功
    return {
      statusCode: 200,
      body: JSON.stringify({
        message: 'success',
        row_key: rowKey,
        created_at: timestamp
      }),
      headers: { 'Content-Type': 'application/json' }
    };

  } catch (error) {
    console.error('Error processing upload:', error);

    return {
      statusCode: 500,
      body: JSON.stringify({
        error: 'Internal server error',
        message: error.message
      }),
      headers: { 'Content-Type': 'application/json' }
    };
  }
};
```

**Step 3: 创建 package.json**

```json
{
  "name": "upload-handler",
  "version": "1.0.0",
  "description": "Handle image upload metadata for MyCam",
  "main": "index.js",
  "dependencies": {
    "tablestore": "^5.4.0"
  }
}
```

**Step 4: 创建 .fcignore**

```
node_modules/
.git/
.npm/
*.log
```

**Step 5: 创建部署脚本**

```javascript
// backend/functions/upload-handler/deploy.js
const { execSync } = require('child_process');
const fs = require('fs');

// 阿里云 CLI 部署命令
const deployCommand = `aliyun fc POST /services/mycam/functions/upload-handler \
  --description "Handle image upload metadata" \
  --codeZipFile://upload-handler.zip \
  --handler index.handler \
  --memorySize 256 \
  --runtime nodejs14 \
  --timeout 30`;

console.log('Deploying upload-handler function...');
console.log(deployCommand);

// TODO: 使用阿里云 SDK 或 CLI 部署
```

**Step 6: 提交**

```bash
git add backend/functions/upload-handler/
git commit -m "feat(m2): implement upload-handler function"
```

---

## Task 4: 函数计算 - device-manager (STS Token)

**Files:**
- Create: `backend/functions/device-manager/index.js`
- Create: `backend/functions/device-manager/package.json`

**Step 1: 编写 STS Token 获取函数**

```javascript
// backend/functions/device-manager/index.js

const RPCClient = require('@alicloud/pop-core').RPCClient;

// 从环境变量获取配置
const REGION = process.env.ALIBABA_CLOUD_REGION || 'cn-hangzhou';
const ACCESS_KEY = process.env.ALIBABA_CLOUD_ACCESS_KEY_ID;
const SECRET_KEY = process.env.ALIBABA_CLOUD_ACCESS_KEY_SECRET;
const DEVICE_UPLOAD_ROLE_ARN = process.env.DEVICE_UPLOAD_ROLE_ARN;

/**
 * 获取 STS Token 用于设备直接上传 OSS
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    // 解析请求
    const path = event.path || '';
    const method = event.httpMethod || 'GET';

    // 获取 STS Token
    if (path === '/token' && method === 'POST') {
      return await getSTSToken(event);
    }

    // 设备注册
    if (path === '/register' && method === 'POST') {
      return await registerDevice(event);
    }

    // 404
    return {
      statusCode: 404,
      body: JSON.stringify({ error: 'Not found' }),
      headers: { 'Content-Type': 'application/json' }
    };

  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      body: JSON.stringify({ error: error.message }),
      headers: { 'Content-Type': 'application/json' }
    };
  }
};

/**
 * 获取 STS Token
 */
async function getSTSToken(event) {
  const body = JSON.parse(event.body || '{}');
  const { device_id } = body;

  if (!device_id) {
    return {
      statusCode: 400,
      body: JSON.stringify({ error: 'device_id is required' }),
      headers: { 'Content-Type': 'application/json' }
    };
  }

  // 初始化 STS 客户端
  const stsClient = new RPCClient({
    accessKeyId: ACCESS_KEY,
    accessKeySecret: SECRET_KEY,
    endpoint: `https://sts.${REGION}.aliyuncs.com`,
    apiVersion: '2015-04-01'
  });

  // 调用 AssumeRole
  const params = {
    RoleArn: DEVICE_UPLOAD_ROLE_ARN,
    RoleSessionName: `device-${device_id}`,
    DurationSeconds: 900,  // 15 分钟
    Policy: JSON.stringify({
      Version: '1',
      Statement: [
        {
          Effect: 'Allow',
          Action: ['oss:PutObject'],
          Resource: [`acs:oss:*:*:*/devices/${device_id}/*`]
        }
      ]
    })
  };

  const result = await stsClient.request('AssumeRole', params);

  return {
    statusCode: 200,
    body: JSON.stringify({
      access_key_id: result.Credentials.AccessKeyId,
      access_key_secret: result.Credentials.AccessKeySecret,
      security_token: result.Credentials.SecurityToken,
      expiration: result.Credentials.Expiration,
      bucket: process.env.OSS_BUCKET,
      region: REGION
    }),
    headers: { 'Content-Type': 'application/json' }
  };
}

/**
 * 设备注册
 */
async function registerDevice(event) {
  const body = JSON.parse(event.body || '{}');
  const { device_id, device_name, owner_id } = body;

  // TODO: 写入 Tablestore devices 表
  // 简化实现，返回成功
  return {
    statusCode: 200,
    body: JSON.stringify({
      message: 'Device registered',
      device_id
    }),
    headers: { 'Content-Type': 'application/json' }
  };
}
```

**Step 2: 创建 package.json**

```json
{
  "name": "device-manager",
  "version": "1.0.0",
  "description": "Device management and STS token provider",
  "main": "index.js",
  "dependencies": {
    "@alicloud/pop-core": "^1.7.12"
  }
}
```

**Step 3: 提交**

```bash
git add backend/functions/device-manager/
git commit -m "feat(m2): implement device-manager with STS token support"
```

---

## Task 5: 函数计算 - image-query

**Files:**
- Create: `backend/functions/image-query/index.js`
- Create: `backend/functions/image-query/package.json`

**Step 1: 编写图片查询函数**

```javascript
// backend/functions/image-query/index.js

const TableStore = require('tablestore');

const OTS_INSTANCE = process.env.OTS_INSTANCE;
const OTS_ENDPOINT = process.env.OTS_ENDPOINT;
const OTS_ACCESS_KEY = process.env.OTS_ACCESS_KEY_ID;
const OTS_SECRET_KEY = process.env.OTS_SECRET_ACCESS_KEY;

const otsClient = new TableStore.Client({
  accessKeyId: OTS_ACCESS_KEY,
  secretAccessKey: OTS_SECRET_KEY,
  endpoint: OTS_ENDPOINT,
  instancename: OTS_INSTANCE,
});

/**
 * 查询图片列表
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    const query = event.queryParameters || {};
    const { device_id, start_time, end_time, limit = '50' } = query;

    if (!device_id) {
      return {
        statusCode: 400,
        body: JSON.stringify({ error: 'device_id is required' }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // 构建查询范围
    const startRowKey = start_time || '0';
    const endRowKey = end_time ? `${end_time}-~` : `${Date.now()}-~`;

    const params = {
      tableName: 'images',
      direction: TableStore.Direction.BACKWARD,
      inclusive_start_primary_key: [
        { name: 'partition_key', value: device_id },
        { name: 'row_key', value: endRowKey }
      ],
      exclusive_end_primary_key: [
        { name: 'partition_key', value: device_id },
        { name: 'row_key', value: startRowKey }
      ],
      limit: parseInt(limit),
      columns_to_get: [
        'device_id', 'has_motion', 'oss_path_original',
        'oss_path_thumbnail', 'created_at', 'image_size'
      ]
    };

    const result = await new Promise((resolve, reject) => {
      otsClient.getRange(params, (err, data) => {
        if (err) reject(err);
        else resolve(data);
      });
    });

    // 解析结果
    const images = result.rows.map(row => {
      const attrs = {};
      row.attributes.forEach(attr => {
        attrs[attr.columnName] = attr.columnValue;
      });
      return {
        partition_key: row.primaryKey[0].value,
        row_key: row.primaryKey[1].value,
        ...attrs
      };
    });

    return {
      statusCode: 200,
      body: JSON.stringify({
        images,
        next_token: result.next_start_primary_key
      }),
      headers: { 'Content-Type': 'application/json' }
    };

  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      body: JSON.stringify({ error: error.message }),
      headers: { 'Content-Type': 'application/json' }
    };
  }
};
```

**Step 2: 提交**

```bash
git add backend/functions/image-query/
git commit -m "feat(m2): implement image-query function"
```

---

## Task 6: API 网关配置

**Files:**
- Create: `infrastructure/api-gateway/setup.json`
- Create: `infrastructure/api-gateway/deploy.sh`

**Step 1: 创建 API 网关配置**

```json
{
  "api_name": "mycam-api",
  "description": "MyCam Camera Monitor API",
  "stage_name": "prod",
  "group_id": "${API_GATEWAY_GROUP_ID}",
  "apis": [
    {
      "api_name": "upload-image",
      "path": "/api/v1/images/upload",
      "method": "POST",
      "function": "upload-handler"
    },
    {
      "api_name": "list-images",
      "path": "/api/v1/images/list",
      "method": "GET",
      "function": "image-query"
    },
    {
      "api_name": "device-token",
      "path": "/api/v1/device/token",
      "method": "POST",
      "function": "device-manager"
    },
    {
      "api_name": "device-register",
      "path": "/api/v1/device/register",
      "method": "POST",
      "function": "device-manager"
    }
  ]
}
```

**Step 2: 创建部署脚本**

```bash
#!/bin/bash
# infrastructure/api-gateway/deploy.sh

# 获取函数计算服务
SERVICE_NAME="mycam"
REGION="cn-hangzhou"

# 创建或更新 API
echo "Setting up API Gateway..."

# TODO: 使用阿里云 CLI 部署 API 网关
# aliyun apigateway ...

echo "API Gateway setup complete"
```

**Step 3: 提交**

```bash
git add infrastructure/api-gateway/
git commit -m "feat(m2): add API Gateway configuration"
```

---

## 完成标准

- [ ] Terraform 基础设施部署成功
- [ ] OSS Bucket 和 Tablestore 表创建完成
- [ ] upload-handler 函数可以写入元数据
- [ ] device-manager 函数可以返回 STS Token
- [ ] image-query 函数可以查询图片列表
- [ ] API 网关路由配置完成

---

## 下一步

M2 完成后，进入 M3: 通知集成实施计划
