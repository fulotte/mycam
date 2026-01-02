// frontend/src/router/index.js
import { createRouter, createWebHistory } from 'vue-router';

const routes = [
  {
    path: '/',
    redirect: '/devices'
  },
  {
    path: '/devices',
    name: 'DeviceList',
    component: () => import('@/views/DeviceList.vue'),
    meta: { title: '设备列表' }
  },
  {
    path: '/preview/:id',
    name: 'Preview',
    component: () => import('@/views/Preview.vue'),
    meta: { title: '实时预览' }
  },
  {
    path: '/images',
    name: 'ImageList',
    component: () => import('@/views/ImageList.vue'),
    meta: { title: '历史照片' }
  },
  {
    path: '/settings',
    name: 'Settings',
    component: () => import('@/views/Settings.vue'),
    meta: { title: '设置' }
  }
];

const router = createRouter({
  history: createWebHistory(),
  routes
});

router.beforeEach((to, from, next) => {
  document.title = to.meta.title ? `${to.meta.title} - MyCam` : 'MyCam';
  next();
});

export default router;
