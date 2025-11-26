#include "ESPNOWManager.h"

#ifdef BUILD_ESPNOW

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESPixelStick.h"
#include "output/OutputMgr.hpp"
#include "output/OutputRmt.hpp"
#include "WebMgr.hpp"
#include "input/InputMgr.hpp"
#include "FileMgr.hpp"
#include <TimeLib.h>

extern void DeleteConfig();

// Define a packet type enum
enum ESPNOW_Packet_Type : uint8_t {
    ESPNOW_PACKET_TYPE_SYNC,
    ESPNOW_PACKET_TYPE_COMMAND
};

// Define a common packet header
struct __attribute__((packed)) ESPNOW_Packet_Header_t {
    ESPNOW_Packet_Type type;
};

// Define a data structure for the sync messages
struct __attribute__((packed)) FPP_Sync_Pkt_t
{
    ESPNOW_Packet_Header_t header;
    uint16_t PacketLen;
    uint8_t  SequenceNum;
    uint8_t  Reserved;
    uint32_t FrameNum;
    uint32_t Time;
};

// Pointer to the ESPNOWManager instance
static ESPNOWManager* espnowManagerInstance = nullptr;

// Define a data structure for the command messages
struct __attribute__((packed)) ESPNOW_Command_Pkt_t {
    ESPNOW_Packet_Header_t header;
    char command[32];
    char value[32];
};

void ProcessESPNOWCommand(const ESPNOW_Command_Pkt_t& command)
{
    if (strcmp(command.command, "reboot") == 0)
    {
        String reason = "ESPNOW reboot command received";
        RequestReboot(reason, 100);
    }
    else if (strcmp(command.command, "factory_reset") == 0)
    {
        InputMgr.DeleteConfig();
        OutputMgr.DeleteConfig();
        DeleteConfig();
        String reason = "ESPNOW factory reset command received";
        RequestReboot(reason, 100);
    }
    else if (strcmp(command.command, "clear_stats") == 0)
    {
        InputMgr.ClearStatistics();
        OutputMgr.ClearStatistics();
    }
    else if (strcmp(command.command, "settime") == 0)
    {
        time_t newTime = atol(command.value);
        WebMgr.ProcessSetTimeRequest(newTime);
    }
    else if (strncmp(command.command, "relay/", 6) == 0)
    {
        String cmd(command.command);
        int first_slash = cmd.indexOf('/');
        int second_slash = cmd.indexOf('/', first_slash + 1);
        if (second_slash > first_slash)
        {
            int relayId = cmd.substring(first_slash + 1, second_slash).toInt();
            String relayState = cmd.substring(second_slash + 1);
            String response;
            OutputMgr.RelayUpdate(relayId, relayState, response);
        }
    }
}

// Callback function to handle incoming messages
void ESPNOWManager::onDataRcv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    (void)mac;
    if (!espnowManagerInstance || !espnowManagerInstance->_enabled || len < sizeof(ESPNOW_Packet_Header_t)) {
        return;
    }

    ESPNOW_Packet_Header_t header;
    memcpy(&header, incomingData, sizeof(header));

    if (header.type == ESPNOW_PACKET_TYPE_SYNC && len == sizeof(FPP_Sync_Pkt_t)) {
        FPP_Sync_Pkt_t syncPacket;
        memcpy(&syncPacket, incomingData, sizeof(syncPacket));

        // Notify the RMT task to render a new frame
        std::vector<c_OutputRmt*> rmtInstances;
        OutputMgr.GetRmtInstances(rmtInstances);
        for (auto rmtInstance : rmtInstances) {
            rmtInstance->NotifyRmtTask();
        }
    } else if (header.type == ESPNOW_PACKET_TYPE_COMMAND && len == sizeof(ESPNOW_Command_Pkt_t)) {
        ESPNOW_Command_Pkt_t commandPacket;
        memcpy(&commandPacket, incomingData, sizeof(commandPacket));
        ProcessESPNOWCommand(commandPacket);
    } else {
        Serial.printf("Received unknown ESP-NOW packet of %d bytes from %02x:%02x:%02x:%02x:%02x:%02x\n", len, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}

ESPNOWManager::ESPNOWManager()
{
    // Constructor
    espnowManagerInstance = this;
}

void ESPNOWManager::begin()
{
    LoadConfig();

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        logcon(F("Error initializing ESP-NOW"));
        return;
    }

    // Register for a callback function that will be called when data is received
    esp_now_register_recv_cb(onDataRcv);
}

void ESPNOWManager::loop()
{
    if (ConfigLoadNeeded && (abs(now() - ConfigLoadNeeded) > LOAD_CONFIG_DELAY))
    {
        LoadConfig();
    }
}

void ESPNOWManager::ScheduleLoadConfig()
{
    ConfigLoadNeeded = now();
}

void ESPNOWManager::LoadConfig()
{
    ConfigLoadNeeded = 0;

    if (!FileMgr.LoadFlashFile("/espnow.json", [](DynamicJsonDocument &jsonDoc)
    {
        JsonObject espnow_json = jsonDoc["espnow"];
        if (espnow_json)
        {
            bool enabled = false;
            setFromJSON(enabled, espnow_json, "enabled");
            espnowManagerInstance->SetEnabled(enabled);
        }
    }))
    {
        logcon(F("Could not load /espnow.json"));
        // Create a default config
        DynamicJsonDocument jsonDoc(128);
        JsonObject espnow_json = jsonDoc.createNestedObject("espnow");
        espnow_json["enabled"] = false;
        FileMgr.SaveFlashFile("/espnow.json", jsonDoc);
    }
}

void ESPNOWManager::SetEnabled(bool enabled)
{
    _enabled = enabled;
}

uint8_t ESPNOWManager::GetWiFiChannel()
{
    return WiFi.channel();
}

#endif // BUILD_ESPNOW
