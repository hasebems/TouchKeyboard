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
  _joystick_y(0),
  _cntrlrMode(MD_PLAIN),
  _crntDpt{0},
  _crntVib{0},
  _crntSw{0},
  _uiSw{false}
{}
/*----------------------------------------------------------------------------*/
void TouchKbd::init(int tchSwNum)
{
  _touchSwNum = tchSwNum;
}
/*----------------------------------------------------------------------------*/
void TouchKbd::incCntrlrMode(void)
{
  switch(_cntrlrMode){
    case MD_PLAIN:      break;
    case MD_DEPTH_POLY: _cntrlrMode = MD_TOUCH_MONO; break;
    case MD_TOUCH_MONO: _cntrlrMode = MD_SWITCH; break;
    case MD_SWITCH:     _cntrlrMode = MD_DEPTH_POLY; break;
  }
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
    if (_cntrlrMode == MD_SWITCH){
      setMidiControlChange(19, static_cast<uint8_t>(y/8));
    }
    else {
      setMidiPitchBend(static_cast<uint8_t>(y/8));
    }
  }

#ifdef USE_PCAL9555A
  uint8_t val = 0;
  int err = pcal9555a_get_value(1,&val);
  if (err==0){
    for (int j=0; j<3; j++){
      if ((~val)&(0x80>>j)){
        if (!_uiSw[j]){
          _uiSw[j]=true;
          // on event
          if (j==1){ incCntrlrMode();}
        }
      }
      else {
        _uiSw[j]=false;
      }
    }
  }
  setAda88_5prm(_cntrlrMode, 2, 3, 4, 5);
