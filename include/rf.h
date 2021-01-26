#pragma once
#include <Arduino.h>
#include <AceRoutine.h>
#include <PJONOverSampling.h>

#include "pin.h"
#include "credentials.h"

PJONOverSampling bus(CC_RF_BUS);

COROUTINE(rfLoop)
{
    COROUTINE_LOOP()
    {
        digitalWrite(RF_LED, LOW);
        bus.update();
        bus.receive(20000); // 20ms wait
        COROUTINE_YIELD();
    }
}

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
   digitalWrite(RF_LED, HIGH);
    /**
   * O = Open Event
   * C = Close Event
   * g/G = close/OPEN cler
   * l/L = turn off/ON light
   */
    switch (payload[0])
    {
    case 'O':
        /* Open event */
        break;
    case 'C':
        /* close event */
        break;
    case 'g':
        /* close cler command */
        break;
    case 'G':
        /* open cler command */
        break;
    case 'l':
        /* turn off light */
        break;
    case 'L':
        /* turn on light */
        break;
    default:
        // Unknown
        break;
    }
    if (payload[0] == 'O')
    {
        Serial.print(F("COE:"));
        Serial.println(packet_info.tx.id);
        digitalWrite(RF_LED, HIGH);
        delay(30);
        digitalWrite(RF_LED, LOW);
        bus.reply("C", 1);
    }
    else if (payload[0] == 'C')
    {
        Serial.print(F("RES:"));
        Serial.println(packet_info.tx.id);
        digitalWrite(STATUS_LED, HIGH);
        delay(15);
        digitalWrite(STATUS_LED, LOW);
        bus.reply("C", 1);
    }
};

void error_handler(uint8_t code, uint16_t data, void *custom_pointer)
{
    if (code == PJON_CONNECTION_LOST)
    {
        Serial.print(F("Connection with device ID "));
        Serial.print(bus.packets[data].content[0], DEC);
        Serial.println(" is lost.");
    }
    if (code == PJON_PACKETS_BUFFER_FULL)
    {
        Serial.print(F("Packet buffer is full, has now a length of "));
        Serial.println(data, DEC);
        Serial.println(F("Possible wrong bus configuration!"));
        Serial.println(F("higher PJON_MAX_PACKETS if necessary."));
    }
    if (code == PJON_CONTENT_TOO_LONG)
    {
        Serial.print(F("Content is too long, length: "));
        Serial.println(data);
    }
};

void setupRF()
{
    // PJON

    bus.strategy.set_pins(RF_IN, RF_OUT);
    bus.set_error(error_handler);
    bus.set_receiver(receiver_function);
    bus.set_communication_mode(PJON_HALF_DUPLEX);

    if (CC_RF_BUS != CC_RF_MAIN)
    {
        Serial.print(F("RF: I'm the slave "));
        Serial.println(CC_RF_BUS);
        //  bus.send_repeatedly(CC_RF_MAIN, "B", 1, 5000000); // Send B to device CC_RF_MAIN every 5 seconds
    }
    else
    {
        Serial.print(F("RF: I'm the master "));
        Serial.println(CC_RF_BUS);
    }

    bus.begin();
}