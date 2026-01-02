// firmware/include/wifi_manager.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    bool begin(const char* ssid, const char* password);
    bool isConnected();
    void update();
    String getIP();
    String getMAC();

private:
    unsigned long lastReconnectAttempt = 0;
    void reconnect();
};
