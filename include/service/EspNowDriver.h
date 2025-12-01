#pragma once
/*
* EspNowDriver.h
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2025 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include "ESPixelStick.h"
#include <ArduinoJson.h>

class c_EspNowDriver
{
private:
    bool Enabled = false;
    uint8_t Channel = 1;
    bool HasBeenInitialized = false;

    void validateConfiguration();

public:
    c_EspNowDriver();
    virtual ~c_EspNowDriver();

    void Begin();
    void Poll();
    void GetConfig(JsonObject &jsonConfig);
    bool SetConfig(JsonObject &jsonConfig);
    void GetStatus(JsonObject &jsonStatus);
    void SendPacket(uint8_t *data, size_t len);
    void ProcessConfig(AsyncWebServerRequest* request);
    void HandleConfigBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void ProcessTest(AsyncWebServerRequest* request);
};

extern c_EspNowDriver EspNowDriver;
