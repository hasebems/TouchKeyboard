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
  _crntSw{0},
  _uiSw{false},
  _one_board(true),
  _sub_board(true),
  _note_on_state(false),
  _target_pitch(0),
  _current_pitch(0){}
/*----------------------------------------------------------------------------*/
void TouchKbd::init(int tchSwNum, bool oneb, bool subb)
{
  _touchSwNum = tchSwNum;
  _one_board = oneb;
  _sub_board = subb;
  setMidiControlChange(120,0);  // All Sound Off

  if (_one_board){ setAda88_5prm(_cntrlrMode, 1, 1, 1, 1);}
  else { setAda88_5prm(_cntrlrMode, 2, 2, 2, 2);}
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
void TouchKbd::check_ui_sw(void)
{
#ifdef USE_PCAL9555A
  uint8_t val = 0;
  const int err = pcal9555a_get_value(1,&val);
  if (err!=0){return;}
  const CONTROLLER_MODE cmd[3] = {MD_DEPTH_POLY,MD_TOUCH_MONO,MD_SWITCH};

  for (int j=0; j<3; j++){
    if ((~val)&(0x80>>j)){
      if (!_uiSw[j]){
        _uiSw[j]=true;
        changeControllerMode(cmd[j]); // sw on event
      }
    }
    else {
      _uiSw[j]=false;
    }
  }
#endif
}
/*----------------------------------------------------------------------------*/
void TouchKbd::periodic(void) // once 10msec
{
  for (int i=0; i<MAX_NOTE; ++i){
    if (_anti_chattering_counter[i] < MAX_CHATTERING_TIME){
      _anti_chattering_counter[i]++;
    }
  }

  //  Joystick
  int x = analogRead( JOYSTICK_X ); //  0-1023
  int y = analogRead( JOYSTICK_Y );
  if ((x-_joystick_x>HYSTERISIS_LIMIT) || (x-_joystick_x<-HYSTERISIS_LIMIT)){
    _joystick_x = x;
    setMidiControlChange(18, static_cast<uint8_t>(x/8));  //  0-127
    switch(_cntrlrMode){
      case MD_DEPTH_POLY: setMidiPitchBend(((x-512)*27)/10); break;
      case MD_SWITCH:     setMidiControlChange(18, static_cast<uint8_t>(x/8)); break;
      default: break;
    }
  }
  if ((y-_joystick_y>HYSTERISIS_LIMIT) || (y-_joystick_y<-HYSTERISIS_LIMIT)){
    _joystick_y = y;
    switch(_cntrlrMode){
      case MD_TOUCH_MONO: setMidiControlChange(1, static_cast<uint8_t>(y/8)); break;
      case MD_SWITCH:     setMidiControlChange(19, static_cast<uint8_t>(y/8)); break;
      default: break;
    }
  }

  //  3 mode change switch
  check_ui_sw();

  //  display Ada88
  int max=0;
  for (int i=0; i<_touchSwNum; ++i){
    if (max<_crntDpt[i]){max=_crntDpt[i];}
  }
  setAda88_5prm(_cntrlrMode, x/128, y/128, max/16, max/16);
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
    setMidiControlChange(120,0);  //  All Sound Off
    setMidiProgramChange(mode+15);
  }
  int i;
  switch(mode){
    case MD_DEPTH_POLY: for (i=0;i<MAX_NOTE; i++){ _crntDpt[i]=0;} break;
    case MD_TOUCH_MONO:
      _note_on_state = false;
      _target_pitch = _current_pitch = 0;
      break;
    case MD_SWITCH:     for (i=0;i<MAX_NOTE; i++){ _crntSw[i]=0;} break;
    default: break;
  }
  if (_one_board){ setAda88_5prm(_cntrlrMode, 1, 1, 1, 1);}
  else { setAda88_5prm(_cntrlrMode, 2, 2, 2, 2);}
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
void TouchKbd::pitch_pattern(void)
{  //=== Pitch Pattern ===
  uint8_t bitPtn = 0;
  int highest_key = -1;
  bool  onoff = false;
  for (int key=_touchSwNum-1; key>=0; --key){
    if (_touchSwitch[key][0] || _touchSwitch[key][1] || _touchSwitch[key][3] || _touchSwitch[key][9]){
      bitPtn |= 0x02;
    }
    if (_touchSwitch[key][5] || _touchSwitch[key][6] || _touchSwitch[key][7] || _touchSwitch[key][8]){
      bitPtn |= 0x01;
    }
    if (_touchSwitch[key][2] || _touchSwitch[key][4]){
      bitPtn |= 0x10;
    }
    if (bitPtn != 0){onoff = true; highest_key = key; break;}
  }

  if (onoff){
    const int SEMI_NOTE_PB = 682;
    int pit = 0;
    if (bitPtn>=0x03){pit=SEMI_NOTE_PB*highest_key;}
    else if (bitPtn == 0x02){pit=SEMI_NOTE_PB*highest_key+SEMI_NOTE_PB/2;}
    else if (bitPtn == 0x01){pit=SEMI_NOTE_PB*highest_key-SEMI_NOTE_PB/2;}
    _target_pitch = pit-SEMI_NOTE_PB*12;
  }

  if (!_note_on_state){
    if (onoff){
      //  Note On
      setMidiPitchBend(_current_pitch);
      setMidiNoteOn(0x48, 127);
      _note_on_state = true;
    }
  }
  else {
    if (!onoff){
      //  Note Off
      setMidiNoteOff(0x48, 0);
      _note_on_state = false;
    }
    else {
      int last_pitch = _current_pitch;
      int diff = _target_pitch - _current_pitch;
      if (diff > 100){ _current_pitch += 100;}
      else if (diff < -100){ _current_pitch -= 100;}
      else { _current_pitch = _target_pitch;}
      if (last_pitch != _current_pitch){
        if (_current_pitch<-8192){ _current_pitch = -8192;}
        else if (_current_pitch>8191){ _current_pitch = 8191;}
        setMidiPitchBend(_current_pitch);
      }
    }
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
void TouchKbd::check_touch_ev(uint8_t key, int touch, bool onoff)
{
  switch(_cntrlrMode){
    case MD_DEPTH_POLY: depth_pattern(key); break;
    case MD_TOUCH_MONO: break;
    case MD_SWITCH:     depth_pattern(key); break;
    case MD_PLAIN:      setMidiPAT(key, onoff? 0x40|touch:touch); break;
    default: break;
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::check_touch_each(uint8_t key, uint16_t raw_data)
{
  for (int j=0; j<MAX_ELECTRODE; j++){
    if ((raw_data & 0x0001) && !_touchSwitch[key][j]){
      _touchSwitch[key][j] = true;
      check_touch_ev(key,j,true);
    }
    else if (!(raw_data & 0x0001) && _touchSwitch[key][j]){
      _touchSwitch[key][j] = false;
      check_touch_ev(key,j,false);
    }
    raw_data = raw_data>>1;
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::check_touch(uint16_t sw[]) //  once a 10msec
{
  for (uint8_t i=0; i<MAX_NOTE; ++i){
    check_touch_each(i,sw[i]);
  }

  if (_cntrlrMode == MD_TOUCH_MONO){
    pitch_pattern();
  }
}
/*----------------------------------------------------------------------------*/
void TouchKbd::makeNoteEvent(int notenum, bool onoff, int vel)
{
  uint8_t ofs_note = MAIN_BOARD_OFFSET_NOTE;

  switch(getControllerMode()){
    case MD_TOUCH_MONO:
      if (onoff){
        // Vibrato On
        setMidiControlChange(1,32);
      }
      else {
        //  Vibrato Off
        setMidiControlChange(1,0);
      }
      break;
    case MD_PLAIN:
      ofs_note = SUB_BOARD_OFFSET_NOTE; //  fall through
    default:
      if (onoff){
        // Note On
        setMidiNoteOn(ofs_note + notenum, vel);
      }
      else {
        //  Note Off
        setMidiNoteOff(ofs_note + notenum, 0);
      }
      break;
  }
}