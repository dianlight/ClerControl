#pragma once

#include <Arduino.h>
#include <AceRoutine.h>
#include <EmonLib.h> // Include Emon Library

#include "pin.h"
#include "shared.h"

EnergyMonitor emon1;

void setupPower(){
  // Powe Monitor
  emon1.current(PWR_SENSOR, 111.1); // Start and calibrate PWR sensor for cler moviment
}

COROUTINE(powerLoop)
{
    COROUTINE_LOOP()
    {
        digitalWrite(STATUS_LED, LOW);
        double Irms = emon1.calcIrms(1480); // Calculate Irms only
//        Serial.print(F("Apperant Pwr:"));
//        Serial.print(Irms * 230.0); // Apparent power
        if(Irms > 1){ 
//            Serial.print(F(" Irms:"));
            Serial.print(analogRead(PWR_SENSOR));
            Serial.print("=");
            Serial.println(Irms); // Irms
            _currentStatus.moving=true;
            digitalWrite(STATUS_LED, HIGH);
        } else {
            _currentStatus.moving=false;
        }
        COROUTINE_DELAY(2000); // Check every seconds
    }
}

