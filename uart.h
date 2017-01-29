#ifndef UART_H
#define UART_H

void InitialiseUART();
void UARTPrintf(char *message);
void ISR_uart_tx();
void ISR_uart_rx();

#endif // UART_H
