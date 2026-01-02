#pragma once
#include <DNSServer.h>
#include "config.h"

class CaptivePortal {
private:
    DNSServer* dnsServer = nullptr;
    bool enabled = false;

public:
    void begin();
    void update();
    void stop();
    bool isEnabled() const { return enabled; }
};
