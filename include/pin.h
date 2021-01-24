#pragma once
#include <Arduino.h>

#define CS_PIN      10 /*PB2 */  
#define RF_IN       6 /* PD6 */   
#define RF_OUT      7 /* PD7 */

#define RF_LED      analogInputToDigitalPin(PC2)  
#define STATUS_LED  analogInputToDigitalPin(PC1)
#define LAN_LED     analogInputToDigitalPin(PC0)

#define INT_SENSOR  2 /* PD2 */
#define PWR_SENSOR  A3 /* PC3  org PC5*/ 

#define BTN_WALL    3 /* PD3 */

#define CLER_CONTROL   4  /* PD4 */
#define LIGHT_CONTROL  5  /* PD5 */



