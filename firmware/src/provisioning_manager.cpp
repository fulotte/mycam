#include "provisioning_manager.h"
#include "logger.h"
#include <WiFi.h>

bool ProvisioningManager::begin() {
    Logger::info("PROV", "Provisioning manager starting...");

    if (!storage->init()) {
        Logger::error("PROV", "Storage init failed, going to AP mode");
        enterAPMode();
        return true;
    }

    state = STARTUP;
    stateStartTime = millis();

    // 尝试连接已保存的配置
    if (storage->hasConfig()) {
        Logger::info("PROV", "Found saved config, trying to connect...");
        return tryConnectSaved();
    } else {
        Logger::info("PROV", "No saved config, entering AP mode");
        enterAPMode();
        return true;
    }
}

void ProvisioningManager::update() {
    captivePortal.update();

    switch (state) {
        case STA_CONNECTING:
            if (wifiManager.isConnected()) {
                state = STA_CONNECTED;
                Logger::info("PROV", "STA connected successfully");
            } else if (millis() - stateStartTime > STA_TIMEOUT_MS) {
                Logger::warn("PROV", "STA connection timeout, entering AP mode");
                enterAPMode();
            }
            break;

        case AP_SWITCHING:
            if (wifiManager.isConnected()) {
                state = STA_CONNECTED;
                captivePortal.stop();
                WiFi.mode(WIFI_STA);
                Logger::info("PROV", "Switched to STA mode successfully");
            } else if (millis() - stateStartTime > SWITCHING_TIMEOUT_MS) {
                Logger::error("PROV", "Switching timeout, staying in AP mode");
                state = AP_PROVISIONING;
            }
            break;

        default:
            break;
    }
}

bool ProvisioningManager::saveAndConnect(const char* ssid, const char* password) {
    Logger::info("PROV", "Saving config for SSID: %s", ssid);

    WiFiConfig config;
    config.saved = true;
    config.lastSeen = millis();
    strlcpy(config.ssid, ssid, sizeof(config.ssid));
    strlcpy(config.password, password, sizeof(config.password));

    if (!storage->saveConfig(config)) {
        Logger::error("PROV", "Failed to save config");
        return false;
    }

    Logger::info("PROV", "Config saved, switching to STA mode...");
    state = AP_SWITCHING;
    stateStartTime = millis();

    WiFi.begin(ssid, password);
    return true;
}

bool ProvisioningManager::resetConfig() {
    Logger::info("PROV", "Resetting config...");
    bool cleared = storage->clearConfig();
    if (cleared) {
        Logger::info("PROV", "Config cleared, restarting...");
        delay(1000);
        ESP.restart();
    }
    return cleared;
}

void ProvisioningManager::enterAPMode() {
    state = AP_PROVISIONING;

    // 使用 MAC 后 6 位作为 SSID 后缀
    String mac = WiFi.macAddress();
    String suffix = mac.substring(mac.length() - 6);
    suffix.replace(":", "");

    String apSSID = "MyCam-" + suffix;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSSID.c_str());
    WiFi.softAPConfig(IPAddress(192, 164, 4, 1), IPAddress(192, 164, 4, 1), IPAddress(255, 255, 255, 0));

    captivePortal.begin();

    Logger::info("PROV", "AP mode started: %s", apSSID.c_str());
    Logger::info("PROV", "AP IP: %s", WiFi.softAPIP().toString().c_str());
}

bool ProvisioningManager::tryConnectSaved() {
    WiFiConfig config;
    if (!storage->loadConfig(config)) {
        Logger::error("PROV", "Failed to load config");
        enterAPMode();
        return true;
    }

    state = STA_CONNECTING;
    stateStartTime = millis();

    if (!wifiManager.begin(config.ssid, config.password)) {
        Logger::warn("PROV", "STA connection failed");
        enterAPMode();
    }

    return true;
}
