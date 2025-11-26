#include "InputESPNOW.h"
#include "input/InputMgr.hpp"

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <esp_wifi.h>

c_InputESPNOW* espnowInput = nullptr;

void c_InputESPNOW::Begin()
{
    espnowInput = this;
    if (enabled)
    {
        WiFi.mode(WIFI_AP_STA);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        if (esp_now_init() != ESP_OK)
        {
            logcon(F("Error initializing ESP-NOW"));
            return;
        }
        esp_now_register_recv_cb(onDataReceived);
    }
}

void c_InputESPNOW::GetConfig(JsonObject& json)
{
    json[F("enabled")] = enabled;
    json[F("channel")] = channel;
    json[F("mac_address")] = macAddress;
    json[F("priority")] = priority;
    json[F("timeout")] = timeout;
}

void c_InputESPNOW::GetStatus(JsonObject& json)
{
}

bool c_InputESPNOW::SetConfig(JsonObject& json)
{
    if (json.containsKey(F("enabled")))
    {
        enabled = json[F("enabled")];
    }

    if (json.containsKey(F("channel")))
    {
        channel = json[F("channel")];
    }

    if (json.containsKey(F("mac_address")))
    {
        macAddress = json[F("mac_address")].as<String>();
    }

    if (json.containsKey(F("priority")))
    {
        priority = json[F("priority")].as<String>();
    }

    if (json.containsKey(F("timeout")))
    {
        timeout = json[F("timeout")];
    }

    Begin();
    return true;
}

void c_InputESPNOW::onDataReceived(const uint8_t* mac, const uint8_t* incomingData, int len)
{
    if (espnowInput && espnowInput->isEnabled())
    {
        if (espnowInput->macAddress.length() > 0)
        {
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            if (espnowInput->macAddress != macStr)
            {
                return;
            }
        }
        espnowInput->lastPacketTime = millis();
        // Handle received data
        OutputMgr.WriteChannelData(1, len, (uint8_t*)incomingData);
    }
}

#endif // ARDUINO_ARCH_ESP32
