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
