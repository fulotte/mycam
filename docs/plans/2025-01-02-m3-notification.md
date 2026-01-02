# M3: 通知集成实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**目标:** 实现飞书和钉钉通知推送功能。当检测到运动时，通过云函数调用飞书/钉钉 Webhook 发送消息卡片。

**架构:** upload-handler 函数在图片元数据写入成功后，异步触发 notify-sender 函数。notify-sender 查询设备通知配置，生成消息卡片并调用飞书/钉钉 API。

**技术栈:** 飞书开放平台 API, 钉钉开放平台 API, 阿里云函数计算

---

## Task 1: notify-sender 函数基础

**Files:**
- Create: `backend/functions/notify-sender/index.js`
- Create: `backend/functions/notify-sender/package.json`

**Step 1: 编写通知发送函数**

```javascript
// backend/functions/notify-sender/index.js

const https = require('https');
const URL = require('url');

const OTS_INSTANCE = process.env.OTS_INSTANCE;
const OTS_ENDPOINT = process.env.OTS_ENDPOINT;
const OTS_ACCESS_KEY = process.env.OTS_ACCESS_KEY_ID;
const OTS_SECRET_KEY = process.env.OTS_SECRET_ACCESS_KEY;
const OSS_BUCKET = process.env.OSS_BUCKET;
const OSS_REGION = process.env.OSS_REGION || 'cn-hangzhou';
const OSS_ENDPOINT = `https://oss-${OSS_REGION}.aliyuncs.com`;

const TableStore = require('tablestore');
const otsClient = new TableStore.Client({
  accessKeyId: OTS_ACCESS_KEY,
  secretAccessKey: OTS_SECRET_KEY,
  endpoint: OTS_ENDPOINT,
  instancename: OTS_INSTANCE,
});

/**
 * 发送通知到飞书/钉钉
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    // 从事件中获取图片信息
    // 事件由 upload-handler 触发
    const imageInfo = typeof event === 'string' ? JSON.parse(event) : event;
    const { device_id, row_key, has_motion, oss_path_thumbnail, created_at } = imageInfo;

    if (!device_id) {
      throw new Error('Missing device_id in event');
    }

    // 查询设备通知配置
    const deviceConfig = await getDeviceConfig(device_id);

    if (!deviceConfig) {
      console.log(`Device ${device_id} not found, skipping notification`);
      return { status: 'skipped', reason: 'device_not_found' };
    }

    const results = [];

    // 发送飞书通知
    if (deviceConfig.notify_feishu && deviceConfig.feishu_webhook) {
      const feishuResult = await sendFeishuNotification({
        webhook: deviceConfig.feishu_webhook,
        deviceName: deviceConfig.device_name || device_id,
        thumbnailUrl: `${OSS_ENDPOINT}/${OSS_BUCKET}/${oss_path_thumbnail}`,
        timestamp: created_at || Date.now(),
        hasMotion: has_motion
      });
      results.push({ platform: 'feishu', result: feishuResult });
    }

    // 发送钉钉通知
    if (deviceConfig.notify_dingtalk && deviceConfig.dingtalk_webhook) {
      const dingtalkResult = await sendDingtalkNotification({
        webhook: deviceConfig.dingtalk_webhook,
        deviceName: deviceConfig.device_name || device_id,
        thumbnailUrl: `${OSS_ENDPOINT}/${OSS_BUCKET}/${oss_path_thumbnail}`,
        timestamp: created_at || Date.now(),
        hasMotion: has_motion
      });
      results.push({ platform: 'dingtalk', result: dingtalkResult });
    }

    return {
      status: 'success',
      results
    };

  } catch (error) {
    console.error('Error sending notification:', error);
    return {
      status: 'error',
      error: error.message
    };
  }
};

/**
 * 查询设备配置
 */
async function getDeviceConfig(deviceId) {
  const params = {
    tableName: 'devices',
    primaryKey: [{ name: 'device_id', value: deviceId }],
    columnsToGet: [
      'device_name',
      'notify_feishu',
      'notify_dingtalk',
      'feishu_webhook',
      'dingtalk_webhook'
    ]
  };

  return new Promise((resolve, reject) => {
    otsClient.getRow(params, (err, data) => {
      if (err) {
        reject(err);
      } else if (!data.row) {
        resolve(null);
      } else {
        const config = {};
        data.row.attributes.forEach(attr => {
          config[attr.columnName] = attr.columnValue;
        });
        resolve(config);
      }
    });
  });
}

