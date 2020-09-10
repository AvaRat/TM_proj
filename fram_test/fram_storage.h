#ifndef FRAM_STORAGE_H_
#define FRAM_STORAGE_H_

#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FRAM_START_INF_ADDR     0x1800
#define FRAM_END_INF_ADDR       0x19FF

struct params_struct {
    // last use flag:
    unsigned char if_used_before_reset;     // 0 - nie, 1 - tak

    // signal params:
    unsigned char harmonic_gain;
    unsigned char impulsive_gain;
    unsigned char signal_type;

    // date params:
    unsigned char day_of_week;       // 1-7
    unsigned char hour;              // 0-23
    unsigned char minute;            // 0 59
};


struct fram_params_struct {
    // memory blok in-use flag:
    unsigned char if_in_use_mem_block;
    // last use flag:
    unsigned char if_used_before_reset;     // 0 - nie, 1 - tak

    // signal params:
    unsigned char harmonic_gain;
    unsigned char impulsive_gain;
    unsigned char signal_type;

    // date params:
    unsigned char day_of_week;       // 1-7
    unsigned char hour;              // 0-23
    unsigned char minute;            // 0 59
};


void save_params(struct params_struct * params_struct_ptr);
void get_params(struct params_struct * params_struct_ptr);

void find_FRAM_params_ptr();
void increment_FRAM_params_ptr();
void write_FRAM_params(struct fram_params_struct * struct_ptr);
void read_FRAM_params(struct fram_params_struct * struct_ptr);

void clear_FRAM_params_space();

#endif /* FRAM_STORAGE_H_ */