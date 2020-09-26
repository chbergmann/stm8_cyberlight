#include <string.h>
#include "stm8s.h"
#include "uart.h"
#include "stm8s_uart1.h"
#include "stdio.h"
#include "moodlight.h"
#include "i2c.h"


/*
 * UART Commands:
 * M<0-4>   Moodlight mode (0==off, 4=timer)
 * R<0-255> red
 * G<0-255> green
 * B<0-255> blue
 * SV       save values in flash
 * H<0-255>	hue level at midnight
 * ST<SSMMHHWWDDMMYY>	set time
 * D<0-255> color change speed
 * C[U|D]	change direction Up or Down
 */

typedef enum
{
	USTATE_IDLE = 0,
	USTATE_MOOD,
	USTATE_NUM,
	USTATE_NUM2,
	USTATE_SAVE,
	USTATE_TIME,
	USTATE_DIR,
} UART_STATE_t;

UART_STATE_t uart_state = USTATE_IDLE;

uint8_t newtime[8];
int newtime_idx = 0;
extern int set_time;

char txbuffer[64];
int wrptr = 0;
int rdptr = 0;

//
//  Setup the UART to run at 115200 baud, no parity, one stop bit, 8 data bits.
//
//  Important: This relies upon the system clock being set to run at 16 MHz.
//
void InitialiseUART()
{
	UART1_DeInit();
    UART1_Init((uint32_t)115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
                UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);

	UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);

	/* Output a message on Hyperterminal using printf function */
	printf("\n\rSTM8 running\n\r");
}

/**
  * @brief Retargets the C library printf function to the UART.
  * @param c Character to send
  */
int putchar (int c)
{
	txbuffer[wrptr++] = c;
	if(wrptr >= sizeof(txbuffer))
		wrptr = 0;

	ISR_uart_tx();
	return c;
}

/**
  * @brief Retargets the C library scanf function to the USART.
  * @param None
  * @retval char Character to Read
  */
int getchar (void)
{
  char c = 0;

  /* Loop until the Read data register flag is SET */
  while (UART1_GetFlagStatus(UART1_FLAG_RXNE) == RESET);
    c = UART1_ReceiveData8();
  return (c);
}


void ISR_uart_tx()
{
	if (UART1_GetFlagStatus(UART1_FLAG_TXE) == SET)
	{
		if(wrptr != rdptr) {
			UART1_SendData8(txbuffer[rdptr++]);
			if(rdptr >= sizeof(txbuffer))
				rdptr = 0;
		}
	}
}

uint8_t number = 0;
char last_command = 0;


void ISR_uart_rx()
{
	if (UART1_GetFlagStatus(UART1_FLAG_RXNE) == SET)
	{
		char c = UART1_ReceiveData8();
		switch(uart_state)
		{
		case USTATE_IDLE:
			switch(c)
			{
			case 'M':
				uart_state = USTATE_MOOD;
				break;

			case 'R':
			case 'G':
			case 'B':
			case 'W':
			case 'H':
			case 'D':
				last_command = c;
				uart_state = USTATE_NUM;
				break;
			case 'S':
				uart_state = USTATE_SAVE;
				break;
			case 'C':
				uart_state = USTATE_DIR;
				break;
			}
			break;

		case USTATE_NUM:
			if(c < '0' || c > '9')
			{
				uart_state = USTATE_IDLE;
			}
			else
			{
				uart_state = USTATE_NUM2;
				number = c - '0';
			}
			break;

		case USTATE_NUM2:
			if(c < '0' || c > '9')
			{
				switch(last_command)
				{
				case 'R':
					config.level_r = number;
					config.mode = 0;
					break;
				case 'G':
					config.level_g = number;
					config.mode = 0;
					break;
				case 'B':
					config.level_b = number;
					config.mode = 0;
					break;
				case 'W':
					config.level_w = number;
					config.mode = 0;
					break;
				case 'H':
					config.huelevel = number;
					break;
				case 'D':
					config.delay = number;
					break;
				}
				uart_state = USTATE_IDLE;
				setup_pwm();
			}
			else
				number = number * 10 + c - '0';
			break;

		case USTATE_MOOD:
		{
			config.mode = c - '0';
			uart_state = USTATE_IDLE;
			break;
		}

		case USTATE_SAVE:
			if(c == 'V')
				save_config_to_flash();

			if(c == 'T') {
				newtime[0] = 0;
				newtime_idx = 2;
				uart_state = USTATE_TIME;
			}
			else
				uart_state = USTATE_IDLE;
			break;

		case USTATE_TIME:
			if(c < '0' || c > '9') {
				uart_state = USTATE_IDLE;
				printf("X");
				break;
			}
			{
			uint8_t val = c - '0';

			if((newtime_idx & 1) == 0)
				newtime[newtime_idx/2] = val * 10;
			else
				newtime[newtime_idx/2] += val;
			}
			newtime_idx++;
			if(newtime_idx == 16) {
				uart_state = USTATE_IDLE;
				set_time = 1;
			}

			break;

		case USTATE_DIR:
			if(c == 'D')
				config.flags &= ~ML_FLAG_UP;

			if(c == 'U')
				config.flags |= ML_FLAG_UP;

			uart_state = USTATE_IDLE;
			break;
		}
	}
}


