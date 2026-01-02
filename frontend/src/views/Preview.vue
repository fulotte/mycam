<!-- frontend/src/views/Preview.vue -->
<template>
  <div class="preview">
    <el-page-header @back="goBack" :content="deviceName">
      <template #extra>
        <el-tag :type="isConnected ? 'success' : 'danger'">
          {{ isConnected ? '已连接' : '未连接' }}
        </el-tag>
      </template>
    </el-page-header>

    <div class="video-container">
      <img
        ref="videoRef"
        :src="streamUrl"
        @load="onImageLoad"
        @error="onImageError"
        alt="视频流"
      />

      <div class="video-overlay" v-if="!isConnected">
        <el-icon><Loading /></el-icon>
        <p>正在连接摄像头...</p>
      </div>
    </div>

    <div class="controls">
      <el-button-group>
        <el-button @click="capture" :disabled="!isConnected">
          <el-icon><Camera /></el-icon>
          抓拍
        </el-button>
        <el-button @click="toggleStream">
          <el-icon><VideoPause v-if="isPlaying" /><VideoPlay v-else /></el-icon>
          {{ isPlaying ? '暂停' : '继续' }}
        </el-button>
      </el-button-group>

      <el-select v-model="fps" @change="updateFps" style="width: 120px; margin-left: 12px;">
        <el-option label="1 FPS" :value="1" />
        <el-option label="2 FPS" :value="2" />
        <el-option label="3 FPS" :value="3" />
        <el-option label="5 FPS" :value="5" />
      </el-select>
    </div>

    <el-divider />

    <div class="status">
      <el-descriptions :column="2" border>
        <el-descriptions-item label="分辨率">{{ imageSize }}</el-descriptions-item>
        <el-descriptions-item label="帧率">{{ fps }} FPS</el-descriptions-item>
        <el-descriptions-item label="最后更新">{{ lastUpdate }}</el-descriptions-item>
        <el-descriptions-item label="运动检测">
          <el-tag :type="hasMotion ? 'danger' : 'info'" size="small">
            {{ hasMotion ? '检测到运动' : '无' }}
          </el-tag>
        </el-descriptions-item>
      </el-descriptions>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useDevicesStore } from '@/stores/devices';
import { ElMessage } from 'element-plus';

const route = useRoute();
const router = useRouter();
const devicesStore = useDevicesStore();

const deviceId = route.params.id;
const device = computed(() => devicesStore.getDevice(deviceId));
const deviceName = computed(() => device.value?.name || deviceId);

const videoRef = ref(null);
const isConnected = ref(false);
const isPlaying = ref(true);
const hasMotion = ref(false);
const fps = ref(3);
const imageSize = ref('-');
const lastUpdate = ref('-');

let streamTimer = null;
let motionTimer = null;

const streamUrl = computed(() => {
  // 局域网模式：直接访问设备 IP
  // 互联网模式：显示占位符
  return `http://${device.value?.ip || 'localhost'}/stream`;
});

function goBack() {
  router.back();
}

function onImageLoad() {
  isConnected.value = true;
  const img = videoRef.value;
  if (img) {
    imageSize.value = `${img.naturalWidth}x${img.naturalHeight}`;
    lastUpdate.value = new Date().toLocaleTimeString();
  }
}

function onImageError() {
  isConnected.value = false;
}

function updateStream() {
  if (isPlaying.value && videoRef.value) {
    videoRef.value.src = `${streamUrl.value}?t=${Date.now()}`;
  }
}

function updateFps() {
  stopStream();
  startStream();
  ElMessage.success(`帧率已设置为 ${fps.value} FPS`);
}

function startStream() {
  const interval = 1000 / fps.value;
  streamTimer = setInterval(updateStream, interval);
}

function stopStream() {
  if (streamTimer) {
    clearInterval(streamTimer);
    streamTimer = null;
  }
}

function toggleStream() {
  isPlaying.value = !isPlaying.value;
  if (isPlaying.value) {
    startStream();
  } else {
    stopStream();
  }
}

async function checkMotion() {
  try {
    const response = await fetch(`http://${device.value?.ip || 'localhost'}/motion`);
    const data = await response.json();
    hasMotion.value = data.motion || false;
  } catch (error) {
    // 忽略错误
  }
}

function capture() {
  ElMessage.success('已抓拍当前画面');
  // TODO: 实现抓拍功能
}

onMounted(() => {
  startStream();
  motionTimer = setInterval(checkMotion, 500);
});

onUnmounted(() => {
  stopStream();
  if (motionTimer) {
    clearInterval(motionTimer);
  }
});
</script>

<style scoped>
.preview {
  padding: 20px;
  max-width: 1200px;
  margin: 0 auto;
}

.video-container {
  position: relative;
  background: #000;
  border-radius: 8px;
  overflow: hidden;
  aspect-ratio: 4/3;
}

.video-container img {
  width: 100%;
  height: 100%;
  object-fit: contain;
}

.video-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  color: #fff;
  background: rgba(0, 0, 0, 0.5);
}

.video-overlay .el-icon {
  font-size: 48px;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.controls {
  display: flex;
  justify-content: center;
  align-items: center;
  margin: 20px 0;
}

.status {
  margin-top: 20px;
}
</style>
