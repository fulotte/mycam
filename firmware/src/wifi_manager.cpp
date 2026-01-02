// firmware/src/wifi_manager.cpp
#include "wifi_manager.h"
#include "config.h"

bool WiFiManager::begin(const char* ssid, const char* password) {
    Serial.printf("Connecting to WiFi: %s\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);  // 保持活跃以降低延迟
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.printf("IP address: %s\n", getIP().c_str());
        return true;
    }

    Serial.println("\nWiFi connection failed!");
    return false;
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::update() {
    if (!isConnected() &&
        millis() - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS) {
        reconnect();
    }
}

void WiFiManager::reconnect() {
    Serial.println("Attempting WiFi reconnection...");
    lastReconnectAttempt = millis();
    WiFi.reconnect();
}

String WiFiManager::getIP() {
    return WiFi.localIP().toString();
}

String WiFiManager::getMAC() {
    return WiFi.macAddress();
}
