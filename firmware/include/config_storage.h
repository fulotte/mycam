#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// WiFi 配置结构
struct WiFiConfig {
    char ssid[32];
    char password[63];
    bool saved;
    unsigned long lastSeen;

    WiFiConfig() : saved(false), lastSeen(0) {
        ssid[0] = '\0';
        password[0] = '\0';
    }
};

// 抽象存储接口
class ConfigStorage {
public:
    virtual bool init() = 0;
    virtual bool hasConfig() = 0;
    virtual bool loadConfig(WiFiConfig& config) = 0;
    virtual bool saveConfig(const WiFiConfig& config) = 0;
    virtual bool clearConfig() = 0;
    virtual ~ConfigStorage() = default;
};

// LittleFS 实现
class LittleFSConfigStorage : public ConfigStorage {
private:
    static const char* CONFIG_PATH;

public:
    bool init() override {
        return LittleFS.begin(true);
    }

    bool hasConfig() override {
        return LittleFS.exists(CONFIG_PATH);
    }

    bool loadConfig(WiFiConfig& config) override {
        File file = LittleFS.open(CONFIG_PATH, "r");
        if (!file) return false;

        // 使用 ArduinoJson 读取
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) return false;

        strlcpy(config.ssid, doc["ssid"] | "", sizeof(config.ssid));
        strlcpy(config.password, doc["password"] | "", sizeof(config.password));
        config.saved = doc["saved"] | false;
        config.lastSeen = doc["lastSeen"] | 0;

        return config.saved;
    }

    bool saveConfig(const WiFiConfig& config) override {
        File file = LittleFS.open(CONFIG_PATH, "w");
        if (!file) return false;

        StaticJsonDocument<256> doc;
        doc["ssid"] = config.ssid;
        doc["password"] = config.password;
        doc["saved"] = config.saved;
        doc["lastSeen"] = config.lastSeen;

        bool success = serializeJson(doc, file) > 0;
        file.close();
        return success;
    }

    bool clearConfig() override {
        return LittleFS.remove(CONFIG_PATH);
    }
};
