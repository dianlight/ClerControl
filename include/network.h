#pragma once
#include <Arduino.h>
#include <AceRoutine.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
//#include <extEEPROM.h> //https://github.com/PaoloP74/extEEPROM
#include <Wire.h>
#include <I2C_eeprom.h>
#include <avr/wdt.h>

#include "pin.h"
#include "shared.h"
#include "credentials.h"

//void (*resetptr)( void ) = 0x0000;

//extEEPROM eep(kbits_256, 1, 64, 0x50); //device size, number of devices, page size 28866
#define MEMORY_SIZE 0x8000 //total bytes can be accessed 24LC64 = 0x2000 (maximum address = 0x1FFF)
I2C_eeprom ee(0x50, MEMORY_SIZE);

byte mac[] = CC_MAC;                            // { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress server(CC_MQTT_IP /*, 168, 0, 250*/); // MQTT_SERVER
#define CLER_PULSE_TIME 500
#define OTA_CHUNK_SIZE 64                 /*  but must be a multiple of 4 */
byte OTA_HEADER[] = {'F', 'L', 'X','I','M','G',':',0x0,0x0,':'}; /*FLXIMG:<2byte falsh size>:xxxxxx */

static uint16_t ota_addr = 0;
static uint16_t ota_size = 0;

EthernetClient ethClient;
PubSubClient client(ethClient);


static const char* mqtt_ota = "clerct/mn/ota";
static const char* mqtt_otak = "clerct/mn/otak";
static const char* mqtt_gd_set = "mn_gd/set";
static const char* mqtt_gl_set = "mn_gl/set";

void callback(char *topic, byte *payload, unsigned int length)
{
    //  Serial.print(F("Message arrived ["));
    //  Serial.print(topic);
    //  Serial.println(F("]"));
    if (strstr(topic, mqtt_gd_set) != NULL)
    {
        switch (payload[0])
        {
        case 'O': // OPEN
            if (_currentStatus.closed)
            {
                digitalWrite(CLER_CONTROL, HIGH);
                delay(CLER_PULSE_TIME);
                digitalWrite(CLER_CONTROL, LOW);
            }
            break;
        case 'C': // CLOSE
            if (!_currentStatus.closed)
            {
                digitalWrite(CLER_CONTROL, HIGH);
                delay(CLER_PULSE_TIME);
                digitalWrite(CLER_CONTROL, LOW);
            }
            break;
        case 'S': // STOP
            if (_currentStatus.moving)
            {
                digitalWrite(CLER_CONTROL, HIGH);
                delay(CLER_PULSE_TIME);
                digitalWrite(CLER_CONTROL, LOW);
            }
            break;
        default:
            Serial.println(F("PL!"));
            for (unsigned int i = 0; i < length; i++)
            {
                Serial.print((char)payload[i]);
            }
            Serial.println();
            break;
        }
    }
    else if (strstr(topic, mqtt_gl_set) != NULL)
    {
        switch (payload[1])
        {
        case 'N': // ON
            digitalWrite(LIGHT_CONTROL, HIGH);
            _currentStatus.lightOn = true;
            break;
        case 'F': // OFF
            digitalWrite(LIGHT_CONTROL, LOW);
            _currentStatus.lightOn = false;
            break;
        default:
            Serial.println(F("PL!"));
            for (unsigned int i = 0; i < length; i++)
            {
                Serial.print((char)payload[i]);
            }
            Serial.println();
            break;
        }
    }
    else if (strstr(topic, "ota") != NULL)
    {
        switch (payload[0])
        {
        case 'B':{
            uint16_t fws = payload[1]+(payload[2] << 8);
            Serial.print(F("OTA:"));
            Serial.println(fws*16,HEX);
            ota_size = fws;
            ota_addr = sizeof(OTA_HEADER);
            OTA_HEADER[7]=((fws*16) >> 8) & 0xFF;
            OTA_HEADER[8]=(fws*16) & 0xFF;
            client.publish(mqtt_otak,"K");
            }
            break;
        case 'C':
            if (ota_size > 0)
            {
                Serial.print(ota_addr, 16);
                Serial.print(F(":"));
                Serial.print(payload[1]+(payload[2] << 8));
                Serial.print(F(":"));
                Serial.print(length-3);            
                //eep.write(ota_addr, &payload[3], length - 3);
                ee.writeBlock(ota_addr, &payload[3], length - 3);

                // TEST.
                byte test;
                //eep.read(ota_addr+((length-3)/2), &test, 1);
                ee.readBlock(ota_addr+((length-3)/2), &test, 1);
                Serial.print(F(":"));
                if (test == payload[3+((length-3)/2)])
                {
                    Serial.println(F("K"));
                    ota_addr += length - 3;
                    ota_size = payload[1]+(payload[2] << 8);
                    client.publish(mqtt_otak,"K");
                }
                else
                {
                    Serial.println(F("E"));
                    ota_size = 0;
                    client.publish(mqtt_otak,"E");
                }
            }
            else
            {
                Serial.println(F("C:E"));
                client.publish(mqtt_otak,"E");
            }
            break;
        case 'E':
            if (ota_size > 0)
            {
//                eep.write(0x00, OTA_HEADER, sizeof(OTA_HEADER));
                ee.writeBlock(0x00, OTA_HEADER, sizeof(OTA_HEADER));
                // Verify
                byte test[sizeof(OTA_HEADER)];                
//                eep.read(0x00, test, sizeof(OTA_HEADER));
                ee.readBlock(0x00, test, sizeof(OTA_HEADER));
                if( memcmp(test,OTA_HEADER,sizeof(OTA_HEADER)) == 0 ){
                    Serial.println(F("Reboot"));
                    client.publish(mqtt_otak,"K");
                    // Do AVR reboot with watchdog!
                    hwReboot();
                } else {
                    Serial.print("!");
                    Serial.print((char *)test);
                    client.publish(mqtt_otak,"E");
                }



            }
            break;
        }
    }
    else
    {
        Serial.print(F("!tpc:"));
        Serial.println(topic);
    }
}

