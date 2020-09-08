/*
 * system_headers.h
 *
 *  Created on: Sep 4, 2020
 *      Author: marce
 */

#ifndef SYSTEM_HEADERS_H_
#define SYSTEM_HEADERS_H_

void init_ADC12(void);
void init_timerB0_for_ADC(void);
void init_DMA(volatile unsigned int *);
void set_params(int);
void init_timerA_for_RNG(int);
void initClockTo16MHz(void);

#endif /* SYSTEM_HEADERS_H_ */
