#include <Arduino.h>
#include "credentials.h"
#include "pin.h"

#include "network.h"
//#include "rf.h"
#include "power.h"
#include "wall.h"
//#include "test_eeprom.h"

#include "shared.h"

ClrStatus _currentStatus;
ClrStatus _lastKnowStatus;

void interruprHandler()
{
  _currentStatus.closed = digitalRead(INT_SENSOR) == LOW;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("$"));
  Serial.println(MCUSR,BIN);
  Serial.flush();

  // PIN CONFIG
  pinMode(LAN_LED, OUTPUT);
  digitalWrite(LAN_LED, HIGH); // Initialize LAN_LED
  pinMode(RF_LED, OUTPUT);
  digitalWrite(RF_LED, HIGH); // Initialize LED_RF
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH); // Initialize LED_RF
  delay(1000);
  pinMode(INT_SENSOR, INPUT_PULLUP);
  _currentStatus.closed = digitalRead(INT_SENSOR) == LOW;
  _lastKnowStatus.closed = _currentStatus.closed;
  attachInterrupt(digitalPinToInterrupt(INT_SENSOR), interruprHandler, CHANGE);
  
  pinMode(CLER_CONTROL,OUTPUT);
  pinMode(LIGHT_CONTROL,OUTPUT);
  _currentStatus.lightOn = false;
  _lastKnowStatus.lightOn = true;

  setupNetwork();
  //setupRF();
  setupPower();
  setupWall();

  // End Boot
  delay(1000);
  digitalWrite(LAN_LED, LOW);    // Initialize LAN_LED
  digitalWrite(RF_LED, LOW);     // Initialize LAN_LED
  digitalWrite(STATUS_LED, LOW); // Initialize LAN_LED
  
  ace_routine::CoroutineScheduler::setup();
}

void loop()
{
  ace_routine::CoroutineScheduler::loop();
}

/**
 * Logic 
 */

COROUTINE(logicLoop)
{

    ClrStatus snapshot;

    COROUTINE_LOOP()
    {
        // Save the status for working.
        long milli = millis();
        memcpy(&snapshot,&_currentStatus,sizeof(ClrStatus));
        digitalWrite(STATUS_LED, HIGH);
        // Debug
//        Serial.print(snapshot.closed);Serial.print(F("|"));Serial.println(_lastKnowStatus.closed);
//        Serial.print(snapshot.moving);Serial.print(F("@"));Serial.println(_lastKnowStatus.moving);
//        Serial.print(snapshot.lightOn);Serial.print(F("$"));Serial.println(_lastKnowStatus.lightOn);
        // Cler Status
        if(snapshot.moving && !_lastKnowStatus.moving && _lastKnowStatus.closed){
            // Cler Opening
            MqttMessage message = {
              "clerct/mn_gd/state",
              "opening"
            };
            _lastKnowStatus.moving = true;
            _lastKnowStatus.lastAnnunced = milli;

            COROUTINE_CHANNEL_WRITE(mqttChannel,message);
        } else if(snapshot.moving && !_lastKnowStatus.moving && !_lastKnowStatus.closed){
            // Cler Closing
            MqttMessage message = {
              "clerct/mn_gd/state",
              "closing"
            };
            _lastKnowStatus.moving = true;
            _lastKnowStatus.lastAnnunced = milli;

            COROUTINE_CHANNEL_WRITE(mqttChannel,message);
        } else if(!snapshot.moving && (snapshot.closed != _lastKnowStatus.closed || milli-_lastKnowStatus.lastAnnunced > 30000L)) {
            // Cler status is _lastKnowStatus.closed.
//            Serial.println(snapshot.closed);
            MqttMessage message = {
              "clerct/mn_gd/state",
              snapshot.closed?"closed":"open"
            };
            _lastKnowStatus.moving = false;
            _lastKnowStatus.closed = snapshot.closed;
            _lastKnowStatus.lastAnnunced = milli;

            COROUTINE_CHANNEL_WRITE(mqttChannel,message);
        } else if(snapshot.lightOn != _lastKnowStatus.lightOn || milli-_lastKnowStatus.lastLightAnnunced > 30000L){
          
            MqttMessage message = {
              "clerct/mn_gl/state",
              snapshot.lightOn?"ON":"OFF"
            };
            _lastKnowStatus.lightOn = snapshot.lightOn;
            _lastKnowStatus.lastLightAnnunced = milli;

            COROUTINE_CHANNEL_WRITE(mqttChannel,message);
        }
        // Funcions
        switch (snapshot.btnStatus)
        {
        case CLICK:
          /* code */
          break;
        case DOUBLE_CLICK:
          /* code */
          break;
        case TRIPLE_CLICK:
          /* code */
          break;        
        case LONG_PRESS:
          /* code */
          break;        
        case NONE:
        default:
          break;
        }
        digitalWrite(STATUS_LED, LOW);
        COROUTINE_DELAY(1000);
    }
} 