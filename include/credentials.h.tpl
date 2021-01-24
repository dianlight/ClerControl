#pragma once
/**
 * This is the crendetial file. Please configura with your data and copy to 
 * credentials.h
 */

// COMMON CONFIG

// MAIN CONFIG
#define CC_MAC              { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }
#define CC_MQTT_IP          192,168,0,240
#define CC_MQTT_ID          "CLR_CTR"
#define CC_MQTT_USER        "<user on mqtt>"
#define CC_MQTT_PASSWORD    "<super secret password>"
#define CC_RF_MAIN          45

// SATELLITE CONFIG
#define CC_RF_SATELLITE     CC_RF_MAIN + 1