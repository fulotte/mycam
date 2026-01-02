// backend/functions/upload-handler/index.js

const TableStore = require('tablestore');

// 从环境变量获取配置
const OTS_INSTANCE = process.env.OTS_INSTANCE;
const OTS_ENDPOINT = process.env.OTS_ENDPOINT || `https://${OTS_INSTANCE}.${process.env.OTS_REGION || 'cn-hangzhou'}.ots.aliyuncs.com`;
const OTS_ACCESS_KEY = process.env.OTS_ACCESS_KEY_ID;
const OTS_SECRET_KEY = process.env.OTS_SECRET_ACCESS_KEY;

// 初始化 TableStore 客户端
const otsClient = new TableStore.Client({
  accessKeyId: OTS_ACCESS_KEY,
  secretAccessKey: OTS_SECRET_KEY,
  endpoint: OTS_ENDPOINT,
  instancename: OTS_INSTANCE,
});

/**
 * 处理图片上传元数据
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    // 解析请求体
    const body = JSON.parse(event.body || '{}');

    // 验证必需字段
    const { device_id, oss_path_original, oss_path_thumbnail, has_motion, image_size } = body;

    if (!device_id || !oss_path_original) {
      return {
        statusCode: 400,
        body: JSON.stringify({ error: 'Missing required fields' }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // 生成 row_key (timestamp + random suffix)
    const timestamp = Date.now();
    const rowKey = `${timestamp}-${Math.random().toString(36).substr(2, 9)}`;

    // 写入 Tablestore
    const params = {
      tableName: 'images',
      condition: new TableStore.RowExistenceExpectation(TableStore.RowExistenceExpectation.IGNORE),
      primaryKey: [
        { name: 'partition_key', value: device_id },
        { name: 'row_key', value: rowKey }
      ],
      attributeColumns: [
        { name: 'device_id', value: device_id },
        { name: 'has_motion', value: has_motion || false },
        { name: 'oss_path_original', value: oss_path_original },
        { name: 'oss_path_thumbnail', value: oss_path_thumbnail || '' },
        { name: 'created_at', value: timestamp },
        { name: 'image_size', value: image_size || 0 }
      ]
    };

    await new Promise((resolve, reject) => {
      otsClient.putRow(params, (err, data) => {
        if (err) reject(err);
        else resolve(data);
      });
    });

    console.log('Image metadata saved successfully');

    // 返回成功
    return {
      statusCode: 200,
      body: JSON.stringify({
        message: 'success',
        row_key: rowKey,
        created_at: timestamp
      }),
      headers: { 'Content-Type': 'application/json' }
    };

  } catch (error) {
    console.error('Error processing upload:', error);

    return {
      statusCode: 500,
      body: JSON.stringify({
        error: 'Internal server error',
        message: error.message
      }),
      headers: { 'Content-Type': 'application/json' }
    };
  }
};
