
#pragma once

#include "input/InputCommon.hpp"

#ifdef ARDUINO_ARCH_ESP32
#include <esp_now.h>

class c_InputESPNOW : public c_InputCommon
{
public:
    c_InputESPNOW(c_InputMgr::e_InputChannelIds NewInputChannelId,
                   c_InputMgr::e_InputType       NewChannelType,
                   uint32_t                        BufferSize) :
        c_InputCommon(NewInputChannelId, NewChannelType, BufferSize) {}
    ~c_InputESPNOW() {}

    virtual void Begin();
    virtual void GetConfig(JsonObject& json);
    virtual void GetStatus(JsonObject& json);
    virtual bool SetConfig(JsonObject& json);
    virtual bool isEnabled() { return enabled; }
    virtual void GetDriverName(String& sDriverName) { sDriverName = "ESP-NOW"; }
    virtual void Process() {}
    virtual void SetBufferInfo(uint32_t BufferSize) {}

    String getPriority() { return priority; }
    uint32_t getLastPacketTime() { return lastPacketTime; }
    uint32_t getTimeout() { return timeout; }

private:
    friend class c_InputMgr;
    bool enabled = false;
    uint8_t channel = 1;
    String macAddress;
    String priority;
    uint32_t timeout = 5;
    uint32_t lastPacketTime = 0;

    static void onDataReceived(const uint8_t* mac, const uint8_t* incomingData, int len);
};

#endif // ARDUINO_ARCH_ESP32