/**
 * 发送飞书通知
 */
async function sendFeishuNotification({ webhook, deviceName, thumbnailUrl, timestamp, hasMotion }) {
  const card = {
    msg_type: 'interactive',
    card: {
      header: {
        title: {
          tag: 'plain_text',
          content: '📸 监控摄像头'
        },
        template: hasMotion ? 'red' : 'blue'
      },
      elements: [
        {
          tag: 'img',
          img_key: thumbnailUrl,
          alt: {
            tag: 'plain_text',
            content: '监控图片'
          }
        },
        {
          tag: 'div',
          fields: [
            {
              is_short: true,
              text: {
                tag: 'lark_md',
                content: `**设备**：${deviceName}`
              }
            },
            {
              is_short: true,
              text: {
                tag: 'lark_md',
                content: `**状态**：${hasMotion ? '检测到运动' : '定时拍照'}`
              }
            },
            {
              is_short: true,
              text: {
                tag: 'lark_md',
                content: `**时间**：${new Date(timestamp).toLocaleString('zh-CN')}`
              }
            }
          ]
        },
        {
          tag: 'action',
          actions: [
            {
              tag: 'button',
              text: {
                tag: 'plain_text',
                content: '查看详情'
              },
              type: 'default',
              url: `https://your-frontend-domain.com/images?device=${deviceName}&time=${timestamp}`
            }
          ]
        }
      ]
    }
  };

  return sendWebhook(webhook, card);
}

/**
 * 发送钉钉通知
 */
async function sendDingtalkNotification({ webhook, deviceName, thumbnailUrl, timestamp, hasMotion }) {
  const card = {
    msgtype: 'actionCard',
    actionCard: {
      title: '📸 监控摄像头',
      text: `
### ${hasMotion ? '⚠️ 检测到运动' : '📷 定时拍照'}

**设备**：${deviceName}
**时间**：${new Date(timestamp).toLocaleString('zh-CN')}

![监控图片](${thumbnailUrl})
      `,
      btnOrientation: '0',
      btns: [
        {
          title: '查看详情',
          actionURL: `https://your-frontend-domain.com/images?device=${deviceName}&time=${timestamp}`
        }
      ]
    }
  };

  return sendWebhook(webhook, card);
}

/**
 * 通用 Webhook 发送
 */
function sendWebhook(webhookUrl, data) {
  return new Promise((resolve, reject) => {
    const url = URL.parse(webhookUrl);
    const postData = JSON.stringify(data);

    const options = {
      hostname: url.hostname,
      port: 443,
      path: url.path,
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(postData)
      }
    };

    const req = https.request(options, (res) => {
      let responseData = '';

      res.on('data', (chunk) => {
        responseData += chunk;
      });

      res.on('end', () => {
        if (res.statusCode === 200) {
          const result = JSON.parse(responseData);
          if (result.errcode === 0) {
            resolve({ success: true });
          } else {
            reject(new Error(`API error: ${result.errmsg}`));
          }
        } else {
          reject(new Error(`HTTP ${res.statusCode}: ${responseData}`));
        }
      });
    });

    req.on('error', (error) => {
      reject(error);
    });

    req.write(postData);
    req.end();
  });
}
```

**Step 2: 创建 package.json**

```json
{
  "name": "notify-sender",
  "version": "1.0.0",
  "description": "Send notifications to Feishu and Dingtalk",
  "main": "index.js",
  "dependencies": {
    "tablestore": "^5.4.0"
  }
}
```

**Step 3: 提交**

```bash
git add backend/functions/notify-sender/
git commit -m "feat(m3): implement notify-sender function for Feishu and Dingtalk"
```

---

## Task 2: 修改 upload-handler 触发通知

**Files:**
- Modify: `backend/functions/upload-handler/index.js`

**Step 1: 添加通知触发逻辑**

```javascript
// backend/functions/upload-handler/index.js

const https = require('https');

