#include "captive_portal.h"
#include "logger.h"

void CaptivePortal::begin() {
    if (dnsServer) return;

    dnsServer = new DNSServer();
    dnsServer->start(53, "*", WiFi.softAPIP());
    enabled = true;
    Logger::info("CAPTIVE", "DNS server started");
}

void CaptivePortal::update() {
    if (dnsServer && enabled) {
        dnsServer->processNextRequest();
    }
}

void CaptivePortal::stop() {
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
    enabled = false;
    Logger::info("CAPTIVE", "DNS server stopped");
}
