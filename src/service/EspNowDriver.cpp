/*
* EspNowDriver.cpp
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

#include "service/EspNowDriver.h"
#include "service/FPPDiscovery.h"
#include "FileMgr.hpp"
#include "network/NetworkMgr.hpp"

#ifdef ARDUINO_ARCH_ESP32
#include <esp_now.h>
#include <WiFi.h>
#else
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif

// Global callback function needs to be outside the class or static
#ifdef ARDUINO_ARCH_ESP32
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
#else
void OnDataRecv(uint8_t *mac_addr, uint8_t *data, uint8_t data_len)
#endif
{
    if (data_len > 0)
    {
        // Construct a fake IP address from the MAC address to satisfy the FPPDiscovery API
        // This IP is not used for routing, just identification in the FPP logic if needed.
        IPAddress senderIP(mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        FPPDiscovery.ProcessFPPPacket((uint8_t *)data, (size_t)data_len, senderIP);
    }
}

c_EspNowDriver::c_EspNowDriver()
{
}

c_EspNowDriver::~c_EspNowDriver()
{
}

void c_EspNowDriver::Begin()
{
    if (HasBeenInitialized)
    {
        return;
    }

    // Load configuration
    // We don't have a dedicated config file loader here yet, we rely on SetConfig being called
    // or we can load from a file here. Since this is a service, maybe we should hook into the main config load?
    // For now, let's assume config is passed via SetConfig or loaded from a specific file if we want persistence separate from main config.
    // The requirement implies a new tab, so maybe a separate config file or part of the main config.
    // I'll implement loading from "espnow_config.json" for persistence.

    DynamicJsonDocument jsonDoc(1024);
    if (FileMgr.LoadFlashFile("/espnow_config.json", jsonDoc))
    {
        JsonObject jsonConfig = jsonDoc.as<JsonObject>();
        SetConfig(jsonConfig);
    }

    HasBeenInitialized = true;
}

void c_EspNowDriver::Poll()
{
    // ESP-NOW doesn't strictly need polling for RX, it uses callbacks.
    // But if we need to manage state or retries, we could do it here.
}

void c_EspNowDriver::validateConfiguration()
{
    if (Channel < 1) Channel = 1;
    if (Channel > 13) Channel = 13; // Typical WiFi channels
}

void c_EspNowDriver::GetConfig(JsonObject &jsonConfig)
{
    jsonConfig["enabled"] = Enabled;
    jsonConfig["channel"] = Channel;
}

bool c_EspNowDriver::SetConfig(JsonObject &jsonConfig)
{
    bool ConfigChanged = false;
    if (jsonConfig.containsKey("enabled"))
    {
        bool newEnabled = jsonConfig["enabled"];
        if (newEnabled != Enabled)
        {
            Enabled = newEnabled;
            ConfigChanged = true;
        }
    }

    if (jsonConfig.containsKey("channel"))
    {
        uint8_t newChannel = jsonConfig["channel"];
        if (newChannel != Channel)
        {
            Channel = newChannel;
            ConfigChanged = true;
        }
    }

    validateConfiguration();

    if (ConfigChanged)
    {
        // Re-init ESP-NOW if state changed
        if (Enabled)
        {
            // If already initialized, we might need to deinit first?
            // esp_now_deinit(); // Not always safe to call repeatedly without checking

            // Check if WiFi is active
            if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP)
            {
                // Must use current WiFi channel
                // We don't change channel here if WiFi is connected.
            }
            else
            {
                // Set WiFi channel if not connected
                // Note: This might require setting WiFi mode to STA or AP_STA even if not connected
                WiFi.mode(WIFI_STA);
                WiFi.setChannel(Channel);
            }

            if (esp_now_init() != 0)
            {
                logcon("Error initializing ESP-NOW");
            }
            else
            {
                esp_now_register_recv_cb(OnDataRecv);
                logcon("ESP-NOW Initialized");

                // Add broadcast peer for sending
                uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#ifdef ARDUINO_ARCH_ESP32
                esp_now_peer_info_t peerInfo;
                memcpy(peerInfo.peer_addr, broadcastMac, 6);
                peerInfo.channel = 0;
                peerInfo.encrypt = false;
                if (esp_now_add_peer(&peerInfo) != ESP_OK){
                    logcon("Failed to add peer");
                }
#else
                esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
                esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
#endif
            }
        }
        else
        {
            esp_now_deinit();
            logcon("ESP-NOW De-Initialized");
        }

        // Save config
        DynamicJsonDocument jsonDoc(1024);
        JsonObject jsonRoot = jsonDoc.to<JsonObject>();
        GetConfig(jsonRoot);
        FileMgr.SaveFlashFile("/espnow_config.json", jsonDoc);
    }
    return true;
}

void c_EspNowDriver::GetStatus(JsonObject &jsonStatus)
{
    JsonObject espnowStatus = jsonStatus.createNestedObject("EspNow");
    espnowStatus["enabled"] = Enabled;
    espnowStatus["configured_channel"] = Channel;
    espnowStatus["current_channel"] = WiFi.channel();
    espnowStatus["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
}

void c_EspNowDriver::SendPacket(uint8_t *data, size_t len)
{
    if (!Enabled) return;

    uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastMac, data, len);
}

void c_EspNowDriver::ProcessConfig(AsyncWebServerRequest* request)
{
    if (request->method() == HTTP_GET)
    {
        AsyncJsonResponse *response = new AsyncJsonResponse();
        JsonObject root = response->getRoot();
        GetConfig(root);
        response->setLength();
        request->send(response);
    }
    // POST handled in HandleConfigBody
}

void c_EspNowDriver::HandleConfigBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    // Accumulate the body
    // For small JSON configs, we can assume it fits in one packet or simple accumulation.
    // However, AsyncWebServer doesn't persist state across chunks easily without custom objects.
    // But since `script.js` sends small JSON, it likely comes in one chunk.

    // We'll try to parse the JSON directly from the data chunk if it's the full body.
    if (index == 0 && len == total)
    {
        DynamicJsonDocument jsonDoc(1024);
        DeserializationError error = deserializeJson(jsonDoc, data, len);
        if (!error)
        {
            JsonObject jsonConfig = jsonDoc.as<JsonObject>();
            SetConfig(jsonConfig);
            request->send(200, "application/json", "{\"status\":\"OK\"}");
        }
        else
        {
            request->send(400, "application/json", "{\"status\":\"Invalid JSON\"}");
        }
    }
    else
    {
        // Chunked upload not implemented for simplicity
        request->send(501, "application/json", "{\"status\":\"Not Implemented\"}");
    }
}

void c_EspNowDriver::ProcessTest(AsyncWebServerRequest* request)
{
    // Send a test packet
    // Construct a dummy FPP Sync packet
    struct {
        char header[4];
        uint8_t packet_type;
        uint16_t data_len;
        uint8_t sync_action;
        uint8_t sync_type;
        uint32_t frame_number;
        float seconds_elapsed;
        char filename[1]; // truncated
    } testPacket;

    testPacket.header[0] = 'F';
    testPacket.header[1] = 'P';
    testPacket.header[2] = 'P';
    testPacket.header[3] = 'D';
    testPacket.packet_type = 1; // CTRL_PKT_SYNC
    testPacket.data_len = sizeof(testPacket);
    testPacket.sync_action = 0; // START
    testPacket.sync_type = 0; // FSEQ
    testPacket.frame_number = 0;
    testPacket.seconds_elapsed = 0.0;
    testPacket.filename[0] = 0;

    SendPacket((uint8_t*)&testPacket, sizeof(testPacket));
    request->send(200, "application/json", "{\"status\":\"Sent\"}");
}

c_EspNowDriver EspNowDriver;
