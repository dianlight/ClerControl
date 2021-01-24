#pragma once
#include <Arduino.h>
#include <AceRoutine.h>

#include "pin.h"
#include "shared.h"

#define CLICK_SPEED         500
#define LONG_PRESS_MOL        2

static uint8_t clicks,release, longpress;

void interruprBtnHandler()
{
  int c = digitalRead(BTN_WALL);
  clicks += c==LOW?1:0;
  release += c==HIGH?1:0;
}

COROUTINE(wallLoop)
{
    COROUTINE_LOOP()
    {
        if(longpress > LONG_PRESS_MOL){
            _currentStatus.btnStatus = BtnStatus::LONG_PRESS;
        } else if ( clicks >= 3) {
            _currentStatus.btnStatus = BtnStatus::TRIPLE_CLICK;
        } else if ( clicks >= 2) {
            _currentStatus.btnStatus = BtnStatus::DOUBLE_CLICK;
        } else if ( clicks == 1){
            _currentStatus.btnStatus = BtnStatus::CLICK;
        } 
        if(clicks != 0)clicks--;
        if(release !=0)release--;
        if(clicks > release)longpress++;        
        else if(longpress != 0) longpress--;
        COROUTINE_DELAY(CLICK_SPEED); // Double click speed. 2 in 500ms
    }
}

void setupWall(){
  pinMode(BTN_WALL,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_WALL), interruprBtnHandler, CHANGE);
  clicks = 0; release = 0; longpress = 0;
  _currentStatus.btnStatus = BtnStatus::NONE;
}