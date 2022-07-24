/* ========================================
 *
 *  TouchMIDI_AVR_common_32U4
 *    description: Main Loop
 *    for Arduino Leonardo
 *
 *  Copyright(c)2021- Masahiko Hasebe at Kigakudoh
 *  This software is released under the MIT License, see LICENSE.txt
 *
 * ========================================
 */
#include  <MsTimer2.h>
#include  <Adafruit_NeoPixel.h>
#include  <MIDI.h>
#include  <MIDIUSB.h>

#include  "configuration.h"
#include  "TouchMIDI_AVR_if.h"

#include  "i2cdevice.h"
#include  "keyboard.h"

#ifdef __AVR__
  #include <avr/power.h>
#endif

/*----------------------------------------------------------------------------*/
//
//     Global Variables
//
/*----------------------------------------------------------------------------*/
Adafruit_NeoPixel led = Adafruit_NeoPixel(MAX_LED, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
MIDI_CREATE_DEFAULT_INSTANCE();

/*----------------------------------------------------------------------------*/
int maxCapSenseDevice;
bool availableEachDevice[MAX_DEVICE_MBR3110];

GlobalTimer gt;
static TouchKbd tchkbd;

/*----------------------------------------------------------------------------*/
void flash()
{
  gt.incGlobalTime();
}
/*----------------------------------------------------------------------------*/
void setup()
{
  int err;
  int i;

  //  Initialize Hardware
  wireBegin();
  MIDI.setHandleNoteOff(handlerNoteOff);
  MIDI.setHandleNoteOn(handlerNoteOn);
  MIDI.setHandleControlChange(handlerCC);
  MIDI.setHandleAfterTouchPoly(handlerPAT);
  MIDI.begin();
  MIDI.turnThruOff();

#ifdef USE_ADA88
  ada88_init();
  ada88_write(0);
#endif

  bool sub_board = false;
#ifdef USE_PCAL9555A
  err = pcal9555a_init();
  if (err!=0){sub_board=true;}
#endif

  //  Read Jumper Pin Setting
  pinMode(KBD_C, INPUT); 
  pinMode(KBD_CS, INPUT);
  pinMode(KBD_D, INPUT);
  pinMode(KBD_DS, INPUT);
  pinMode(KBD_E, INPUT);
  pinMode(KBD_F, INPUT);
  pinMode(KBD_FS, INPUT);
  pinMode(KBD_G, INPUT);
  pinMode(KBD_GS, INPUT);
  pinMode(KBD_A, INPUT);
  pinMode(KBD_AS, INPUT);
  pinMode(KBD_B, INPUT);

  pinMode(LED_ERR, OUTPUT);
  digitalWrite(LED_ERR, HIGH);
  //pinMode(LED1, OUTPUT);
  //digitalWrite(LED1, HIGH);
  //pinMode(LED2, OUTPUT);
  //digitalWrite(LED2, HIGH);

  for (i=0; i<MAX_DEVICE_MBR3110; ++i){
    availableEachDevice[i] = true;
  }
  maxCapSenseDevice = MAX_DEVICE_MBR3110;

  uint16_t errNum = 0;
#if SETUP_MODE // CapSense Setup Mode
  for (i=0; i<maxCapSenseDevice; ++i){
    err = MBR3110_setup(i);
    if (err){
      availableEachDevice[i] = false;
      digitalWrite(LED_ERR, HIGH);
      errNum += 0x0001<<i;
    }
  }
  setAda88_Bit(errNum);
  delay(2000);          // if something wrong, 2sec LED_ERR on
  for (i=0; i<3; i++){  // when finished, flash 3times.
    digitalWrite(LED_ERR, LOW);
    delay(100);
    digitalWrite(LED_ERR, HIGH);
    delay(100);
  }
  digitalWrite(LED_ERR, LOW);
#endif

  //  Normal Mode
  MBR3110_resetAll(maxCapSenseDevice);
  errNum = 0;
  for (i=0; i<maxCapSenseDevice; ++i){
    if (availableEachDevice[i]){
      err = MBR3110_init(i);
      if (err){
        availableEachDevice[i] = false;
        errNum += 0x0001<<i;
      }
    }
  }
  if (errNum){
    //  if err, stop 5sec.
    digitalWrite(LED_ERR, HIGH);
    setAda88_Bit(errNum);
    delay(5000);  //  5sec LED_ERR on
    digitalWrite(LED_ERR, LOW);
    ada88_write(0);
  }

  //  Init Touch Keyboard
  if (!sub_board){
    //  Main Board
    tchkbd.init(maxCapSenseDevice*2);
    tchkbd.changeControllerMode(MD_DEPTH_POLY);
  }
  else {
    //  Sub Board
    tchkbd.init(maxCapSenseDevice);
  }

  //  Set NeoPixel Library 
  led.begin();
  led.show(); // Initialize all pixels to 'off'

  //  Set Interrupt
  MsTimer2::set(10, flash);     // 10ms Interval Timer Interrupt
  MsTimer2::start();

#ifdef USE_MPRLF0001PG
  mprlf0001pg_init();
#endif
}
/*----------------------------------------------------------------------------*/
void loop()
{  
  //  Global Timer 
  generateTimer();

  //  Application
  tchkbd.mainLoop();

  //  MIDI Receive
  receiveMidi();

  if ( gt.timer10msecEvent() ){
    //  Touch Sensor
    uint16_t sw[MAX_DEVICE_MBR3110] = {0};
 #ifdef USE_CY8CMBR3110
    int errNum = 0;
    for (int i=0; i<maxCapSenseDevice; ++i){
      if (availableEachDevice[i] == true){
        uint8_t swtmp[2] = {0};
        int err = MBR3110_readTouchSw(swtmp,i);
        if (err){
          errNum += 0x01<<i;
        }
        sw[i] = (((uint16_t)swtmp[1])<<8) + swtmp[0];
        //setAda88_Number(sw[i]*10);
      }
    }
    if (errNum){
      setAda88_Number(errNum*10);
      digitalWrite(LED_ERR, HIGH);
    }
    else {
      digitalWrite(LED_ERR, LOW);
      tchkbd.checkTouch(sw);
    }
 #endif
    //  Processing
    //  do something
    tchkbd.periodic();
  }
}
/*----------------------------------------------------------------------------*/
//
//     Global Timer
//
/*----------------------------------------------------------------------------*/
void generateTimer( void )
{
  uint32_t diff = gt.readGlobalTimeAndClear();

  gt.clearAllTimerEvent();
  gt.updateTimer(diff);
  //setAda88_Number(diff);

  if ( gt.timer100msecEvent() == true ){
    //  heatbeat for Debug
    //(gt.timer100ms() & 0x0002)? digitalWrite(LED2, HIGH):digitalWrite(LED2, LOW);
  }
}
/*----------------------------------------------------------------------------*/
//
//     MIDI Out
//
/*----------------------------------------------------------------------------*/
void setMidiNoteOn( uint8_t note, uint8_t vel )
{
  MIDI.sendNoteOn( note, vel, 1 );
  midiEventPacket_t event = {0x09, 0x90, note, vel};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
/*----------------------------------------------------------------------------*/
void setMidiNoteOff( uint8_t note, uint8_t vel )
{
  MIDI.sendNoteOff( note, vel, 1 );
  midiEventPacket_t event = {0x09, 0x90, note, 0};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
/*----------------------------------------------------------------------------*/
void setMidiProgramChange( uint8_t number )
{
  MIDI.sendProgramChange( number, 0);
  midiEventPacket_t event = {0x0c, 0xc0, number, 0};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
/*----------------------------------------------------------------------------*/
void setMidiControlChange( uint8_t controller, uint8_t value )
{
  MIDI.sendControlChange( controller, value, 0 );
  midiEventPacket_t event = {0x0b, 0xb0, controller, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
/*----------------------------------------------------------------------------*/
void setMidiPAT( uint8_t note, uint8_t value )
{
  MIDI.sendPolyPressure( note, value, 0 );
  midiEventPacket_t event = {0x0a, 0xa0, note, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
/*----------------------------------------------------------------------------*/
void setMidiPitchBend(int bend)
{
  MIDI.sendPitchBend(bend, 0);
  bend += 0x4000;
  midiEventPacket_t event = {0x0e, 0xe0, static_cast<uint8_t>(bend & 0x007f), static_cast<uint8_t>((bend & 0x3f80)>>7)};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}
/*----------------------------------------------------------------------------*/
//      Serial MIDI In
/*----------------------------------------------------------------------------*/
void receiveMidi( void ){
  MIDI.read();
  // midiEventPacket_t rx = MIDIUSB.read();
}
/*----------------------------------------------------------------------------*/
void handlerNoteOn( byte channel , byte number , byte value ){
  if (channel == 0){
    setMidiNoteOn(number, value);
  }
}
/*----------------------------------------------------------------------------*/
void handlerNoteOff( byte channel , byte number , byte value ){
  if (channel == 0){
    setMidiNoteOff(number, value);
  }
}
/*----------------------------------------------------------------------------*/
void handlerCC( byte channel , byte number , byte value )
{
  if (channel == 0){
    setMidiControlChange(number, value);
  }
}
/*----------------------------------------------------------------------------*/
void handlerPAT( byte channel , byte note , byte pressure )
{
  if (channel == 0){  //  for main board
    uint16_t noteBitPtn = note & 0x0070;
    uint16_t prsBitPtn = pressure & 0x007f;
    uint8_t key = note & 0x0f;

    uint16_t newTouch = (noteBitPtn<<3) | prsBitPtn;
    tchkbd.checkTouchEach(key+12, newTouch);  //  1octave above
  }
}
/*----------------------------------------------------------------------------*/
//void midiClock( uint8_t msg )
//{
//  if ( isMasterBoard == false ){
//    MIDI.sendControlChange( midi::GeneralPurposeController1, msg, 1 );
//  }
//}
/*----------------------------------------------------------------------------*/
//
//     Hardware Access Functions
//
/*----------------------------------------------------------------------------*/
void setAda88_Number( int number )
{
#ifdef USE_ADA88
  ada88_writeNumber(number);  // -1999 - 1999
#endif
}
void setAda88_Bit( uint16_t bit )
{
#ifdef USE_ADA88
  ada88_writeBit(bit);  // 0x0000 - 0xffff
#endif
}
void setAda88_5prm(uint8_t prm1,uint8_t prm2,uint8_t prm3,uint8_t prm4,uint8_t prm5)
//  prm1:0-9, prm2-5:0-7
{
#ifdef USE_ADA88
  ada88_write_5param(prm1,prm2,prm3,prm4,prm5);
#endif
}
/*----------------------------------------------------------------------------*/
//
//     Blink LED by NeoPixel Library
//
/*----------------------------------------------------------------------------*/
const uint8_t colorTable[16][3] = {
  { 200,   0,   0 },//  C
  { 175,  30,   0 },
  { 155,  50,   0 },//  D
  { 135,  70,   0 },
  { 110,  90,   0 },//  E
  {   0, 160,   0 },//  F
  {   0, 100, 100 },
  {   0,   0, 250 },//  G
  {  30,   0, 230 },
  {  60,   0, 190 },//  A
  { 100,   0, 140 },
  { 140,   0,  80 },//  B

  { 100, 100, 100 },
  { 100, 100, 100 },
  { 100, 100, 100 },
  { 100, 100, 100 }
 };
/*----------------------------------------------------------------------------*/
uint8_t colorTbl( uint8_t doremi, uint8_t rgb ){ return colorTable[doremi][rgb];}
void setLed( int ledNum, uint8_t red, uint8_t green, uint8_t blue )
{
  led.setPixelColor(ledNum,led.Color(red, green, blue));
}
void lightLed( void )
{
  led.show();
}
