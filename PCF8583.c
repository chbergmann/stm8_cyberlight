//#######################################################################################################
//#################################### Plugin 007: ExtWiredAnalog #######################################
//#######################################################################################################

/*********************************************************************************************\
 * This plugin provides support for 4 extra analog inputs, using the PCF8591 (NXP/Philips)
 * Support            : www.esp8266.nu
 * Date               : Oct 2016
 * Compatibility      : R004
 * Syntax             : "ExtWiredRTC <Par1:Port>, <Par2:Variable>"
 *********************************************************************************************
 * Technical description:
 *
 * De PCF8583 is a IO Expander chip that connects through the I2C bus
 * Basic I2C address = 0x51
 \*********************************************************************************************/
#include "stm8s.h"
#include <string.h>
#include <stdio.h>
#include "i2c.h"

#define PLUGIN_106
#define PLUGIN_ID_106         106
#define PLUGIN_NAME_106       "Real Time Clock - PCF8583"
#define PLUGIN_VALUENAME1_106 "Hundredth second"
#define PLUGIN_VALUENAME2_106 "Second"
#define PLUGIN_VALUENAME3_106 "Minute"
#define PLUGIN_VALUENAME4_106 "Hour"
#define PLUGIN_VALUENAME5_106 "Weekday"
#define PLUGIN_VALUENAME6_106 "Day"
#define PLUGIN_VALUENAME7_106 "Month"
#define PLUGIN_VALUENAME8_106 "Year"
#define I2CADDR 0x51


typedef enum {
	I2C_IDLE,
	I2C_TX,
	I2C_START_READ,
	I2C_RECV_TIME,
} i2c_state_t;

i2c_state_t i2c_state = I2C_IDLE;

const char weekdays[7][4] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

#define BCD_TO_BIN(x) ((x >> 4) * 10 + (x & 0x0F))
#define BIN_TO_BCD(x) ((x % 10) + ((x/10) << 4))

uint8_t timerval[8];

void PCF8583_ReadTime(uint8_t* i2cval)
{
	timerval[0] = BCD_TO_BIN(i2cval[1]);  // 100th seconds
	timerval[1] = BCD_TO_BIN(i2cval[2]);  // seconds
	timerval[2] = BCD_TO_BIN(i2cval[3]);  // minutes
	timerval[3] = ((i2cval[4] >> 4) & 3) * 10 + (i2cval[4] & 0x0F); // hours
	timerval[4] = (i2cval[6] >> 5);       // weekday
	if(timerval[4] >= 7)
		timerval[4] = 0;
	{
	uint8_t day10 = ((i2cval[5] & 0x30) >> 4) * 10;
	uint8_t m10 = ((i2cval[6] & 0x10) >> 4) * 10;
	timerval[5] = (i2cval[5] & 0x0F) +  day10; // day
	timerval[6] = (i2cval[6] & 0x0F) + m10;  // month
	timerval[7] = (i2cval[5] >> 6);       // years after last leap year
	}
}

void PCF8583_WriteTime(uint8_t* timerval)
{
    uint8_t i2cval[7];
    i2cval[0] = 1;
    i2cval[1] = BIN_TO_BCD(timerval[0]);
    i2cval[2] = BIN_TO_BCD(timerval[1]);
    i2cval[3] = BIN_TO_BCD(timerval[2]);
    i2cval[4] = BIN_TO_BCD(timerval[3]);
    i2cval[5] = BIN_TO_BCD(timerval[5]) + (BIN_TO_BCD(timerval[7]) << 6);
    i2cval[6] = BIN_TO_BCD(timerval[6]) + (BIN_TO_BCD(timerval[4]) << 5);

    I2C_start_write(7, i2cval);
}

void I2C_read_finished(uint8_t *rxbytes)
{
  char timestr[50];
  //const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  PCF8583_ReadTime(rxbytes);
  if(timerval[5] == 0)
  {
	  int i;
	  for(i=0; i<7; i++)
		  printf("%02X ", rxbytes[i]);
	  printf("\r\n");
	  for(i=0; i<7; i++)
		  printf("%02X ", timerval[i]);
	  printf("\r\n");
  }
  else
  {
    sprintf(timestr, "%d:%02d:%02d %s %d.%d", timerval[3], timerval[2], timerval[1],
      weekdays[timerval[4]], timerval[5], timerval[6]);
    printf(timestr);
  }
  printf("\r\n");
}
