// backend/functions/notify-sender/index.js

const TableStore = require('tablestore');
const axios = require('axios');

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
 * 发送飞书卡片消息
 */
async function sendFeishuNotification(webhook, deviceInfo, imageData) {
  const card = {
    msg_type: 'interactive',
    card: {
      header: {
        title: {
          tag: 'plain_text',
          content: '📸 检测到画面移动'
        },
        template: 'red'
      },
      elements: [
        {
          tag: 'div',
          text: {
            tag: 'lark_md',
            content: `**设备名称**: ${deviceInfo.device_name || '未命名'}\n**设备ID**: ${imageData.device_id}\n**时间**: ${new Date(imageData.created_at).toLocaleString('zh-CN')}\n**动作**: 检测到画面变化，已自动拍照保存到云端`
          }
        },
        {
          tag: 'action',
          actions: [
            {
              tag: 'button',
              text: {
                tag: 'plain_text',
                content: '查看图片'
              },
              type: 'primary',
              url: imageData.oss_path_original
            },
            {
              tag: 'button',
              text: {
                tag: 'plain_text',
                content: '打开监控页面'
              },
              url: `https://your-domain.com/device/${imageData.device_id}`
            }
          ]
        }
      ]
    }
  };

  await sendWebhook(webhook, card);
}

/**
 * 发送钉钉 ActionCard 消息
 */
async function sendDingtalkNotification(webhook, deviceInfo, imageData) {
  const card = {
    msgtype: 'actionCard',
    actionCard: {
      title: '📸 检测到画面移动',
      text: `### ${deviceInfo.device_name || '未命名'}\n\n` +
            `**设备ID**: ${imageData.device_id}\n` +
            `**时间**: ${new Date(imageData.created_at).toLocaleString('zh-CN')}\n` +
            `**动作**: 检测到画面变化，已自动拍照保存到云端`,
      btnOrientation: '1',
      btns: [
        {
          title: '查看图片',
          actionURL: imageData.oss_path_original
        },
        {
          title: '打开监控页面',
          actionURL: `https://your-domain.com/device/${imageData.device_id}`
        }
      ]
    }
  };

  await sendWebhook(webhook, card);
}

/**
 * 发送 HTTPS POST 请求到 Webhook
 */
async function sendWebhook(webhook, data) {
  try {
    const response = await axios.post(webhook, data, {
      headers: {
        'Content-Type': 'application/json'
      },
      timeout: 5000
    });

    console.log(`Webhook sent: ${webhook}, status: ${response.status}`);
    return response.data;
  } catch (error) {
    console.error(`Webhook failed: ${webhook}`, error.message);
    throw error;
  }
}

/**
 * 从 Tablestore 查询设备配置
 */
async function getDeviceConfig(deviceId) {
  const params = {
    tableName: 'devices',
    primaryKey: [{ name: 'device_id', value: deviceId }]
  };

  const result = await new Promise((resolve, reject) => {
    otsClient.getRow(params, (err, data) => {
      if (err) reject(err);
      else resolve(data);
    });
  });

  if (!result.row) {
    return null;
  }

  const deviceInfo = { device_id: deviceId };
  result.row.attributes.forEach(attr => {
    deviceInfo[attr.columnName] = attr.columnValue;
  });

  return deviceInfo;
}

/**
 * 主处理函数
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    // 解析事件
    const eventData = typeof event === 'string' ? JSON.parse(event) : event;
    const { device_id, ...imageData } = eventData;

    if (!device_id) {
      console.error('Missing device_id in event');
      return { statusCode: 400, body: 'Missing device_id' };
    }

    // 查询设备配置
    const deviceInfo = await getDeviceConfig(device_id);

    if (!deviceInfo) {
      console.log(`Device not found: ${device_id}`);
      return { statusCode: 404, body: 'Device not found' };
    }

    // 检查通知开关
    const notifyEnabled = deviceInfo.notify_enabled !== 'false';

    if (!notifyEnabled) {
      console.log(`Notification disabled for device: ${device_id}`);
      return { statusCode: 200, body: 'Notification disabled' };
    }

    const promises = [];

    // 发送飞书通知
    if (deviceInfo.feishu_webhook) {
      promises.push(sendFeishuNotification(deviceInfo.feishu_webhook, deviceInfo, imageData));
    }

    // 发送钉钉通知
    if (deviceInfo.dingtalk_webhook) {
      promises.push(sendDingtalkNotification(deviceInfo.dingtalk_webhook, deviceInfo, imageData));
    }

    if (promises.length === 0) {
      console.log('No webhook configured for device');
      return { statusCode: 200, body: 'No webhook configured' };
    }

    // 并行发送所有通知
    await Promise.allSettled(promises);

    return {
      statusCode: 200,
      body: JSON.stringify({ message: 'Notifications sent' })
    };

  } catch (error) {
    console.error('Error:', error);
    return {
      statusCode: 500,
      body: JSON.stringify({ error: error.message })
    };
  }
};
