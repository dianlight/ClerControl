#pragma once
#include <AceRoutine.h>

#define hwReboot() wdt_enable(WDTO_15MS); while (1)


enum BtnStatus{
    NONE,
    CLICK,
    DOUBLE_CLICK,
    TRIPLE_CLICK,
    LONG_PRESS
};

struct ClrStatus {
    bool moving;
    bool closed;
    BtnStatus btnStatus;
    bool lightOn;
    long lastAnnunced;
    long lastLightAnnunced;
} __attribute__((packed, aligned(1)));

class MqttMessage {
    public:
        const char* topic;
        const char* json;
};

ace_routine::Channel<MqttMessage> mqttChannel;

extern ClrStatus _currentStatus;
extern ClrStatus _lastKnowStatus;