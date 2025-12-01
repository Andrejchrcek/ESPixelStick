
#include "InputESPNOW.h"
#include "input/InputMgr.hpp"
#include "service/FPPDiscovery.h"
#include "service/fseq.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#include <WiFi.h>
#ifdef ARDUINO_ARCH_ESP32
#include <esp_wifi.h>
#endif

c_InputESPNOW* espnowInput = nullptr;

void c_InputESPNOW::Begin()
{
    espnowInput = this;
    if (enabled)
    {
        logcon(F("Initializing ESP-NOW..."));
        WiFi.mode(WIFI_AP_STA);
        logcon(String(F("Setting ESP-NOW channel to ")) + String(channel));
#ifdef ARDUINO_ARCH_ESP32
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
#elif defined(ARDUINO_ARCH_ESP8266)
        wifi_set_channel(channel);
#endif
        if (esp_now_init() != ESP_OK)
        {
            logcon(F("Error initializing ESP-NOW"));
            return;
        }
        logcon(F("ESP-NOW initialized."));
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

bool c_InputESPNOW::SetConfig(ArduinoJson::JsonObject& jsonConfig)
{
    if (jsonConfig.containsKey(F("enabled")))
    {
        enabled = jsonConfig[F("enabled")];
    }

    if (jsonConfig.containsKey(F("channel")))
    {
        channel = jsonConfig[F("channel")];
    }

    if (jsonConfig.containsKey(F("mac_address")))
    {
        macAddress = jsonConfig[F("mac_address")].as<String>();
    }

    if (jsonConfig.containsKey(F("priority")))
    {
        priority = jsonConfig[F("priority")].as<String>();
    }

    if (jsonConfig.containsKey(F("timeout")))
    {
        timeout = jsonConfig[F("timeout")];
    }

    Begin();
    return true;
}

void c_InputESPNOW::onDataReceived(const uint8_t* mac, const uint8_t* incomingData, int len)
{
    if (espnowInput && espnowInput->isEnabled())
    {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Serial.println(String(F("ESP-NOW packet received from ")) + String(macStr) + String(F(", length ")) + String(len));

        if (espnowInput->macAddress.length() > 0)
        {
            if (!espnowInput->macAddress.equalsIgnoreCase(macStr))
            {
                Serial.println(String(F("Ignoring packet from ")) + String(macStr) + String(F(" due to MAC address filter.")));
                return;
            }
        }
        espnowInput->lastPacketTime = millis();

        FPPPacket* fppPacket = (FPPPacket*)incomingData;
        if (fppPacket->header[0] != 'F' || fppPacket->header[1] != 'P' || fppPacket->header[2] != 'P' || fppPacket->header[3] != 'D')
        {
            return;
        }

        switch (fppPacket->packet_type)
        {
            case CTRL_PKT_SYNC:
            {
                FPPMultiSyncPacket* msPacket = (FPPMultiSyncPacket*)incomingData;
                if (msPacket->sync_type == SYNC_FILE_SEQ)
                {
                    FPPDiscovery.ProcessSyncPacket(msPacket->sync_action, String(msPacket->filename), msPacket->seconds_elapsed);
                }
                break;
            }
            case CTRL_PKT_BLANK:
            {
                FPPDiscovery.ProcessBlankPacket();
                break;
            }
        }
    }
}

#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