// 在现有代码后添加通知触发
async function triggerNotifySender(imageInfo) {
  // 通过函数计算触发器或 HTTP 调用
  // 这里使用事件总线或直接调用

  const NOTIFY_FUNCTION = process.env.NOTIFY_FUNCTION_NAME || 'notify-sender';
  const FC_ENDPOINT = process.env.FC_ENDPOINT || `https://${process.env.ACCOUNT_ID}.${process.env.FC_REGION}.fc.aliyuncs.com`;

  return new Promise((resolve, reject) => {
    const postData = JSON.stringify(imageInfo);
    const url = `${FC_ENDPOINT}/2016-08-15/proxy/${process.env.SERVICE_NAME}/${NOTIFY_FUNCTION}/`;

    https.request(url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(postData),
        'Authorization': `Bearer ${process.env.FC_AUTH_TOKEN}`
      }
    }, (res) => {
      let data = '';
      res.on('data', chunk => data += chunk);
      res.on('end', () => {
        if (res.statusCode === 200) resolve(JSON.parse(data));
        else reject(new Error(`Failed to trigger notification: ${res.statusCode}`));
      });
    }).on('error', reject).write(postData);
  });
}

// 修改主函数，在写入成功后触发通知
module.exports.handler = async (event, context) => {
  // ... 现有代码 ...

  await new Promise((resolve, reject) => {
    otsClient.putRow(params, (err, data) => {
      if (err) reject(err);
      else resolve(data);
    });
  });

  console.log('Image metadata saved successfully');

  // 触发通知（异步，不等待结果）
  const imageInfo = {
    device_id,
    row_key: rowKey,
    has_motion: has_motion || false,
    oss_path_thumbnail: oss_path_thumbnail || '',
    created_at: timestamp
  };

  // 不等待通知结果，快速返回
  triggerNotifySender(imageInfo).catch(err => {
    console.error('Failed to trigger notification:', err);
  });

  return {
    statusCode: 200,
    body: JSON.stringify({
      message: 'success',
      row_key: rowKey,
      created_at: timestamp
    }),
    headers: { 'Content-Type': 'application/json' }
  };
};
```

**Step 2: 提交**

```bash
git add backend/functions/upload-handler/
git commit -m "feat(m3): add notification trigger to upload-handler"
```

---

## Task 3: 设备配置管理接口

**Files:**
- Create: `backend/functions/device-config/index.js`
- Create: `backend/functions/device-config/package.json`

**Step 1: 编写设备配置函数**

```javascript
// backend/functions/device-config/index.js

const TableStore = require('tablestore');

const OTS_INSTANCE = process.env.OTS_INSTANCE;
const OTS_ENDPOINT = process.env.OTS_ENDPOINT;
const OTS_ACCESS_KEY = process.env.OTS_ACCESS_KEY_ID;
const OTS_SECRET_KEY = process.env.OTS_SECRET_ACCESS_KEY;

const otsClient = new TableStore.Client({
  accessKeyId: OTS_ACCESS_KEY,
  secretAccessKey: OTS_SECRET_KEY,
  endpoint: OTS_ENDPOINT,
  instancename: OTS_INSTANCE,
});

/**
 * 设备配置管理
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    const path = event.path || '';
    const method = event.httpMethod || 'GET';

    // 获取配置
    if (path === '/config' && method === 'GET') {
      return await getDeviceConfig(event);
    }

    // 更新配置
    if (path === '/config' && method === 'POST') {
      return await updateDeviceConfig(event);
    }

    // 验证 Webhook
    if (path === '/verify-webhook' && method === 'POST') {
      return await verifyWebhook(event);
    }

    return {
      statusCode: 404,
      body: JSON.stringify({ error: 'Not found' }),
      headers: { 'Content-Type': 'application/json' }
    };

  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      body: JSON.stringify({ error: error.message }),
      headers: { 'Content-Type': 'application/json' }
    };
  }
};

/**
 * 获取设备配置
 */
