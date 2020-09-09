/*
 * constants.h
 *
 *  Created on: Sep 4, 2020
 *      Author: marce
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define WDT_SETUP 0x5A2B // 1 sekunda, zegar ACLK
#define STOP_WATCHDOG 0x5A80 // Stop the watchdog

#define MAX_STR_LEN 256

#define SAMPLING_PERIOD 10         // ms

// dostępne tryby:
#define HARMONICAL      0
#define ECG             1

#define HARMONICAL_NOISES    (1 << 0)
#define PULSE_NOISES         (1 << 1)

#define ADC_BITS        12

#define NOISE_MAX       16

#define PULSE_BITS      5
#define NOISE_AMPLIFY   10



#endif /* CONSTANTS_H_ */
