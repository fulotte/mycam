// firmware/include/http_server.h
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

class HTTPServer {
public:
    void begin();
    void setBuffer(camera_fb_t* fb);

private:
    AsyncWebServer server = AsyncWebServer(HTTP_PORT);
    camera_fb_t* currentFb = nullptr;

    void setupRoutes();
};
