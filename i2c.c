#include "stm8s.h"
#include "i2c.h"
#include <stdio.h>
#include <string.h>

/* Code derived from
 * http://blog.mark-stevens.co.uk/2015/05/stm8s-i2c-master-devices/
 */

uint8_t Rx_Idx = 0;
uint8_t Tx_Idx = 0;
extern uint8_t NumBytesToRead = 1;
extern uint8_t NumBytesToSend = 1;
uint8_t RxBuffer[BUFFERSIZE];
uint8_t TxBuffer[BUFFERSIZE];

#define SETBIT(A,B)	A |= B
#define CLRBIT(A,B)	A &= ~B
//
//  Initialise the I2C system.
//
void InitialiseI2C()
{
	CLRBIT(I2C->CR1, I2C_CR1_PE);                     //  Diable I2C before configuration starts.
    //
    //  Setup the clock information.
    //
    I2C->FREQR = 16;                     //  Set the internal clock frequency (MHz).
    CLRBIT(I2C->CCRH, I2C_CCRH_FS);                   //  I2C running is standard mode.
    I2C->CCRL = 0xa0;                    //  SCL clock speed is 50 KHz.
    CLRBIT(I2C->CCRH, I2C_CCRH_CCR);
    //
    //  Set the address of this device.
    //
    CLRBIT(I2C->OARH, I2C_OARH_ADDMODE);               //  7 bit address mode.
    SETBIT(I2C->OARH, I2C_OARH_ADDCONF);               //  Docs say this must always be 1.
    //
    //  Setup the bus characteristics.
    //
    I2C->TRISER = 17;
    //
    //  Turn on the interrupts.
    //
    SETBIT(I2C->ITR, I2C_ITR_ITBUFEN);                //  Buffer interrupt enabled.
    SETBIT(I2C->ITR, I2C_ITR_ITEVTEN);                //  Event interrupt enabled.
    SETBIT(I2C->ITR, I2C_ITR_ITERREN);
    //
    //  Configuration complete so turn the peripheral on.
    //
    SETBIT(I2C->CR1, I2C_CR1_PE);
}

void I2C_start_read(int num_bytes)
{
	NumBytesToRead = num_bytes;
	NumBytesToSend = 0;
	Rx_Idx = 0;
    //
    //  Enter master mode.
    //
    SETBIT(I2C->CR2, I2C_CR2_ACK);
    SETBIT(I2C->CR2, I2C_CR2_START);
}

void I2C_start_write(int num_bytes, uint8_t *txbytes)
{
	NumBytesToRead = 0;
	NumBytesToSend = num_bytes;
	memcpy(TxBuffer, txbytes, num_bytes);
	Tx_Idx = 0;
    //
    //  Enter master mode.
    //
    SETBIT(I2C->CR2, I2C_CR2_ACK);
    SETBIT(I2C->CR2, I2C_CR2_START);
}

//
//  I2C interrupts all share the same handler.
//
void I2C_ISR()
{
	uint8_t sr1 = I2C->SR1;
	if (sr1 & I2C_SR1_SB)
	{
	    //
	    //  Master mode, send the address of the peripheral we
	    //  are talking to.  Reading SR1 clears the start condition.
	    //
	    volatile uint8_t reg = I2C->SR1;
	    //
	    //  Send the slave address and the read bit.
	    //
	    if(NumBytesToRead > 0)
		    I2C->DR = (DEVICE_ADDRESS << 1) | 1;
	    else
		    I2C->DR = (DEVICE_ADDRESS << 1);

	    //
	    //  Clear the address registers.
	    //
	    CLRBIT(I2C->OARL, I2C_OARL_ADD);
	    CLRBIT(I2C->OARH, I2C_OARH_ADD);
	    return;
	}

	if(sr1 & I2C_SR1_BTF)
	{
		SETBIT(I2C->CR2, I2C_CR2_STOP);
		if(Tx_Idx < NumBytesToSend)
		{
			I2C->DR = TxBuffer[Tx_Idx++];
		}
		return;
	}

	if (sr1 & I2C_SR1_ADDR)
	{
		//
		//  In master mode, the address has been sent to the slave.
		//  Clear the status registers and wait for some data from the slave.
		//
		(void)I2C->SR1;
		(void)I2C->SR3;
		if(NumBytesToSend > 0)
		{
			I2C->DR = TxBuffer[Tx_Idx++];
		}
	    return;
	}

	if(sr1 & I2C_SR1_TXE)
	{
		if(Tx_Idx < NumBytesToSend)
		{
			I2C->DR = TxBuffer[Tx_Idx++];
		}
		return;
	}

	if (sr1 & I2C_SR1_RXNE)
	{
		RxBuffer[Rx_Idx++] = I2C->DR;
		if (Rx_Idx == NumBytesToRead - 1)
		{
			CLRBIT(I2C->CR2, I2C_CR2_ACK);
			SETBIT(I2C->CR2, I2C_CR2_STOP);
		}
		if (Rx_Idx == NumBytesToRead)
			I2C_read_finished(RxBuffer);
		return;
	}

	//
	//  If we get here then we have an error so clear
	//  the error and continue.
	//
	if(sr1 != 0)
		printf("I2C error %X %X\r\n", I2C->SR1, I2C->SR3);
}
