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
#include "configuration.h"
#include "i2cdevice.h"

constexpr uint8_t MAX_NOTE = 12;
constexpr uint8_t MAX_CHATTERING_TIME = 3;  // *10msec
constexpr uint8_t MAX_ELECTRODE = 10;

enum CONTROLLER_MODE {
  MD_PLAIN = 0,       //  Sub Board
  MD_DEPTH_POLY = 1,
  MD_TOUCH_MONO = 2,
  MD_SWITCH = 3
};

class TouchKbd {

public:
  TouchKbd(void);
  void init(int tchSwNum);
  void incCntrlrMode(void);
  void periodic(void);
  void mainLoop(void);
  void checkTouch(uint16_t sw[]);
  void checkTouch3dev(uint16_t sw[]);
  void makeNoteEvent(int tchNum, bool onoff, int vel=127);

  void changeControllerMode(CONTROLLER_MODE mode);
  CONTROLLER_MODE getControllerMode(void) const {return _cntrlrMode;}

private:
  void depth_pattern(int sw);
  void vibrato_pattern(int sw);
  void switch_pattern(int key);
  void select_pattern(int sw);
  
  static const uint8_t KEY_SWITCH[MAX_NOTE];
  bool      _crntTouch[MAX_NOTE];
  uint8_t   _anti_chattering_counter[MAX_NOTE];

  bool      _touchSwitch[MAX_DEVICE_MBR3110][MAX_ELECTRODE];
  int       _touchSwNum;
  int       _joystick_x;
  int       _joystick_y;
  CONTROLLER_MODE _cntrlrMode;

  uint8_t   _crntDpt[MAX_NOTE];
  uint8_t   _crntVib[MAX_NOTE];
  uint8_t   _crntSw[MAX_NOTE];

  bool      _uiSw[3];

};
#endif