void setupNetwork()
{
    SPI.begin();
//    delay(1000);
    Ethernet.init(CS_PIN);
//    delay(1000);
    Ethernet.begin(mac);
    Serial.println(Ethernet.localIP());
    Serial.flush();

    client.setServer(server, 1883);
    client.setCallback(callback);

/*
    uint8_t eepStatus = eep.begin(eep.twiClock400kHz); //go fast!
    if (eepStatus)
    {
        Serial.print(F("eEPR!"));
        Serial.println(eepStatus);
    }
    dump(0x00,3);
    eep.write(0x00,0x10);
    eep.write(0x01,0x20);
    eep.write(0x02,0x30);
*/    

    ee.begin();
    if (! ee.isConnected())
    {
        Serial.println("eEPR!");
    } else {
        int size = ee.determineSize(false);  
        Serial.print(size);Serial.println(F("Kb"));
        Serial.print((char)ee.readByte(0x00)); // F
        Serial.print((char)ee.readByte(0x01)); // L
        Serial.print((char)ee.readByte(0x02)); // X 
        Serial.print((char)ee.readByte(0x03)); // I 
        Serial.print((char)ee.readByte(0x04)); // M
        Serial.print((char)ee.readByte(0x05)); // G
        Serial.print((char)ee.readByte(0x06)); // :
        Serial.print(ee.readByte(0x07),16); // 0x
        Serial.print(ee.readByte(0x08),16); // 0x
        Serial.print((char)ee.readByte(0x09)); // :
        Serial.print(ee.readByte(0x0A),16); // 0x
        Serial.println(ee.readByte(0x0B),16); // 0x
    }

    delay(1000);
}

bool reconnect()
{
    Serial.print(F("MQTT."));
    // Attempt to connect
    if (client.connect(CC_MQTT_ID, CC_MQTT_USER, CC_MQTT_PASSWORD))
    {
        client.subscribe(mqtt_ota);

        Serial.println(F(":K"));
        client.publish("homeassistant/cover/clerct/mn_gd/config",
                       "{"
                       "\"~\":\"clerct/mn_gd\","
                       "\"name\":\"mn_gd\","
                       "\"unique_id\":\"mn_gd\","
                       "\"dev_cla\":\"garage\"," //      open | opening | closed | closing
                       "\"stat_t\":\"~/state\"," //      OPEN | CLOSE | STOP
                       "\"cmd_t\":\"~/set\""     //        "\"value_template\":\"{{value.x}}\""
                       "}");
        client.subscribe("clerct/mn_gd/set");
        client.publish("homeassistant/light/clerct/mn_gl/config",
                       "{"
                       "\"~\":\"clerct/mn_gl\","
                       "\"name\":\"mn_gl\","
                       "\"unique_id\":\"mn_gl\"," // on|off
                       "\"stat_t\":\"~/state\","    // ON|OFF
                       "\"cmd_t\":\"~/set\""
                       "}");
        client.subscribe("clerct/mn_gl/set");
        return true;
    }
    else
    {
        Serial.print(F("!"));
        Serial.print(client.state());
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
            Serial.println(F("!HWE"));
            COROUTINE_END();
        }

        COROUTINE_AWAIT(Ethernet.linkStatus() == LinkON);
        if (!BEGIN_NETWORK)
        {
            if (Ethernet.begin(mac) == 0)
            {
                Serial.println(F("!DHCP"));
                COROUTINE_DELAY(5000); // Wait 5sec before retry
            }
            else
            {
                BEGIN_NETWORK = true;
            }
        }
        else
        {
            switch (Ethernet.maintain())
            {
            case 1:
                //renewed fail
                Serial.println(F("!rnw"));
                break;
            case 2:
                //renewed success
                //                    Serial.println(F("Renewed success"));
                //print your local IP address:
                //                    Serial.print(F("My IP address: "));
                //                    Serial.println(Ethernet.localIP());
                break;
            case 3:
                //rebind fail
                Serial.println(F("!reb"));
                break;
            case 4:
                //rebind success
                //                    Serial.println(F("Rebind success"));
                //print your local IP address:
                //                    Serial.print(F("My IP address: "));
                //                    Serial.println(Ethernet.localIP());
                break;
            default:
                //nothing happened
                break;
            }
            if (!client.connected())
            {
                if (!reconnect())
                    COROUTINE_DELAY(5000);
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
        COROUTINE_CHANNEL_READ(mqttChannel, message);
        if (client.connected())
        {
            client.publish(message.topic, message.json);
            _currentStatus.lastAnnunced = millis();
        }
        else
        {
            Serial.print(F("Msg!"));
        }
        Serial.println(message.topic);
        Serial.println(message.json);
    }
}