#pragma once
#include <Arduino.h>
#include "config_storage.h"
#include "wifi_manager.h"
#include "captive_portal.h"

enum ProvisioningState {
    STARTUP,
    STA_CONNECTING,
    STA_CONNECTED,
    AP_PROVISIONING,
    AP_SWITCHING
};

class ProvisioningManager {
private:
    ConfigStorage* storage;
    WiFiManager& wifiManager;
    CaptivePortal captivePortal;
    ProvisioningState state = STARTUP;
    unsigned long stateStartTime = 0;

    static const unsigned long STA_TIMEOUT_MS = 30000;
    static const unsigned long SWITCHING_TIMEOUT_MS = 30000;

public:
    ProvisioningManager(ConfigStorage* s, WiFiManager& wm)
        : storage(s), wifiManager(wm) {}

    bool begin();
    void update();
    ProvisioningState getState() const { return state; }

    // API 方法
    bool saveAndConnect(const char* ssid, const char* password);
    bool resetConfig();

private:
    void enterAPMode();
    bool tryConnectSaved();
};
