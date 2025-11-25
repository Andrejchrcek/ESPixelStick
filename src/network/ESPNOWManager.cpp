/*
* ESPNOWManager.cpp - ESP-NOW Management class
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

#include "ESPNOWManager.h"
#include "InputMgr.h"

#ifdef ESPNOW_SUPPORT

ESPNOWManager::ESPNOWManager() :
    enabled(false)
{
    memset(peer_mac, 0, sizeof(peer_mac));
}

void ESPNOWManager::GetConfig(JsonObject& json)
{
    json["enabled"] = enabled;
    json["peer_mac"] = peer_mac;
}

void ESPNOWManager::SetConfig(JsonObject& json)
{
    enabled = json["enabled"];
    strlcpy(peer_mac, json["peer_mac"], sizeof(peer_mac));
}

void ESPNOWManager::begin()
{
    if (!enabled)
    {
        return;
    }

    if (WiFi.getMode() != WIFI_STA)
    {
        WiFi.mode(WIFI_STA);
    }

    if (esp_now_init() != ESP_OK)
    {
        logcon("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    addPeer(broadcastAddress);
    if (strlen(peer_mac) > 0)
    {
        uint8_t mac[6];
        sscanf(peer_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        addPeer(mac);
    }
    keepAliveTimer.Start(5000);
}

void ESPNOWManager::loop()
{
    if (!enabled)
    {
        return;
    }

    if (keepAliveTimer.IsExpired())
    {
        sendData("keep-alive", "");
        keepAliveTimer.Start(5000);
    }
}

void ESPNOWManager::sendData(const String& command, const String& payload)
{
    struct_message message;
    strncpy(message.command, command.c_str(), sizeof(message.command) - 1);
    message.command[sizeof(message.command) - 1] = '\0';
    strncpy(message.payload, payload.c_str(), sizeof(message.payload) - 1);
    message.payload[sizeof(message.payload) - 1] = '\0';

    esp_now_send(broadcastAddress, (uint8_t *)&message, sizeof(message));
}

void ESPNOWManager::addPeer(const uint8_t * mac_addr)
{
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, mac_addr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        logcon("Failed to add peer");
        return;
    }
}

void ESPNOWManager::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // logcon(String("Last Packet Send Status: ") + (status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail"));
}

void ESPNOWManager::onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    struct_message message;
    memcpy(&message, incomingData, sizeof(message));

    String command = message.command;
    String payload = message.payload;

    WebMgr.ProcessESPNOWCommand(command, payload);
}

#endif // ESPNOW_SUPPORT
