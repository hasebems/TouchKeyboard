/* ========================================
 *
 *	configuration.h
 *		description: TouchMidi Configuration
 *
 *	Copyright(c)2017- Masahiko Hasebe at Kigakudoh
 *  This software is released under the MIT License, see LICENSE.txt.
 *
 * ========================================
*/
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//---------------------------------------------------------
//    Touch Sensor Setup Mode
//---------------------------------------------------------
#define   SETUP_MODE    0   //  1: Setup Mode, 0: Normal Mode

#define   MAX_DEVICE_MBR3110    12

//---------------------------------------------------------
//    Hardware Setting
//      ref: https://docs.arduino.cc/hacking/hardware/PinMapping32u4
//---------------------------------------------------------
#define   KBD_C     4   //  PD4
#define   KBD_CS    30  //  PD5
#define   KBD_D     12  //  PD6
#define   KBD_DS    A2  //  PF5
#define   KBD_E     A1  //  PF6
#define   KBD_F     A0  //  PF7
#define   KBD_FS    11  //  PB7
#define   KBD_G     5   //  PC6
#define   KBD_GS    13  //  PC7
#define   KBD_A     8   //  PB4
#define   KBD_AS    9   //  PB5
#define   KBD_B     10  //  PD6

#define NEOPIXEL_PIN  7   // PE6  -- no connect
#define LED_ERR       6   //  PD7
#define LED1          A5   // PF0
#define LED2          A4   // PF1
#define JOYSTICK_X    A5   // PF0
#define JOYSTICK_Y    A4   // PF1

//  Not Use
#define   MAX_LED       12

//#define  USE_NEO_PIXEL
//---------------------------------------------------------
//    I2C Device Configuration
//---------------------------------------------------------
#define   USE_CY8CMBR3110
#define   USE_ADA88
//#define   USE_AP4
//#define   USE_LPS22HB
//#define   USE_LPS25H
//#define   USE_AQM1602XA
//#define   USE_ADXL345
//#define   USE_PCA9685
//#define   USE_ATTINY
//#define   USE_PCA9544A
#define   USE_PCAL9555A
#endif
