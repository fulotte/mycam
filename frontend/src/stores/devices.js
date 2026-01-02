// frontend/src/stores/devices.js
import { defineStore } from 'pinia';
import { ref } from 'vue';

export const useDevicesStore = defineStore('devices', () => {
  const devices = ref([]);

  function addDevice(device) {
    const index = devices.value.findIndex(d => d.id === device.id);
    if (index >= 0) {
      devices.value[index] = device;
    } else {
      devices.value.push(device);
    }
  }

  function removeDevice(id) {
    const index = devices.value.findIndex(d => d.id === id);
    if (index >= 0) {
      devices.value.splice(index, 1);
    }
  }

  function updateDevice(id, updates) {
    const device = devices.value.find(d => d.id === id);
    if (device) {
      Object.assign(device, updates);
    }
  }

  function getDevice(id) {
    return devices.value.find(d => d.id === id);
  }

  return {
    devices,
    addDevice,
    removeDevice,
    updateDevice,
    getDevice
  };
});
