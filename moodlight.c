/*
 * RGB Moodlight
 *
 * moodlight.c - moodlight modes and remote control
 *
 * (c) Axel (XL) Schwenke, axel.schwenke@gmx.net
 *
 * $Id: moodlight.c 287 2015-10-25 23:22:49Z schwenke $
 */

#include "stm8s.h"
#include "moodlight.h"
#include <string.h>

#define MAX_BRIGHT 255


/* more moodlight constants */

#define ML_MODE3_PERIOD 0x40  /* must be a power of 2 */

static uint16_t cycle = 0;     /* counter used by modes */
extern uint8_t timerval[8];

struct config_st config;
struct config_st* PointerAttr pConfigFlash = (struct config_st*)FLASH_DATA_START_PHYSICAL_ADDRESS;

void load_config_from_flash()
{
	FLASH_Unlock(FLASH_MEMTYPE_DATA);
    memcpy(&config, pConfigFlash, sizeof(struct config_st));
	FLASH_Lock(FLASH_MEMTYPE_DATA);
}

void save_config_to_flash()
{
	FLASH_Unlock(FLASH_MEMTYPE_DATA);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->mode, config.mode);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->delay, config.delay);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->flags, config.flags);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->level_r, config.level_r);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->level_g, config.level_g);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->level_b, config.level_b);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->level_w, config.level_w);
	FLASH_ProgramByte((uint32_t)&pConfigFlash->huelevel, config.huelevel);
	FLASH_Lock(FLASH_MEMTYPE_DATA);
}

/* clock tick, decremented once in each PWM cycle */

volatile uint8_t tick = 1;


/* initialization */
void moodlight_init(void)
{
    tick = 1;
    load_config_from_flash();
    if(config.delay == 0)
    {
        config.mode  = 1;
        config.delay = 6;
        config.flags = 0;
        cycle = 1;
    }
}


/* called after waking up from power-off */
void moodlight_wakeup(void)
{
    switch (config.mode) {
    case 3:
        {
            cycle &= ~(ML_MODE3_PERIOD-1);
            break;
        }
    }
    tick = 1;
}

#define MAX_CYCLE	(3*(MAX_BRIGHT+1)-1)

void hue_to_rgb(uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint16_t lum = cycle % (MAX_BRIGHT + 1);
    if (cycle <= MAX_BRIGHT) {
        *r = MAX_BRIGHT - lum;
        *g = lum;
        *b = 0;
    }
    else if (cycle < 2*(MAX_BRIGHT+1)) {
        *r = 0;
        *g = MAX_BRIGHT - lum;
        *b = lum;
    }
    else {
        *r = lum;
        *g = 0;
        *b = MAX_BRIGHT - lum;
    }
}

void fire(uint16_t cycle, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint8_t grn;
	int rand = timerval[0] & 3;
	*b = 0;
	*r = 255;

	if(cycle < 42)
	{
		grn = 128 + cycle;
	}
	else if(cycle < 126)
	{
		grn = 212 - cycle;
	}
	else if(cycle < 170)
	{
		grn = cycle - 42;
	}
	else
	{
		grn = 128;
	}

	if(rand == 0)
	{
		*g = grn;
	}
	else if(rand == 1)
	{
		*g = 255 - grn;
	}
	else
	{
		*g = 128;
	}
}

uint16_t cycle_from_clock()
{
	uint16_t hue = config.huelevel;
	uint32_t minSinceMN = timerval[3];
	minSinceMN = (minSinceMN * 60 + timerval[2]) * MAX_BRIGHT / 480;
	if (config.flags & ML_FLAG_UP) {
		cycle = (minSinceMN + hue * 3);
		if(cycle > MAX_CYCLE)
			cycle -= (MAX_CYCLE+1);
	} else {
		cycle = (hue * 3 - minSinceMN);
		if(cycle > MAX_CYCLE)
			cycle += MAX_CYCLE+1;
	}
	return cycle;
}

