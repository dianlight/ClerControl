#pragma once
#include <AceRoutine.h>


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
};

class MqttMessage {
    public:
        const char* topic;
        const char* json;
};

ace_routine::Channel<MqttMessage> mqttChannel;

extern ClrStatus _currentStatus;
extern ClrStatus _lastKnowStatus;