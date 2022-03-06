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
#ifndef TOUCHMIDI40_H
#define TOUCHMIDI40_H

#include <Arduino.h>

constexpr uint8_t MAX_NOTE = 12;

class TouchKbd {

public:
  TouchKbd(void);
  void mainLoop(void);
  void checkTouch(uint16_t sw[]);
  void checkTouch3dev(uint16_t sw[]);
  void makeNoteEvent(int tchNum, bool onoff, int vel=127);

private:
  static const uint8_t KEY_SWITCH[MAX_NOTE];
  bool  crntTouch[MAX_NOTE];

};
#endif
