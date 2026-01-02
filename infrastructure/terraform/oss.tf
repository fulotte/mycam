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
