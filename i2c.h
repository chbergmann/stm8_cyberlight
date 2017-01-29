#ifndef PCF8583_H
#define PCF8583_H

void InitialiseI2C();
void I2C_ISR();

void I2C_start_write(int num_bytes, uint8_t *txbytes);
void I2C_start_read(int num_bytes);
extern void I2C_read_finished(uint8_t *rxbytes);
extern void I2C_write_finished();
void PCF8583_WriteTime(uint8_t* timerval);

#define BUFFERSIZE	8
#define DEVICE_ADDRESS	0x51

#endif
