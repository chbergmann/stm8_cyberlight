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

uint16_t cycle;     /* counter used by modes */
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

volatile uint8_t tick;


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


/* called after last pwm phase when tick == 0 */
void moodlight_step(void)
{
    if (config.flags & ML_FLAG_OFF) {
        tick= 255;
        return;
    }

    switch (config.mode) {

    case 1: /* fading R-G-B */
        {
            if (cycle <= MAX_BRIGHT) {
                config.level_r = (MAX_BRIGHT+1) - cycle;
                config.level_g = cycle;
                config.level_b = 0;
            }
            else if (cycle <= 2*MAX_BRIGHT) {
                config.level_r = 0;
                config.level_g = (2*MAX_BRIGHT+1) - cycle;
                config.level_b = cycle - MAX_BRIGHT;
            }
            else {
                config.level_r = cycle - (2*MAX_BRIGHT);
                config.level_g = 0;
                config.level_b = (3*MAX_BRIGHT+1) - cycle;
            }

            if ((config.flags & ML_FLAG_PAUSE) == 0) {
                if (config.flags & ML_FLAG_UP) {
                    cycle = (cycle == 3*MAX_BRIGHT+1 ? 1 : cycle+1);
                } else {
                    cycle = (cycle == 0 ? 3*MAX_BRIGHT : cycle-1);;
                }
            }
            tick = config.delay;
            break;
        }

    case 2: /* fading R-R+G-G-G+B-B-B+R */
    case 4: /* fade by clock */
        {
            if (cycle <= MAX_BRIGHT) {
                /* R -> R+G */
                config.level_r = MAX_BRIGHT;
                config.level_g = cycle;
                config.level_b = 0;
            }
            else if (cycle <= 2*MAX_BRIGHT) {
                /* R+G -> G */
                config.level_r = 2*MAX_BRIGHT+1-cycle;
                config.level_g = MAX_BRIGHT;
                config.level_b = 0;
            }
            else if (cycle <= 3*MAX_BRIGHT) {
                /* G -> G+B */
                config.level_r = 0;
                config.level_g = MAX_BRIGHT;
                config.level_b = cycle - 2*MAX_BRIGHT;
            }
            else if (cycle <= 4*MAX_BRIGHT) {
                /* G+B -> B */
                config.level_r = 0;
                config.level_g = 4*MAX_BRIGHT+1 - cycle;
                config.level_b = MAX_BRIGHT;
            }
            else if (cycle <= 5*MAX_BRIGHT) {
                /* B -> B+R */
                config.level_r = cycle - 4*MAX_BRIGHT;
                config.level_g = 0;
                config.level_b = MAX_BRIGHT;
            }
            else {
                /* B+R -> R */
                config.level_r = MAX_BRIGHT;
                config.level_g = 0;
                config.level_b = 6*MAX_BRIGHT+1 - cycle;
            }

            if ((config.flags & ML_FLAG_PAUSE) == 0) {
            	uint32_t minSinceMN1 = (timerval[3] * 60 + timerval[2]);
            	uint32_t minSinceMN = minSinceMN1 * MAX_BRIGHT / 240;
                if (config.flags & ML_FLAG_UP) {
                	if(config.mode == 4)
                		cycle = (minSinceMN + config.huelevel * 6);
                	else
                		cycle = (cycle == 6*MAX_BRIGHT+1 ? 1 : cycle+1);
                } else {
                	if(config.mode == 4)
                		cycle = (config.huelevel * 6 - minSinceMN);
                	else
                		cycle = (cycle == 0 ? 6*MAX_BRIGHT : cycle-1);
                }

                if(cycle >= 6*MAX_BRIGHT)
                	 cycle = cycle - (6*MAX_BRIGHT);
            }
            tick = config.delay;
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
