// backend/functions/device-manager/index.js

const RPCClient = require('@alicloud/pop-core').RPCClient;

// 从环境变量获取配置
const REGION = process.env.ALIBABA_CLOUD_REGION || 'cn-hangzhou';
const ACCESS_KEY = process.env.ALIBABA_CLOUD_ACCESS_KEY_ID;
const SECRET_KEY = process.env.ALIBABA_CLOUD_ACCESS_KEY_SECRET;
const DEVICE_UPLOAD_ROLE_ARN = process.env.DEVICE_UPLOAD_ROLE_ARN;

/**
 * 获取 STS Token 用于设备直接上传 OSS
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    // 解析请求
    const path = event.path || '';
    const method = event.httpMethod || 'GET';

    // 获取 STS Token
    if (path === '/token' && method === 'POST') {
      return await getSTSToken(event);
    }

    // 设备注册
    if (path === '/register' && method === 'POST') {
      return await registerDevice(event);
    }

    // 404
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
 * 获取 STS Token
 */
async function getSTSToken(event) {
  const body = JSON.parse(event.body || '{}');
  const { device_id } = body;

  if (!device_id) {
    return {
      statusCode: 400,
      body: JSON.stringify({ error: 'device_id is required' }),
      headers: { 'Content-Type': 'application/json' }
    };
  }

  // 初始化 STS 客户端
  const stsClient = new RPCClient({
    accessKeyId: ACCESS_KEY,
    accessKeySecret: SECRET_KEY,
    endpoint: `https://sts.${REGION}.aliyuncs.com`,
    apiVersion: '2015-04-01'
  });

  // 调用 AssumeRole
  const params = {
    RoleArn: DEVICE_UPLOAD_ROLE_ARN,
    RoleSessionName: `device-${device_id}`,
    DurationSeconds: 900,  // 15 分钟
    Policy: JSON.stringify({
      Version: '1',
      Statement: [
        {
          Effect: 'Allow',
          Action: ['oss:PutObject'],
          Resource: [`acs:oss:*:*:*/devices/${device_id}/*`]
        }
      ]
    })
  };

  const result = await stsClient.request('AssumeRole', params);

  return {
    statusCode: 200,
    body: JSON.stringify({
      access_key_id: result.Credentials.AccessKeyId,
      access_key_secret: result.Credentials.AccessKeySecret,
      security_token: result.Credentials.SecurityToken,
      expiration: result.Credentials.Expiration,
      bucket: process.env.OSS_BUCKET,
      region: REGION
    }),
    headers: { 'Content-Type': 'application/json' }
  };
}

/**
 * 设备注册
 */
async function registerDevice(event) {
  const body = JSON.parse(event.body || '{}');
  const { device_id, device_name, owner_id } = body;

  // TODO: 写入 Tablestore devices 表
  // 简化实现，返回成功
  return {
    statusCode: 200,
    body: JSON.stringify({
      message: 'Device registered',
      device_id
    }),
    headers: { 'Content-Type': 'application/json' }
  };
}
