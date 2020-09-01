/*
 * app_logic.h
 *
 *  Created on: Aug 31, 2020
 *      Author: marce
 */
#include <driverlib.h>
#include "hal_LCD.h"

#ifndef APP_LOGIC_H_
#define APP_LOGIC_H_


// define app states
#define STARTUP 0
#define SETTINGS 1
#define SETTINGS_BEGIN 2
#define SETTINGS_PERIODIC_SIN 3
#define SETTINGS_PERIODIC_EKG 4
#define SETTINGS_PERIODIC_GAIN 5
#define SETTINGS_IMPULSIVE_GAIN 6
#define NORMAL 7


struct params_struct {
    unsigned int periodic_gain;
    unsigned int impulsive_gain;
    unsigned int periodic_interf_type;
};

extern volatile unsigned char app_state;
struct params_struct params;

extern volatile unsigned char S1buttonDebounce;
extern volatile unsigned char S2buttonDebounce;
extern volatile unsigned int holdCount;

volatile unsigned int gain_value;
volatile unsigned int show_text;

enum periodic_interf_type {SIN=SETTINGS_PERIODIC_SIN, EKG=SETTINGS_PERIODIC_EKG};

void display_params(int pg, int ig, int);
void init_settings();
void LPM3_delay(void);

#endif /* APP_LOGIC_H_ */
