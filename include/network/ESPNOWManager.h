#pragma once

#ifdef BUILD_ESPNOW

#include <Arduino.h>
#include <esp_now.h>

class ESPNOWManager
{
public:
    ESPNOWManager();
    void begin();
    void loop();
    void SetEnabled(bool enabled);
    bool IsEnabled() { return _enabled; }
    uint8_t GetWiFiChannel();
    void ScheduleLoadConfig();

private:
    bool _enabled = false;
    time_t ConfigLoadNeeded = 0;
    void LoadConfig();
    static void onDataRcv(const uint8_t *mac, const uint8_t *incomingData, int len);
};

#endif // BUILD_ESPNOW
