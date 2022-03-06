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
/*----------------------------------------------------------------------------*/
TouchKbd::TouchKbd(void) :
  crntTouch{0}
{}
/*----------------------------------------------------------------------------*/
void TouchKbd::mainLoop(void)
{
  for (int i=0; i<MAX_NOTE; ++i){
    uint8_t sw = digitalRead(KEY_SWITCH[i]);
    if (sw == HIGH){
      digitalWrite(LED_ERR, LOW);
      if (crntTouch[i] == true){
        makeNoteEvent(60+i,false);
        crntTouch[i] = false;
      }
    }
    else {
      digitalWrite(LED_ERR, HIGH);
      if (crntTouch[i] == false){
        makeNoteEvent(60+i,true);
        crntTouch[i] = true;
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::checkTouch(uint16_t sw[])
{
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