/* called after last pwm phase when tick == 0 */
void moodlight_step(void)
{
    if (config.flags & ML_FLAG_OFF) {
    	config.level_r = 0;
    	config.level_g = 0;
    	config.level_b = 0;
        tick= 255;
        return;
    }

    switch (config.mode) {

    case 1: /* fading R-G-B */
    {
    	hue_to_rgb(&config.level_r, &config.level_g, &config.level_b);
        tick = config.delay;
    	if(config.flags & ML_FLAG_UP)
            cycle = (cycle >= MAX_CYCLE ? 0 : cycle+1);
    	else
    		cycle = (cycle == 0 ? MAX_CYCLE : cycle-1);
    	break;
    }

    case 2: /* fading B-G-R */
    {
    	uint8_t r, g, b;
    	hue_to_rgb(&r, &g, &b);
    	config.level_r = MAX_BRIGHT - r;
    	config.level_g = MAX_BRIGHT - g;
    	config.level_b = MAX_BRIGHT - b;
        tick = config.delay;
    	if(config.flags & ML_FLAG_UP)
            cycle = (cycle >= MAX_CYCLE ? 0 : cycle+1);
    	else
    		cycle = (cycle == 0 ? MAX_CYCLE : cycle-1);
        break;
    }

    case 3: /* switching R-R+G-G-G+B-B-R+B-R+G+B */
	{
		if ((cycle & (ML_MODE3_PERIOD-1)) == 0) {
			switch (cycle/ML_MODE3_PERIOD) {
			case 0:
				{
					config.level_r = MAX_BRIGHT;
					config.level_g = 0;
					config.level_b = 0;
					break;
				}
			case 1:
				{
					config.level_r = MAX_BRIGHT;
					config.level_g = MAX_BRIGHT;
					config.level_b = 0;
					break;
				}
			case 2:
				{
					config.level_r = 0;
					config.level_g = MAX_BRIGHT;
					config.level_b = 0;
					break;
				}
			case 3:
				{
					config.level_r = 0;
					config.level_g = MAX_BRIGHT;
					config.level_b = MAX_BRIGHT;
					break;
				}
			case 4:
				{
					config.level_r = 0;
					config.level_g = 0;
					config.level_b = MAX_BRIGHT;
					break;
				}
			case 5:
				{
					config.level_r = MAX_BRIGHT;
					config.level_g = 0;
					config.level_b = MAX_BRIGHT;
					break;
				}
			case 6:
				{
					config.level_r = MAX_BRIGHT;
					config.level_g = MAX_BRIGHT;
					config.level_b = MAX_BRIGHT;
					break;
				}
			}
		}

		if ((config.flags & ML_FLAG_PAUSE) == 0) {
			if (config.flags & ML_FLAG_UP) {
				cycle = (cycle == (7*ML_MODE3_PERIOD-1) ? 0 : cycle+1);
			} else {
				cycle = (cycle == 0 ? (7*ML_MODE3_PERIOD-1) : cycle-1);;
			}
		}
		tick = config.delay;
		break;
	}

    case 4: /* fade by clock */
		if ((config.flags & ML_FLAG_PAUSE) == 0) {
			cycle_from_clock();
			hue_to_rgb(&config.level_r, &config.level_g, &config.level_b);
		}
		tick = 250;
		break;

    case 5: /* fading R-G-B */
    {
        cycle = (cycle >= 188 ? 0 : cycle+1);
    	fire(cycle, &config.level_r, &config.level_g, &config.level_b);
        tick = 1;
    	break;
    }
    }
}

inline uint16_t rgblog(uint16_t level)
{
	return (level * (level+2));
}

void setup_pwm(void)
{
	uint16_t pwmval = rgblog(config.level_r);
    TIM2->CCR1H=(pwmval>>8);
    TIM2->CCR1L=(pwmval&0xFF);

    pwmval= rgblog(config.level_g);
    TIM2->CCR2H=(pwmval>>8);
    TIM2->CCR2L=(pwmval&0xFF);

    pwmval= rgblog(config.level_b);
    TIM2->CCR3H=(pwmval>>8);
    TIM2->CCR3L=(pwmval&0xFF);
}

