/*
 * uart.h
 *
 *  Created on: Sep 2, 2020
 *      Author: marce
 */

#ifndef UART_H_
#define UART_H_

#include <msp430.h>
#include <driverlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void init_clock(void);
void init_uartA0(void);
void transmitString(char *);






#endif /* UART_H_ */
