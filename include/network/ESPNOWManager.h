/*
* ESPNOWManager.h - ESP-NOW Management class
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2021, 2022 Shelby Merrick
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

#pragma once

#include "ESPixelStick.h"

#ifdef ESPNOW_SUPPORT

#include <esp_now.h>
#include <WiFi.h>
#include "FastTimer.h"

typedef struct struct_message {
    char command[32];
    char payload[220];
} struct_message;

class ESPNOWManager
{
public:
    ESPNOWManager();
    void begin();
    void loop();
    void GetConfig(JsonObject& json);
    void SetConfig(JsonObject& json);
    void sendData(const String& command, const String& payload);

private:
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
    void addPeer(const uint8_t * mac_addr);

    bool enabled;
    char peer_mac[18];
    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    FastTimer keepAliveTimer;
};

#endif // ESPNOW_SUPPORT
