// backend/shared/error-handler.js

class AppError extends Error {
  constructor(message, statusCode) {
    super(message);
    this.statusCode = statusCode;
    this.isOperational = true;
    Error.captureStackTrace(this, this.constructor);
  }
}

class ErrorHandler {
  static async handleError(err, context) {
    // 记录错误
    await this.logError(err, context);

    // 发送告警（严重错误）
    if (err.statusCode >= 500) {
      await this.sendAlert(err, context);
    }
  }

  static async logError(err, context) {
    const logEntry = {
      timestamp: new Date().toISOString(),
      error: {
        message: err.message,
        stack: err.stack,
        code: err.code
      },
      context
    };

    // 写入日志
    console.error(JSON.stringify(logEntry));
  }

  static async sendAlert(err, context) {
    // TODO: 发送到钉钉/飞书群
    console.error('[ALERT] Critical error:', err.message);
  }

  static response(error) {
    if (error.isOperational) {
      return {
        statusCode: error.statusCode,
        body: JSON.stringify({
          error: error.message
        }),
        headers: { 'Content-Type': 'application/json' }
      };
    }

    // 未知错误
    return {
      statusCode: 500,
      body: JSON.stringify({
        error: 'Internal server error'
      }),
      headers: { 'Content-Type': 'application/json' }
    };
  }
}

module.exports = { AppError, ErrorHandler };
