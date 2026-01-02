<!-- frontend/src/views/ImageList.vue -->
<template>
  <div class="image-list">
    <el-page-header @back="goBack" content="历史照片" />

    <div class="filters">
      <el-select v-model="selectedDevice" placeholder="选择设备" @change="loadImages">
        <el-option label="全部设备" :value="null" />
        <el-option
          v-for="device in devices"
          :key="device.id"
          :label="device.name"
          :value="device.id"
        />
      </el-select>

      <el-select v-model="motionOnly" @change="loadImages">
        <el-option label="全部照片" :value="false" />
        <el-option label="仅运动照片" :value="true" />
      </el-select>

      <el-date-picker
        v-model="dateRange"
        type="daterange"
        range-separator="至"
        start-placeholder="开始日期"
        end-placeholder="结束日期"
        @change="loadImages"
      />
    </div>

    <el-divider />

    <div v-loading="loading" class="images-grid">
      <ImageCard
        v-for="image in images"
        :key="image.row_key"
        :image="image"
        @click="viewImage"
      />
    </div>

    <el-empty v-if="!loading && images.length === 0" description="暂无照片" />

    <div class="pagination" v-if="hasMore">
      <el-button @click="loadMore" :loading="loadingMore">
        加载更多
      </el-button>
    </div>

    <!-- 图片预览对话框 -->
    <el-dialog v-model="showPreview" :width="'90%'" :style="{ maxWidth: '800px' }">
      <div class="preview-dialog">
        <img :src="previewUrl" alt="预览" />
        <div class="preview-actions">
          <el-button @click="downloadImage">
            <el-icon><Download /></el-icon>
            下载
          </el-button>
          <el-button @click="deleteImage" type="danger">
            <el-icon><Delete /></el-icon>
            删除
          </el-button>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, onMounted, computed } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useDevicesStore } from '@/stores/devices';
import { api } from '@/api';
import ImageCard from '@/components/ImageCard.vue';
import { ElMessage, ElMessageBox } from 'element-plus';

const route = useRoute();
const router = useRouter();
const devicesStore = useDevicesStore();
const { devices } = devicesStore;

const loading = ref(false);
const loadingMore = ref(false);
const images = ref([]);
const hasMore = ref(false);
const selectedDevice = ref(route.query.device || null);
const motionOnly = ref(false);
const dateRange = ref(null);
const showPreview = ref(false);
const currentImage = ref(null);

const previewUrl = computed(() => {
  return currentImage.value?.oss_path_original || '';
});

function goBack() {
  router.back();
}

async function loadImages() {
  loading.value = true;
  try {
    const params = {
      device_id: selectedDevice.value,
      limit: 50
    };

    if (dateRange.value) {
      params.start_time = dateRange.value[0].getTime();
      params.end_time = dateRange.value[1].getTime();
    }

    const result = await api.getImages(params);
    images.value = result.images || [];
    hasMore.value = !!result.next_token;
  } catch (error) {
    ElMessage.error('加载图片失败');
  } finally {
    loading.value = false;
  }
}

async function loadMore() {
  loadingMore.value = true;
  try {
    // TODO: 实现分页加载
    await loadImages();
  } finally {
    loadingMore.value = false;
  }
}

function viewImage(image) {
  currentImage.value = image;
  showPreview.value = true;
}

function downloadImage() {
  const link = document.createElement('a');
  link.href = previewUrl.value;
  link.download = `mycam-${currentImage.value.row_key}.jpg`;
  link.click();
}

async function deleteImage() {
  try {
    await ElMessageBox.confirm('确定删除这张照片吗？', '提示', {
      type: 'warning'
    });

    // TODO: 调用删除 API
    ElMessage.success('删除成功');
    showPreview.value = false;
    loadImages();
  } catch {
    // 用户取消
  }
}

onMounted(() => {
  loadImages();
});
</script>

<style scoped>
.image-list {
  padding: 20px;
  max-width: 1400px;
  margin: 0 auto;
}

.filters {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin-top: 20px;
}

.images-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 16px;
  margin-top: 20px;
}

.pagination {
  text-align: center;
  margin-top: 30px;
}

.preview-dialog {
  text-align: center;
}

.preview-dialog img {
  max-width: 100%;
  max-height: 70vh;
  border-radius: 8px;
}

.preview-actions {
  display: flex;
  justify-content: center;
  gap: 12px;
  margin-top: 20px;
}
</style>
