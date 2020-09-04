/*
 * system.h
 *
 *  Created on: Sep 4, 2020
 *      Author: marce
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include<msp430.h>

#include "constants.h"

// niech na razie wszystko będzie w ms

volatile unsigned int input_signal;               // temperature measurement result storage variable
//volatile unsigned int sine_counter = 0;
//volatile float scale = 1;                        // skala funkcji sinus. Przy równej 1: max = 4095, min = 0
//volatile int xd = 0;

extern unsigned int wave_counter;

extern int current_pulse_bit; // = PULSE_BITS - 1;
extern int noise_counter;
//unsigned int noise_amplifier = NOISE_AMPLIFY;                          // skala impulsów - jeśli równa 1, to impulsy są w zakresie 0-15
//int noise_amplitude = NOISE_MAX * NOISE_AMPLIFY;


extern unsigned int next_pulse_amplitude;
extern unsigned int tmp_next_pulse_amplitude;


extern unsigned int rn;
extern unsigned long A0_ticks;



extern volatile int mode;

extern volatile int noise_max;      // max wielkość szumów jako wykladnik potegi 2 (np jesli 7 to 2^7 = 128)
extern volatile int noise_period_ms;


extern volatile int noises;
extern unsigned char sign;
extern volatile int noise_step; // uzyskuje wartość w set_params()

extern int debug_wave_counter;
extern int debug_ktora_fala;
extern int debug_ktora_liczba;
extern int debug_harmonized_signal;
extern int debug_current_noise;
extern int debug_current_pulse_noise;
extern int debug_current_harmonical_noise;
extern int debug_sign;

#endif /* SYSTEM_H_ */
