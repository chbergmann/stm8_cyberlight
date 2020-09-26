Cyberlight STM8
===============

RGBW light controller

by Christian Bergmann

 * RGB LEDs at 3 PWM GPIO outputs
 * white LED controlled by software PWM
 * I2C: PCF8583 Real Time Clock
 * change color by time of day
 * UART1: ESP8266 Wifi module (see https://github.com/chbergmann/Cyberlight)
 * control LED colors and brightness with your smartphone via web interface
 
Compile with http://sdcc.sourceforge.net

Flash with https://github.com/vdudouyt/stm8flash

## UART Commands:
 * M<0-4>   Moodlight mode (0==off, 4=timer)
 * R<0-255> red
 * G<0-255> green
 * B<0-255> blue
 * SV       save values in flash
 * H<0-255>	hue level at midnight
 * ST<SSMMHHWWDDMMYY>	set time
 * D<0-255> color change speed
 * C[U|D]	change direction Up or Down
