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
