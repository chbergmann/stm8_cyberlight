/*
 * RGB Moodlight
 *
 * moodlight.h - moodlight modes and remote control, prototypes
 *
 * (c) Axel (XL) Schwenke, axel.schwenke@gmx.net
 *
 * $Id: moodlight.h 287 2015-10-25 23:22:49Z schwenke $
 */

#ifndef _MOODLIGHT_H_
#define _MOODLIGHT_H_

/* bits in config.flags */

#define ML_FLAG_OFF   0x80
#define ML_FLAG_PAUSE 0x02
#define ML_FLAG_UP    0x01

struct config_st
{
    uint8_t  mode;      /* the active moodlight mode */
    uint8_t  delay;     /* run moodlight_step() every "delay" PWM cycles */
    uint8_t  flags;     /* on/off, stopped, direction */
	uint8_t level_r;
	uint8_t level_g;
	uint8_t level_b;
	uint8_t level_w;
	uint8_t huelevel;
};
extern struct config_st config;

extern volatile uint8_t tick;

void moodlight_remote(void);
void moodlight_init(void);
void moodlight_wakeup(void);
void moodlight_step(void);
void setup_pwm(void);
void save_config_to_flash(void);

#endif
