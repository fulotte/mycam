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
