#pragma once
#include <Arduino.h>
#include <AceRoutine.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "pin.h"
#include "shared.h"
#include "credentials.h"


byte mac[] = CC_MAC; // { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress server(CC_MQTT_IP/*, 168, 0, 250*/);    // MQTT_SERVER
#define CLER_PULSE_TIME 500


EthernetClient ethClient;
PubSubClient client(ethClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.println(F("]"));
  if( strstr(topic,"main_gd/set") != NULL){
      switch (payload[0])
      {
      case 'O': // OPEN
          if(_currentStatus.closed){
            digitalWrite(CLER_CONTROL,HIGH);
            delay(CLER_PULSE_TIME);
            digitalWrite(CLER_CONTROL,LOW);
          }
          break;
      case 'C': // CLOSE
          if(!_currentStatus.closed){
            digitalWrite(CLER_CONTROL,HIGH);
            delay(CLER_PULSE_TIME);
            digitalWrite(CLER_CONTROL,LOW);
          }
          break;
      case 'S': // STOP
          if(_currentStatus.moving){
            digitalWrite(CLER_CONTROL,HIGH);
            delay(CLER_PULSE_TIME);
            digitalWrite(CLER_CONTROL,LOW);
          }
          break;      
      default:
            Serial.println("Unknown GD Payload!");
            for (unsigned int i=0;i<length;i++) {
                Serial.print((char)payload[i]);
            }
            Serial.println();
          break;
      }
  } else if( strstr(topic,"main_gl/set") != NULL){
      switch (payload[1])
      {
      case 'N': // ON
          digitalWrite(LIGHT_CONTROL,HIGH);
          _currentStatus.lightOn=true;
          break;
      case 'F': // OFF
          digitalWrite(LIGHT_CONTROL,LOW);
          _currentStatus.lightOn=false;
          break;
      default:
            Serial.println(F("Unknown GL Payload!"));
            for (unsigned int i=0;i<length;i++) {
                Serial.print((char)payload[i]);
            }
            Serial.println();
          break;
      }
  } else {
      Serial.print(F("Unknown topic:"));
      Serial.println(topic);
  }
}


void setupNetwork()
{    
    SPI.begin();
    delay(1000);
    Ethernet.init(CS_PIN);
    delay(1000);
    Serial.print(F("Start Network...."));
    Ethernet.begin(mac);
    Serial.print(Ethernet.localIP());
    Serial.println(F(" Done."));
    Serial.flush();

    client.setServer(server, 1883);
    client.setCallback(callback);  
    delay(1000);  
}

bool reconnect() {
    Serial.print(F("MQTT connection..."));
    // Attempt to connect
    if (client.connect(CC_MQTT_ID,CC_MQTT_USER,CC_MQTT_PASSWORD)) {
      Serial.println(F("connected"));
      client.publish("homeassistant/cover/clercontrol/main_gd/config",
      "{" \
        "\"~\":\"clercontrol/main_gd\"," \
        "\"name\":\"main_gd\"," \
        "\"unique_id\":\"main_gd\"," \
        "\"dev_cla\":\"garage\"," \
//      open | opening | closed | closing
        "\"stat_t\":\"~/state\"," \
//      OPEN | CLOSE | STOP        
        "\"cmd_t\":\"~/set\"" \
//        "\"value_template\":\"{{value.x}}\""
      "}"
      );
      client.subscribe("clercontrol/main_gd/set");
      client.publish("homeassistant/light/clercontrol/main_gl/config",
      "{" \
        "\"~\":\"clercontrol/main_gl\"," \
        "\"name\":\"main_gl\"," \
        "\"unique_id\":\"main_gl\"," \
        // on|off
        "\"stat_t\":\"~/state\"," \
        // ON|OFF
        "\"cmd_t\":\"~/set\"" \
      "}"
      );
      client.subscribe("clercontrol/main_gl/set");
      return true;
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(F(" try again in 5 seconds"));
      return false;
    }
}


static bool BEGIN_NETWORK = false;

COROUTINE(networkLoop)
{
    COROUTINE_LOOP()
    {
        digitalWrite(LAN_LED, LOW);
        // Check HW
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            Serial.println(F("Ethernet shield was not found.  Sorry, can't run without hardware. :("));
            COROUTINE_END();
        }

        COROUTINE_AWAIT(Ethernet.linkStatus() == LinkON);
            if (!BEGIN_NETWORK)
            {
                if (Ethernet.begin(mac) == 0)
                {
                    Serial.println(F("Failed to configure using DHCP"));
                    COROUTINE_DELAY(5000); // Wait 5sec before retry
                } else {
                    BEGIN_NETWORK = true;
                }
            }
            else
            {
                switch (Ethernet.maintain())
                {
                case 1:
                    //renewed fail
                    Serial.println(F("Error: renewed fail"));
                    break;
                case 2:
                    //renewed success
                    Serial.println(F("Renewed success"));
                    //print your local IP address:
                    Serial.print(F("My IP address: "));
                    Serial.println(Ethernet.localIP());
                    break;
                case 3:
                    //rebind fail
                    Serial.println(F("Error: rebind fail"));
                    break;
                case 4:
                    //rebind success
                    Serial.println(F("Rebind success"));
                    //print your local IP address:
                    Serial.print(F("My IP address: "));
                    Serial.println(Ethernet.localIP());
                    break;
                default:
                    //nothing happened
                    break;
                }
                if (!client.connected()) {
                    if(!reconnect())COROUTINE_DELAY(5000);
                }
                client.loop();                
                digitalWrite(LAN_LED, HIGH);
            }
            COROUTINE_DELAY(500);
    }
}

COROUTINE(mqttLoop)
{
    COROUTINE_LOOP()
    {
        MqttMessage message;
        COROUTINE_CHANNEL_READ(mqttChannel,message);
        if (client.connected()) {
            client.publish(message.topic,message.json);
            _currentStatus.lastAnnunced = millis();
        } else {
            Serial.print(F("Losing Message.."));
        }
        Serial.println(message.topic);
        Serial.println(message.json);
    }
}