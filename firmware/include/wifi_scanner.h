#pragma once
#include <Arduino.h>
#include <WiFi.h>

// WiFi 网络信息结构体
struct NetworkInfo {
    char ssid[32];      // WiFi 名称
    int rssi;           // 信号强度 (dBm)
    bool encrypted;     // 是否加密

    NetworkInfo() : rssi(0), encrypted(false) {
        ssid[0] = '\0';
    }
};

// WiFi 扫描器类
class WiFiScanner {
public:
    // 扫描可用的 WiFi 网络
    // @param networks 存储扫描结果的数组
    // @param maxNetworks 数组最大容量
    // @return 实际找到的网络数量
    int scan(NetworkInfo* networks, int maxNetworks);

    // 快速获取网络数量（不保存详细信息）
    // @return 可用网络数量
    int scanCount();
};
