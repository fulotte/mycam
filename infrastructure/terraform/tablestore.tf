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
