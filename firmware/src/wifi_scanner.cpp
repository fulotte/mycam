#include "wifi_scanner.h"
#include "logger.h"

int WiFiScanner::scan(NetworkInfo* networks, int maxNetworks) {
    Logger::info("SCAN", "开始扫描 WiFi 网络...");

    // 扫描可用网络
    int n = WiFi.scanNetworks();
    Logger::info("SCAN", "发现 %d 个网络", n);

    // 如果没有找到网络，直接返回
    if (n <= 0) {
        return 0;
    }

    // 保存扫描结果
    int count = 0;
    for (int i = 0; i < n && count < maxNetworks; i++) {
        NetworkInfo& info = networks[count];

        // 使用 strlcpy 防止缓冲区溢出
        strlcpy(info.ssid, WiFi.SSID(i).c_str(), sizeof(info.ssid));

        // 保存信号强度
        info.rssi = WiFi.RSSI(i);

        // 判断是否加密（非开放网络）
        info.encrypted = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;

        count++;
    }

    // 释放扫描结果占用的内存
    WiFi.scanDelete();

    Logger::info("SCAN", "扫描完成，返回 %d 个网络", count);
    return count;
}

int WiFiScanner::scanCount() {
    // 扫描网络并返回数量
    int n = WiFi.scanNetworks();

    // 释放内存
    WiFi.scanDelete();

    return n;
}
