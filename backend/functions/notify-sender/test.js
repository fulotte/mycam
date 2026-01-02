// backend/functions/notify-sender/test.js
// 本地测试脚本 - 模拟事件并测试通知功能

// 模拟函数计算环境
process.env.OTS_INSTANCE = 'mycam-ots-instance';
process.env.OTS_ENDPOINT = 'https://mycam-ots-instance.cn-hangzhou.ots.aliyuncs.com';
process.env.OTS_ACCESS_KEY_ID = 'your-access-key-id';
process.env.OTS_SECRET_ACCESS_KEY = 'your-secret-access-key';

const handler = require('./index').handler;

/**
 * 测试场景 1: 模拟运动检测事件
 */
async function testMotionEvent() {
  console.log('\n=== 测试场景 1: 运动检测事件 ===\n');

  const event = {
    device_id: 'cams3-001',
    oss_path_original: 'https://mycam-bucket.oss-cn-hangzhou.aliyuncs.com/images/cams3-001/20240102_123456_original.jpg',
    oss_path_thumbnail: 'https://mycam-bucket.oss-cn-hangzhou.aliyuncs.com/images/cams3-001/20240102_123456_thumb.jpg',
    has_motion: true,
    image_size: 102400,
    created_at: 1704192356000
  };

  try {
    const result = await handler(event, {});
    console.log('Result:', result);
  } catch (error) {
    console.error('Error:', error.message);
  }
}

/**
 * 测试场景 2: 无运动检测事件（应跳过通知）
 */
async function testNoMotionEvent() {
  console.log('\n=== 测试场景 2: 无运动检测事件 ===\n');

  const event = {
    device_id: 'cams3-001',
    oss_path_original: 'https://mycam-bucket.oss-cn-hangzhou.aliyuncs.com/images/cams3-001/20240102_123456_original.jpg',
    has_motion: false,
    created_at: 1704192356000
  };

  try {
    const result = await handler(event, {});
    console.log('Result:', result);
  } catch (error) {
    console.error('Error:', error.message);
  }
}

/**
 * 测试场景 3: 缺少 device_id（应返回错误）
 */
async function testMissingDeviceId() {
  console.log('\n=== 测试场景 3: 缺少 device_id ===\n');

  const event = {
    oss_path_original: 'https://mycam-bucket.oss-cn-hangzhou.aliyuncs.com/images/test.jpg',
    has_motion: true,
    created_at: 1704192356000
  };

  try {
    const result = await handler(event, {});
    console.log('Result:', result);
  } catch (error) {
    console.error('Error:', error.message);
  }
}

/**
 * 运行所有测试
 */
async function runTests() {
  console.log('=================================================');
  console.log('  通知功能测试脚本');
  console.log('=================================================');
  console.log('\n注意：此测试需要实际配置：');
  console.log('1. Tablestore 环境变量（OTS_*)');
  console.log('2. devices 表中存在测试设备');
  console.log('3. 设备配置了 feishu_webhook 或 dingtalk_webhook');
  console.log('\n=================================================\n');

  await testMotionEvent();
  await testNoMotionEvent();
  await testMissingDeviceId();

  console.log('\n=================================================');
  console.log('  测试完成');
  console.log('=================================================\n');
}

// 运行测试
runTests().catch(console.error);
