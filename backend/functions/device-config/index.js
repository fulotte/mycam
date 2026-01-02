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
 * 从路径获取 device_id
 */
function getDeviceId(path) {
  const match = path.match(/\/device\/([^\/]+)/);
  return match ? match[1] : null;
}

/**
 * 获取设备配置
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
 * 创建或更新设备配置
 */
async function putDeviceConfig(deviceId, config) {
  const attributeColumns = [];

  for (const [key, value] of Object.entries(config)) {
    if (key !== 'device_id') {
      attributeColumns.push({ name: key, value: value });
    }
  }

  // 添加更新时间
  attributeColumns.push({ name: 'updated_at', value: Date.now() });

  const params = {
    tableName: 'devices',
    condition: new TableStore.RowExistenceExpectation(TableStore.RowExistenceExpectation.IGNORE),
    primaryKey: [{ name: 'device_id', value: deviceId }],
    attributeColumns
  };

  await new Promise((resolve, reject) => {
    otsClient.putRow(params, (err, data) => {
      if (err) reject(err);
      else resolve(data);
    });
  });

  return { device_id: deviceId, ...config };
}

/**
 * 更新设备配置的部分字段
 */
async function updateDeviceConfig(deviceId, updates) {
  const current = await getDeviceConfig(deviceId);

  if (!current) {
    return null;
  }

  const merged = { ...current, ...updates };
  return await putDeviceConfig(deviceId, merged);
}

/**
 * 主处理函数
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    const path = event.path || '';
    const method = event.httpMethod || event.method;
    const deviceId = getDeviceId(path);

    if (!deviceId) {
      return {
        statusCode: 400,
        body: JSON.stringify({ error: 'Invalid device_id in path' }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // GET - 获取设备配置
    if (method === 'GET') {
      const config = await getDeviceConfig(deviceId);

      if (!config) {
        return {
          statusCode: 404,
          body: JSON.stringify({ error: 'Device not found' }),
          headers: { 'Content-Type': 'application/json' }
        };
      }

      return {
        statusCode: 200,
        body: JSON.stringify(config),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // POST - 创建或替换设备配置
    if (method === 'POST') {
      const body = JSON.parse(event.body || '{}');
      const config = await putDeviceConfig(deviceId, body);

      return {
        statusCode: 200,
        body: JSON.stringify({
          message: 'Device config saved',
          config
        }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // PATCH - 部分更新设备配置
    if (method === 'PATCH') {
      const body = JSON.parse(event.body || '{}');
      const config = await updateDeviceConfig(deviceId, body);

      if (!config) {
        return {
          statusCode: 404,
          body: JSON.stringify({ error: 'Device not found' }),
          headers: { 'Content-Type': 'application/json' }
        };
      }

      return {
        statusCode: 200,
        body: JSON.stringify({
          message: 'Device config updated',
          config
        }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    return {
      statusCode: 405,
      body: JSON.stringify({ error: 'Method not allowed' }),
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
