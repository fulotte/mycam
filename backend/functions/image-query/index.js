// backend/functions/image-query/index.js

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
 * 查询图片列表
 */
module.exports.handler = async (event, context) => {
  console.log('Received event:', JSON.stringify(event, null, 2));

  try {
    const query = event.queryParameters || {};
    const { device_id, start_time, end_time, limit = '50' } = query;

    if (!device_id) {
      return {
        statusCode: 400,
        body: JSON.stringify({ error: 'device_id is required' }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // 构建查询范围
    const startRowKey = start_time || '0';
    const endRowKey = end_time ? `${end_time}-~` : `${Date.now()}-~`;

    const params = {
      tableName: 'images',
      direction: TableStore.Direction.BACKWARD,
      inclusive_start_primary_key: [
        { name: 'partition_key', value: device_id },
        { name: 'row_key', value: endRowKey }
      ],
      exclusive_end_primary_key: [
        { name: 'partition_key', value: device_id },
        { name: 'row_key', value: startRowKey }
      ],
      limit: parseInt(limit),
      columns_to_get: [
        'device_id', 'has_motion', 'oss_path_original',
        'oss_path_thumbnail', 'created_at', 'image_size'
      ]
    };

    const result = await new Promise((resolve, reject) => {
      otsClient.getRange(params, (err, data) => {
        if (err) reject(err);
        else resolve(data);
      });
    });

    // 解析结果
    const images = result.rows.map(row => {
      const attrs = {};
      row.attributes.forEach(attr => {
        attrs[attr.columnName] = attr.columnValue;
      });
      return {
        partition_key: row.primaryKey[0].value,
        row_key: row.primaryKey[1].value,
        ...attrs
      };
    });

    return {
      statusCode: 200,
      body: JSON.stringify({
        images,
        next_token: result.next_start_primary_key
      }),
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
