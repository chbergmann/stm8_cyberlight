/********************************************************************
 * STM8S103F3 RGB Moodlight with remote control                     *
 *                                                                  *
 * LEDs connected to TIM2CH1(PD4), TIM2CH2(PD3), TIM2CH3(PA3)       *
 * IR receiver TSOP34136 or similar connected to PA1                *
 *                                                                  *
 * (w) XL 2015, $Id: main.c 287 2015-10-25 23:22:49Z schwenke $   *
 ********************************************************************/

#include "stm8s.h"
#include "moodlight.h"
#include "uart.h"
#include "i2c.h"
#include <stdio.h>

volatile uint8_t sw_pwm_tick;
volatile uint8_t tickcount = 0;
extern uint8_t newtime[8];

int set_time = 0;
extern uint8_t time_read_finished;

void PCF8583_PrintTime();

void ISR_tick_100us()
{
	sw_pwm_tick++;
	if(sw_pwm_tick > 127)
		sw_pwm_tick = 0;

	if(sw_pwm_tick >= (config.level_w >> 1))
		GPIOD->ODR &= ~0x02;
	else
		GPIOD->ODR |= 0x02;
}

void ISR_tick()
{
    if (tick) {
        --tick;
    }

	if(set_time == 0)	// read time
	{
		if(tickcount == 0) {
			uint8_t reg = 0;
			time_read_finished = 0;
			I2C_start_write(1, &reg);
		}
	}
	else if(set_time == 1)	// set time
	{
		if(tickcount == 0) {
			PCF8583_WriteTime(newtime);
			set_time = 0;
		}
	}

	tickcount++;
	if(tickcount >= 250)
		tickcount = 0;

	ISR_uart_tx();
}

INTERRUPT_HANDLER(timer4_uev, 23)
{
	ISR_tick_100us();
	//poll_i2c();

    // clear interrupt flag
    TIM4->SR1 = 0;
}

INTERRUPT_HANDLER(timer2_uev, 13)
{
    // clear interrupt flag
    ISR_tick();
    TIM2->SR1 = 0;
}


INTERRUPT_HANDLER(I2C_IRQHandler, 19)
{
	I2C_ISR();
}

INTERRUPT_HANDLER(UART1_TX_IRQHandler, 17)
{
	ISR_uart_tx();
}

INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18)
{
	ISR_uart_rx();
}

void hw_init(void)
{
    // configure indicator LED pin
    GPIOD->DDR = 0x03;
    GPIOD->CR1 = 0x03;
    GPIOD->ODR |= 0x03;

    // configure clock
    CLK->CKDIVR  = 0b00000000; // HSIDIV=1 => master clk=16MHz
    CLK->PCKENR1 |= CLK_PCKENR1_TIM2 | CLK_PCKENR1_TIM4;

    // configure timer2 (PWM)
    TIM2->PSCR = 0;           // no prescaler => clk=16MHz
    TIM2->ARRH = 0xFF;        // timer reload value
    TIM2->ARRL = 0xFF;
    TIM2->CCMR1 = 0b01101000; // CC1: PWM mode 1, preload enabled
    TIM2->CCMR2 = 0b01101000; // CC2: PWM mode 1, preload enabled
    TIM2->CCMR3 = 0b01101000; // CC3: PWM mode 1, preload enabled
    TIM2->CCER1 = 0b00010001; // CC1+CC2 enabled
    TIM2->CCER2 = 0b00000001; // CC3 enabled
    TIM2->IER = TIM2_IER_UIE; // generate update interrupt
    TIM2->EGR = TIM2_EGR_UG;  // force update event to load registers
    TIM2->CR1 = TIM2_CR1_CEN; // enable timer

    // configure timer4 (IRMP)
    TIM4->PSCR = 4; // prescaler=16 => clk=1MHz
    TIM4->ARR = 9; // overflow every 10us
    TIM4->IER = TIM4_IER_UIE; // generate update interrupt
    TIM4->CR1 = TIM4_CR1_CEN; // enable timer
}

int main(void)
{
    hw_init();
    InitialiseUART();
    InitialiseI2C();
    moodlight_init();

/*
    newtime[0] = 0;		// millisecond
    newtime[1] = 0;		// second
    newtime[2] = 55;	// minute
    newtime[3] = 8;		// hour
    newtime[4] = 6;		// weekday
    newtime[5] = 25;	// day
    newtime[6] = 10;		// month
    newtime[7] = 20;	// year
	set_time = 1;
*/
    // enable interrupts
    rim();

	while(time_read_finished == 0)
	{
		wfi();
	}
	moodlight_step();

    // endless loop
    while(1) {
		if(tickcount == 0)
		{
			PCF8583_PrintTime();
			printf("R%d\r\nG%d\r\nB%d\r\nW%d\r\nM%d\r\nH%d\r\nD%d\r\nF%d\r\n",
					config.level_r, config.level_g, config.level_b, config.level_w,
					config.mode, config.huelevel, config.delay, config.flags);
		}

        if (tick == 0) {
			moodlight_step();
			setup_pwm();
        }
		wfi();
    }
}

