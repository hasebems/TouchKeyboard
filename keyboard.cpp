/* ========================================
 *
 *  TouchMIDI Common Platform for AVR
 *  keyboard.h
 *    description: Touch Keyboard
 *
 *  Copyright(c)2022- Masahiko Hasebe at Kigakudoh
 *  This software is released under the MIT License, see LICENSE.txt
 *
 * ========================================
 */
#include "keyboard.h"
#include "TouchMIDI_AVR_if.h"
#include "configuration.h"
#include "i2cdevice.h"

const uint8_t TouchKbd::KEY_SWITCH[MAX_NOTE] = {
  KBD_C, KBD_CS, KBD_D, KBD_DS, KBD_E, KBD_F, KBD_FS, KBD_G, KBD_GS, KBD_A, KBD_AS, KBD_B
};
const int HYSTERISIS_LIMIT = 10;
/*----------------------------------------------------------------------------*/
TouchKbd::TouchKbd(void) :
  _crntTouch{0},
  _anti_chattering_counter{0},
  _touchSwitch{false},
  _touchSwNum(0),
  _joystick_x(0),
  _joystick_y(0)
{}
/*----------------------------------------------------------------------------*/
void TouchKbd::init(int tchSwNum)
{
  _touchSwNum = tchSwNum;
}
/*----------------------------------------------------------------------------*/
void TouchKbd::periodic(void) // once 10msec
{
  for (int i=0; i<MAX_NOTE; ++i){
    if (_anti_chattering_counter[i] < MAX_CHATTERING_TIME){
      _anti_chattering_counter[i]++;
    }
  }
  int x = analogRead( JOYSTICK_X );
  int y = analogRead( JOYSTICK_Y );
  if ((x-_joystick_x>HYSTERISIS_LIMIT) || (x-_joystick_x<-HYSTERISIS_LIMIT)){
    _joystick_x = x;
    setMidiControlChange(18, static_cast<uint8_t>(x/8));
  }
  if ((y-_joystick_y>HYSTERISIS_LIMIT) || (y-_joystick_y<-HYSTERISIS_LIMIT)){
    _joystick_y = y;
    setMidiControlChange(19, static_cast<uint8_t>(y/8));
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::mainLoop(void)
{
  for (int i=0; i<MAX_NOTE; ++i){
    if (_anti_chattering_counter[i] >= MAX_CHATTERING_TIME){
      uint8_t sw = digitalRead(KEY_SWITCH[i]);
      if (sw == HIGH){
        digitalWrite(LED_ERR, LOW);
        if (_crntTouch[i] == true){
          makeNoteEvent(60+i,false);
          _crntTouch[i] = false;
          _anti_chattering_counter[i] = 0;
        }
      }
      else {
        digitalWrite(LED_ERR, HIGH);
        if (_crntTouch[i] == false){
          makeNoteEvent(60+i,true);
          _crntTouch[i] = true;
          _anti_chattering_counter[i] = 0;
        }
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::checkTouch(uint16_t sw[])
{
  for (int i=0; i<_touchSwNum; ++i){
    uint16_t raw_data = sw[i];
    for (int j=0; j<10; j++){
      if ((raw_data & 0x0001) && !_touchSwitch[i][j]){
        setMidiPolyPressure(60+i,j);
        _touchSwitch[i][j] = true;
      }
      else if (!(raw_data & 0x0001) && _touchSwitch[i][j]){
        _touchSwitch[i][j] = false;
      }
      raw_data = raw_data>>1;
    }
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::checkTouch3dev(uint16_t sw[])
{
}
/*----------------------------------------------------------------------------*/
void TouchKbd::makeNoteEvent(int notenum, bool onoff, int vel=127)
{
  if (onoff){
    // Note On
    setMidiNoteOn(notenum,vel);
  }
  else {
    //  Note Off
    setMidiNoteOff(notenum,0);
  }
}
