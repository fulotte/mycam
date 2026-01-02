// frontend/src/api/index.js
import axios from 'axios';
import { useSettingsStore } from '@/stores/settings';

const apiClient = axios.create({
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
});

// 请求拦截器
apiClient.interceptors.request.use(
  (config) => {
    const settings = useSettingsStore();
    if (settings.apiBaseUrl) {
      config.baseURL = settings.apiBaseUrl;
    }
    return config;
  },
  (error) => {
    return Promise.reject(error);
  }
);

// 响应拦截器
apiClient.interceptors.response.use(
  (response) => response.data,
  (error) => {
    console.error('API Error:', error);
    return Promise.reject(error);
  }
);

// API 方法
export const api = {
  // 获取图片列表
  getImages: (params) => apiClient.get('/api/v1/images/list', { params }),

  // 获取 STS Token
  getSTSToken: (deviceId) => apiClient.post('/api/v1/device/token', { device_id: deviceId }),

  // 上报图片元数据
  uploadImageMeta: (data) => apiClient.post('/api/v1/images/upload', data),

  // 获取设备配置
  getDeviceConfig: (deviceId) => apiClient.get(`/api/v1/device/${deviceId}/config`),

  // 更新设备配置
  updateDeviceConfig: (deviceId, data) => apiClient.post(`/api/v1/device/${deviceId}/config`, data),

  // 部分更新设备配置
  patchDeviceConfig: (deviceId, data) => apiClient.patch(`/api/v1/device/${deviceId}/config`, data)
};

export default apiClient;
