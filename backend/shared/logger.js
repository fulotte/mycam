// backend/shared/logger.js

class Logger {
  static info(message, context = {}) {
    console.log(JSON.stringify({
      level: 'INFO',
      timestamp: new Date().toISOString(),
      message,
      ...context
    }));
  }

  static warn(message, context = {}) {
    console.warn(JSON.stringify({
      level: 'WARN',
      timestamp: new Date().toISOString(),
      message,
      ...context
    }));
  }

  static error(message, context = {}) {
    console.error(JSON.stringify({
      level: 'ERROR',
      timestamp: new Date().toISOString(),
      message,
      ...context
    }));
  }

  static debug(message, context = {}) {
    if (process.env.DEBUG === 'true') {
      console.log(JSON.stringify({
        level: 'DEBUG',
        timestamp: new Date().toISOString(),
        message,
        ...context
      }));
    }
  }
}

module.exports = Logger;
