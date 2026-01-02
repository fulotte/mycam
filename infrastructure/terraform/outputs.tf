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