#endif
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
          makeNoteEvent(i,false);
          _crntTouch[i] = false;
          _anti_chattering_counter[i] = 0;
        }
      }
      else {
        digitalWrite(LED_ERR, HIGH);
        if (_crntTouch[i] == false){
          makeNoteEvent(i,true);
          _crntTouch[i] = true;
          _anti_chattering_counter[i] = 0;
        }
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::changeControllerMode(CONTROLLER_MODE mode)
{
  if (mode>MD_SWITCH){mode=MD_SWITCH;}
  else if (mode<MD_PLAIN){mode=MD_PLAIN;}
  _cntrlrMode = mode;
  if (mode!=MD_PLAIN){
    setMidiProgramChange(mode-1);
  }
  int i;
  switch(mode){
    case MD_DEPTH_POLY: for (i=0;i<MAX_NOTE; i++){ _crntDpt[i]=0;} break;
    case MD_TOUCH_MONO: for (i=0;i<MAX_NOTE; i++){ _crntVib[i]=0;} break;
    case MD_SWITCH:     for (i=0;i<MAX_NOTE; i++){ _crntSw[i]=0;} break;
    default: break;
  }  
}
/*----------------------------------------------------------------------------*/
//      |01|09|00|03|  |
//    02|--+--+--+--|04|  player
//      |08|07|06|05|  |
//
//  Depth Pattern  : [4][345][0356][0679][1789][128][2] -> no touch=0,[4]=1...[2]=127
//  Vibrato Pattern: [012349][245678] -> no touch=0,[012349]=1...[245678]=127
//  Switch Pattern : [4][56][03][78][19][2] -> no touch=0,16,32,48,64,80,96
/*----------------------------------------------------------------------------*/
void TouchKbd::depth_pattern(int key)
{  //=== Depth Pattern ===
  bool depth[6]={false,false,false,false,false,false};
  const int dpt_index[MAX_ELECTRODE] = {2,4,5,1,0,1,2,3,4,3};// そのセンサがどこのdepthに位置するか

  for (int i=0; i<MAX_ELECTRODE; i++){
    if (_touchSwitch[key][i]){depth[dpt_index[i]]=true;}
  }
  int start=-1;
  int end=-1;
  for(int i=5; i>=0; --i){
    if (start==-1){
      if (depth[i]){start=end=i;}
    }
    else{
      if (depth[i]){end=i;}
    }
  }
  int midi_value = (start+end+2)*10;// 0-120
  if (midi_value>=120){midi_value=127;}
  if (_crntDpt[key]!=midi_value){
    setMidiPAT(MAIN_BOARD_OFFSET_NOTE+key,midi_value);
    _crntDpt[key] = midi_value;
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::vibrato_pattern(int key)
{  //=== Vibrato Pattern ===
  uint8_t bitPtn = 0x00;
  if (_touchSwitch[key][0] || _touchSwitch[key][1] || _touchSwitch[key][3] || _touchSwitch[key][9]){
    bitPtn = 0x02;
  }
  if (_touchSwitch[key][5] || _touchSwitch[key][6] || _touchSwitch[key][7] || _touchSwitch[key][8]){
    bitPtn = 0x01;
  }
  uint8_t vib = 0;
  if (bitPtn==0x01){vib=1;}
  else if (bitPtn==0x02){vib=2;}
  if (vib!=_crntVib[key]){
    setMidiPAT(MAIN_BOARD_OFFSET_NOTE+key,vib);
    _crntVib[key] = vib;
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::switch_pattern(int key)
{  //=== Switch Pattern ===
  uint8_t bitPtn = 0x00;
  if (_touchSwitch[key][4]){
    bitPtn = 0x01;
  }
  if (_touchSwitch[key][5] || _touchSwitch[key][6]){
    bitPtn = 0x02;
  }
  if (_touchSwitch[key][0] || _touchSwitch[key][3]){
    bitPtn = 0x04;
  }
  if (_touchSwitch[key][7] || _touchSwitch[key][8]){
    bitPtn = 0x08;
  }
  if (_touchSwitch[key][1] || _touchSwitch[key][9]){
    bitPtn = 0x10;
  }
  if (_touchSwitch[key][4]){
    bitPtn = 0x20;
  }

  uint8_t sw = 0;
  switch(bitPtn){
    default:
    case 0x00: break;
    case 0x01: sw=16; break;
    case 0x02: sw=32; break;
    case 0x04: sw=48; break;
    case 0x08: sw=64; break;
    case 0x10: sw=72; break;
    case 0x20: sw=127; break;
  }
  if (sw!=_crntSw[key]){
    setMidiPAT(MAIN_BOARD_OFFSET_NOTE+key,sw);
    _crntSw[key] = sw;
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::send_to_master(int key)  //  only sub board
{
  uint8_t valueBitPtn = 0, noteBitPtn = 0;
  for(int i=0; i<7; i++){
    if (_touchSwitch[key][i]){valueBitPtn |= (0x01<<i);}
  }

  for(int j=0; j<3; j++){
    if (_touchSwitch[key][j+7]){noteBitPtn |= (0x10<<j);}
  }
  setMidiPAT(key+noteBitPtn, valueBitPtn);
}
/*----------------------------------------------------------------------------*/
void TouchKbd::select_pattern(int key)
{
  switch(_cntrlrMode){
    default: // through
    case MD_PLAIN:      send_to_master(key); break;
    case MD_DEPTH_POLY: depth_pattern(key); break;
    case MD_TOUCH_MONO: vibrato_pattern(key); break;
    case MD_SWITCH:     switch_pattern(key); break;
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::checkTouchEach(uint8_t key, uint16_t raw_data)
{
  for (int j=0; j<10; j++){
    if ((raw_data & 0x0001) && !_touchSwitch[key][j]){
      _touchSwitch[key][j] = true;
      select_pattern(key);
    }
    else if (!(raw_data & 0x0001) && _touchSwitch[key][j]){
      _touchSwitch[key][j] = false;
      select_pattern(key);
    }
    raw_data = raw_data>>1;
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::checkTouch(uint16_t sw[])
{
  for (int i=0; i<_touchSwNum; ++i){
    uint16_t raw_data = sw[i];
    checkTouchEach(i,raw_data);
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::makeNoteEvent(int notenum, bool onoff, int vel)
{
  uint8_t ofs_note = MAIN_BOARD_OFFSET_NOTE;
  if (getControllerMode()==MD_PLAIN){
    ofs_note = SUB_BOARD_OFFSET_NOTE;
  }

  if (onoff){
    // Note On
    setMidiNoteOn(ofs_note + notenum, vel);
  }
  else {
    //  Note Off
    setMidiNoteOff(ofs_note + notenum, 0);
  }
}
