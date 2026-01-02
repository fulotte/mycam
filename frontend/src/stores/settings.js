// frontend/src/stores/settings.js
import { defineStore } from 'pinia';
import { ref } from 'vue';

export const useSettingsStore = defineStore('settings', () => {
  const apiBaseUrl = ref(localStorage.getItem('apiBaseUrl') || '');
  const isLocalNetwork = ref(false);

  function setApiBaseUrl(url) {
    apiBaseUrl.value = url;
    localStorage.setItem('apiBaseUrl', url);
  }

  function setIsLocalNetwork(value) {
    isLocalNetwork.value = value;
  }

  return {
    apiBaseUrl,
    isLocalNetwork,
    setApiBaseUrl,
    setIsLocalNetwork
  };
});
