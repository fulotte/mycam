<!-- frontend/src/components/ImageCard.vue -->
<template>
  <div class="image-card" @click="$emit('click', image)">
    <div class="image-wrapper">
      <img :src="thumbnailUrl" :alt="formatTime(image.created_at)" loading="lazy" />
      <el-tag v-if="image.has_motion" type="danger" size="small" class="motion-tag">
        运动
      </el-tag>
    </div>
    <div class="image-info">
      <span class="time">{{ formatTime(image.created_at) }}</span>
      <span class="size">{{ formatSize(image.image_size) }}</span>
    </div>
  </div>
</template>

<script setup>
import { computed } from 'vue';

const props = defineProps({
  image: {
    type: Object,
    required: true
  }
});

defineEmits(['click']);

const thumbnailUrl = computed(() => {
  // 使用 OSS 签名 URL
  return props.image.oss_path_thumbnail || props.image.oss_path_original;
});

function formatTime(timestamp) {
  if (!timestamp) return '';
  const date = new Date(timestamp);
  const now = new Date();
  const diff = now - date;

  if (diff < 60000) return '刚刚';
  if (diff < 3600000) return `${Math.floor(diff / 60000)} 分钟前`;
  if (diff < 86400000) return `${Math.floor(diff / 3600000)} 小时前`;

  return date.toLocaleDateString('zh-CN');
}

function formatSize(bytes) {
  if (!bytes) return '-';
  if (bytes < 1024) return bytes + ' B';
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
  return (bytes / 1024 / 1024).toFixed(1) + ' MB';
}
</script>

<style scoped>
.image-card {
  cursor: pointer;
  transition: transform 0.2s;
}

.image-card:hover {
  transform: scale(1.02);
}

.image-wrapper {
  position: relative;
  border-radius: 8px;
  overflow: hidden;
  aspect-ratio: 4/3;
  background: #f0f0f0;
}

.image-wrapper img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.motion-tag {
  position: absolute;
  top: 8px;
  right: 8px;
}

.image-info {
  display: flex;
  justify-content: space-between;
  padding: 8px 0;
  font-size: 12px;
  color: #666;
}
</style>