async function getDeviceConfig(event) {
  const query = event.queryParameters || {};
  const { device_id } = query;

  if (!device_id) {
    return {
      statusCode: 400,
      body: JSON.stringify({ error: 'device_id is required' }),
      headers: { 'Content-Type': 'application/json' }
    };
  }

  const params = {
    tableName: 'devices',
    primaryKey: [{ name: 'device_id', value: device_id }]
  };

  const result = await new Promise((resolve, reject) => {
    otsClient.getRow(params, (err, data) => {
      if (err) reject(err);
      else resolve(data);
    });
  });

  if (!result.row) {
    return {
      statusCode: 404,
      body: JSON.stringify({ error: 'Device not found' }),
      headers: { 'Content-Type': 'application/json' }
    };
  }

  const config = { device_id };
  result.row.attributes.forEach(attr => {
    config[attr.columnName] = attr.columnValue;
  });

  // 隐藏 webhook 完整 URL
  if (config.feishu_webhook) {
    config.feishu_webhook = maskWebhook(config.feishu_webhook);
  }
  if (config.dingtalk_webhook) {
    config.dingtalk_webhook = maskWebhook(config.dingtalk_webhook);
  }

  return {
    statusCode: 200,
    body: JSON.stringify(config),
    headers: { 'Content-Type': 'application/json' }
  };
}

/**
 * 更新设备配置
 */
async function updateDeviceConfig(event) {
  const body = JSON.parse(event.body || '{}');
  const { device_id, notify_feishu, notify_dingtalk, feishu_webhook, dingtalk_webhook, device_name } = body;

  if (!device_id) {
    return {
      statusCode: 400,
      body: JSON.stringify({ error: 'device_id is required' }),
      headers: { 'Content-Type': 'application/json' }
    };
  }

  // 构建更新参数
  const updateOf = {
    attr_columns: []
  };

  if (notify_feishu !== undefined) {
    updateOf.attr_columns.push({ name: 'notify_feishu', value: notify_feishu });
  }
  if (notify_dingtalk !== undefined) {
    updateOf.attr_columns.push({ name: 'notify_dingtalk', value: notify_dingtalk });
  }
  if (feishu_webhook !== undefined) {
    updateOf.attr_columns.push({ name: 'feishu_webhook', value: feishu_webhook });
  }
  if (dingtalk_webhook !== undefined) {
    updateOf.attr_columns.push({ name: 'dingtalk_webhook', value: dingtalk_webhook });
  }
  if (device_name !== undefined) {
    updateOf.attr_columns.push({ name: 'device_name', value: device_name });
  }

  const params = {
    tableName: 'devices',
    primaryKey: [{ name: 'device_id', value: device_id }],
    updateOf: updateOf
  };

  await new Promise((resolve, reject) => {
    otsClient.updateRow(params, (err, data) => {
      if (err) reject(err);
      else resolve(data);
    });
  });

  return {
    statusCode: 200,
    body: JSON.stringify({ message: 'Config updated' }),
    headers: { 'Content-Type': 'application/json' }
  };
}

/**
 * 验证 Webhook
 */
async function verifyWebhook(event) {
  const body = JSON.parse(event.body || '{}');
  const { platform, webhook } = body;

  if (!platform || !webhook) {
    return {
      statusCode: 400,
      body: JSON.stringify({ error: 'platform and webhook are required' }),
      headers: { 'Content-Type': 'application/json' }
    };
  }

  // 发送测试消息
  const testData = platform === 'feishu'
    ? {
        msg_type: 'text',
        content: { text: 'MyCam 配置测试成功！' }
      }
    : {
        msgtype: 'text',
        text: { content: 'MyCam 配置测试成功！' }
      };

  try {
    await sendWebhook(webhook, testData);

    return {
      statusCode: 200,
      body: JSON.stringify({ message: 'Webhook verified successfully' }),
      headers: { 'Content-Type': 'application/json' }
    };
  } catch (error) {
    return {
      statusCode: 400,
      body: JSON.stringify({ error: 'Webhook verification failed: ' + error.message }),
      headers: { 'Content-Type': 'application/json' }
    };
  }
}

function maskWebhook(webhook) {
  if (!webhook || webhook.length < 20) return webhook;
  return webhook.substring(0, 15) + '***' + webhook.substring(webhook.length - 5);
}

