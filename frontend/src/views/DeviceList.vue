<!-- frontend/src/views/DeviceList.vue -->
<template>
  <div class="device-list">
    <el-container>
      <el-header>
        <h1>MyCam 监控系统</h1>
        <el-button type="primary" @click="showAddDialog = true">
          <el-icon><Plus /></el-icon>
          添加设备
        </el-button>
      </el-header>

      <el-main>
        <el-row :gutter="20" v-if="devices.length > 0">
          <el-col v-for="device in devices" :key="device.id" :xs="24" :sm="12" :md="8" :lg="6">
            <el-card class="device-card" shadow="hover">
              <template #header>
                <div class="card-header">
                  <span>{{ device.name }}</span>
                  <el-tag :type="device.online ? 'success' : 'danger'" size="small">
                    {{ device.online ? '在线' : '离线' }}
                  </el-tag>
                </div>
              </template>

              <div class="device-info">
                <p><el-icon><Location /></el-icon> {{ device.location || '未设置位置' }}</p>
                <p><el-icon><Clock /></el-icon> 最后在线: {{ formatTime(device.lastOnline) }}</p>
              </div>

              <div class="device-actions">
                <el-button type="primary" size="small" @click="goToPreview(device)">
                  <el-icon><VideoCamera /></el-icon>
                  预览
                </el-button>
                <el-button size="small" @click="goToImages(device)">
                  <el-icon><Picture /></el-icon>
                  照片
                </el-button>
                <el-button size="small" @click="editDevice(device)">
                  <el-icon><Setting /></el-icon>
                </el-button>
              </div>
            </el-card>
          </el-col>
        </el-row>

        <el-empty v-else description="暂无设备，请添加设备" />

        <!-- 添加设备对话框 -->
        <el-dialog v-model="showAddDialog" title="添加设备" width="90%" :style="{ maxWidth: '500px' }">
          <el-form :model="newDevice" label-width="80px">
            <el-form-item label="设备 ID">
              <el-input v-model="newDevice.id" placeholder="输入设备 ID" />
            </el-form-item>
            <el-form-item label="设备名称">
              <el-input v-model="newDevice.name" placeholder="输入设备名称" />
            </el-form-item>
            <el-form-item label="设备位置">
              <el-input v-model="newDevice.location" placeholder="输入设备位置" />
            </el-form-item>
          </el-form>
          <template #footer>
            <el-button @click="showAddDialog = false">取消</el-button>
            <el-button type="primary" @click="handleAddDevice">确定</el-button>
          </template>
        </el-dialog>
      </el-main>
    </el-container>
  </div>
</template>

<script setup>
import { ref } from 'vue';
import { useRouter } from 'vue-router';
import { useDevicesStore } from '@/stores/devices';
import { ElMessage } from 'element-plus';

const router = useRouter();
const devicesStore = useDevicesStore();
const { devices } = devicesStore;

const showAddDialog = ref(false);
const newDevice = ref({
  id: '',
  name: '',
  location: ''
});

function formatTime(timestamp) {
  if (!timestamp) return '未知';
  const date = new Date(timestamp);
  return date.toLocaleString('zh-CN');
}

function goToPreview(device) {
  router.push(`/preview/${device.id}`);
}

function goToImages(device) {
  router.push(`/images?device=${device.id}`);
}

function editDevice(device) {
  router.push(`/settings?device=${device.id}`);
}

function handleAddDevice() {
  if (!newDevice.value.id || !newDevice.value.name) {
    ElMessage.warning('请填写设备 ID 和名称');
    return;
  }

  devicesStore.addDevice({
    ...newDevice.value,
    online: false,
    lastOnline: null
  });

  showAddDialog.value = false;
  newDevice.value = { id: '', name: '', location: '' };

  ElMessage.success('设备添加成功');
}
</script>

<style scoped>
.device-list {
  min-height: 100vh;
}

.el-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: #fff;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
  padding: 0 20px;
}

.el-header h1 {
  font-size: 1.5rem;
  margin: 0;
}

.el-main {
  padding: 20px;
}

.device-card {
  margin-bottom: 20px;
  transition: transform 0.2s;
}

.device-card:hover {
  transform: translateY(-2px);
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.device-info p {
  margin: 8px 0;
  display: flex;
  align-items: center;
  gap: 8px;
  color: #666;
}

.device-actions {
  display: flex;
  gap: 8px;
  margin-top: 16px;
}

.device-actions .el-button {
  flex: 1;
}
</style>
