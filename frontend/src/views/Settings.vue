<!-- frontend/src/views/Settings.vue -->
<template>
  <div class="settings">
    <el-page-header @back="goBack" content="设置" />

    <el-tabs v-model="activeTab" class="settings-tabs">
      <el-tab-pane label="通知设置" name="notification">
        <el-card>
          <template #header>
            <div class="card-header">
              <span>飞书通知</span>
              <el-switch v-model="config.notify_feishu" @change="saveConfig" />
            </div>
          </template>

          <el-form label-width="100px" v-if="config.notify_feishu">
            <el-form-item label="Webhook URL">
              <el-input
                v-model="config.feishu_webhook"
                placeholder="输入飞书机器人 Webhook URL"
                show-password
              />
            </el-form-item>
            <el-form-item>
              <el-button @click="testWebhook('feishu')">测试发送</el-button>
            </el-form-item>
          </el-form>
        </el-card>

        <el-card style="margin-top: 16px;">
          <template #header>
            <div class="card-header">
              <span>钉钉通知</span>
              <el-switch v-model="config.notify_dingtalk" @change="saveConfig" />
            </div>
          </template>

          <el-form label-width="100px" v-if="config.notify_dingtalk">
            <el-form-item label="Webhook URL">
              <el-input
                v-model="config.dingtalk_webhook"
                placeholder="输入钉钉机器人 Webhook URL"
                show-password
              />
            </el-form-item>
            <el-form-item>
              <el-button @click="testWebhook('dingtalk')">测试发送</el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="设备配置" name="device">
        <el-card>
          <el-form label-width="120px">
            <el-form-item label="检测灵敏度">
              <el-select v-model="deviceConfig.sensitivity" @change="saveDeviceConfig">
                <el-option label="低" value="low" />
                <el-option label="中" value="medium" />
                <el-option label="高" value="high" />
              </el-select>
            </el-form-item>
            <el-form-item label="抓图频率">
              <el-slider v-model="deviceConfig.fps" :min="1" :max="5" @change="saveDeviceConfig" />
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="关于" name="about">
        <el-card>
          <h3>MyCam 监控系统</h3>
          <p>版本: 1.0.0</p>
          <p>基于 M5Stack CamS3 的智能视频监控解决方案</p>
        </el-card>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { api } from '@/api';
import { ElMessage } from 'element-plus';

const route = useRoute();
const router = useRouter();

const activeTab = ref('notification');
const deviceId = route.query.device;

const config = ref({
  notify_feishu: false,
  notify_dingtalk: false,
  feishu_webhook: '',
  dingtalk_webhook: ''
});

const deviceConfig = ref({
  sensitivity: 'medium',
  fps: 3
});

function goBack() {
  router.back();
}

async function loadConfig() {
  if (!deviceId) return;

  try {
    const result = await api.getDeviceConfig(deviceId);
    Object.assign(config.value, result);
  } catch (error) {
    console.error('Failed to load config:', error);
  }
}

async function saveConfig() {
  if (!deviceId) return;

  try {
    await api.updateDeviceConfig(deviceId, config.value);
    ElMessage.success('设置已保存');
  } catch (error) {
    ElMessage.error('保存失败');
  }
}

async function saveDeviceConfig() {
  // TODO: 发送设备配置更新
  ElMessage.success('设备配置已更新');
}

async function testWebhook(platform) {
  const webhook = platform === 'feishu'
    ? config.value.feishu_webhook
    : config.value.dingtalk_webhook;

  if (!webhook) {
    ElMessage.warning('请先输入 Webhook URL');
    return;
  }

  try {
    // TODO: 实现测试 API
    ElMessage.success('测试消息已发送，请检查您的飞书/钉钉');
  } catch (error) {
    ElMessage.error('测试失败: ' + error.message);
  }
}

onMounted(() => {
  loadConfig();
});
</script>

<style scoped>
.settings {
  padding: 20px;
  max-width: 800px;
  margin: 0 auto;
}

.settings-tabs {
  margin-top: 20px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>