// 复用 sendWebhook 函数（从 notify-sender 复制或共享）
```

**Step 2: 创建 package.json**

```json
{
  "name": "device-config",
  "version": "1.0.0",
  "description": "Device configuration management",
  "main": "index.js",
  "dependencies": {
    "tablestore": "^5.4.0"
  }
}
```

**Step 3: 提交**

```bash
git add backend/functions/device-config/
git commit -m "feat(m3): add device configuration management function"
```

---

## Task 4: 测试通知功能

**Files:**
- Create: `backend/test/test-notification.js`

**Step 1: 创建测试脚本**

```javascript
// backend/test/test-notification.js

const https = require('https');

/**
 * 测试飞书 Webhook
 */
async function testFeishuWebhook(webhookUrl) {
  const data = {
    msg_type: 'interactive',
    card: {
      header: {
        title: {
          tag: 'plain_text',
          content: '📸 监控摄像头 - 测试'
        },
        template: 'blue'
      },
      elements: [
        {
          tag: 'div',
          text: {
            tag: 'plain_text',
            content: '这是一条测试消息，如果您看到这条消息，说明飞书通知配置成功！'
          }
        },
        {
          tag: 'action',
          actions: [
            {
              tag: 'button',
              text: {
                tag: 'plain_text',
                content: '确认收到'
              },
              type: 'default'
            }
          ]
        }
      ]
    }
  };

  await sendWebhook(webhookUrl, data);
  console.log('Feishu webhook test completed');
}

/**
 * 测试钉钉 Webhook
 */
async function testDingtalkWebhook(webhookUrl) {
  const data = {
    msgtype: 'actionCard',
    actionCard: {
      title: '📸 监控摄像头 - 测试',
      text: '这是一条测试消息，如果您看到这条消息，说明钉钉通知配置成功！',
      btnOrientation: '0',
      btns: [
        {
          title: '确认收到',
          actionURL: 'https://example.com'
        }
      ]
    }
  };

  await sendWebhook(webhookUrl, data);
  console.log('Dingtalk webhook test completed');
}

function sendWebhook(url, data) {
  return new Promise((resolve, reject) => {
    const postData = JSON.stringify(data);
    const parsedUrl = new URL(url);

    const options = {
      hostname: parsedUrl.hostname,
      port: 443,
      path: parsedUrl.pathname + parsedUrl.search,
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(postData)
      }
    };

    const req = https.request(options, (res) => {
      let responseData = '';
      res.on('data', chunk => responseData += chunk);
      res.on('end', () => {
        console.log(`Response: ${res.statusCode} - ${responseData}`);
        if (res.statusCode === 200) {
          resolve();
        } else {
          reject(new Error(`HTTP ${res.statusCode}`));
        }
      });
    });

    req.on('error', reject);
    req.write(postData);
    req.end();
  });
}

// 命令行测试
async function main() {
  const args = process.argv.slice(2);
  const platform = args[0];
  const webhook = args[1];

  if (!platform || !webhook) {
    console.log('Usage: node test-notification.js <feishu|dingtalk> <webhook-url>');
    process.exit(1);
  }

  try {
    if (platform === 'feishu') {
      await testFeishuWebhook(webhook);
    } else if (platform === 'dingtalk') {
      await testDingtalkWebhook(webhook);
    } else {
      console.log('Invalid platform. Use "feishu" or "dingtalk"');
      process.exit(1);
    }
    console.log('Test successful!');
  } catch (error) {
    console.error('Test failed:', error.message);
    process.exit(1);
  }
}

if (require.main === module) {
  main();
}

module.exports = { testFeishuWebhook, testDingtalkWebhook };
```

**Step 2: 运行测试**

```bash
# 测试飞书
node backend/test/test-notification.js feishu "https://open.feishu.cn/open-apis/bot/v2/hook/xxx"

# 测试钉钉
node backend/test/test-notification.js dingtalk "https://oapi.dingtalk.com/robot/send?access_token=xxx"
```

**Step 3: 提交**

```bash
git add backend/test/
git commit -m "test(m3): add notification testing script"
```

---

## 完成标准

- [ ] notify-sender 函数可以发送飞书和钉钉通知
- [ ] upload-handler 触发通知流程正常
- [ ] 设备配置可以读取和更新
- [ ] Webhook 验证功能正常
- [ ] 测试脚本验证通知发送成功

---

## 下一步

M3 完成后，进入 M4: 前端基础实施计划
