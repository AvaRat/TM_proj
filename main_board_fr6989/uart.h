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
#include "system.h"


extern char rxString[MAX_STR_LEN];

void init_clock(void);
void init_uartA0(void);
void init_uartA1();
void transmitString(char *);
void trigger_RTC();






#endif /* UART_H_ */
