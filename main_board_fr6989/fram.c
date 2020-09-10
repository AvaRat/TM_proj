#include <fram.h>

unsigned char * FRAM_params_ptr;
const unsigned int params_size = sizeof(struct fram_params_struct);


void save_params(struct params_struct * params_struct_ptr) {
    struct fram_params_struct tmp_fram_struct;
    tmp_fram_struct.if_in_use_mem_block = 0x00;
    tmp_fram_struct.used_before_reset = params_struct_ptr -> used_before_reset;
    tmp_fram_struct.harmonic_gain = params_struct_ptr -> harmonic_gain;
    tmp_fram_struct.impulsive_gain = params_struct_ptr -> impulsive_gain;
    tmp_fram_struct.signal_type = params_struct_ptr -> signal_type;
    strncpy(tmp_fram_struct.time_string, params_struct_ptr->time_string, MAX_STR_LEN);

    write_FRAM_params(& tmp_fram_struct);
}

void get_params(struct params_struct * params_struct_ptr) {
    struct fram_params_struct tmp_fram_struct;
    read_FRAM_params(&tmp_fram_struct);

    params_struct_ptr -> used_before_reset = tmp_fram_struct.used_before_reset;
    params_struct_ptr -> harmonic_gain = tmp_fram_struct.harmonic_gain;
    params_struct_ptr -> impulsive_gain = tmp_fram_struct.impulsive_gain;
    params_struct_ptr -> signal_type = tmp_fram_struct.signal_type;
    strncpy(params_struct_ptr->time_string, tmp_fram_struct.time_string, MAX_STR_LEN);
}


void find_FRAM_params_ptr() {
    if (FRAM_params_ptr > 0)        // ju� by�o ustawione
        return;

    FRAM_params_ptr = (unsigned char *) FRAM_START_INF_ADDR;
    while ((unsigned int) FRAM_params_ptr < FRAM_END_INF_ADDR) {
        if (*FRAM_params_ptr == 0x00)
            break;
        FRAM_params_ptr += params_size;

        if (FRAM_params_ptr > (unsigned char *) FRAM_END_INF_ADDR) {
            FRAM_params_ptr = (unsigned char *) FRAM_START_INF_ADDR;
            break;
        }
    }
}

void increment_FRAM_params_ptr() {
    FRAM_params_ptr += params_size;
    if (FRAM_params_ptr > (unsigned char *) FRAM_END_INF_ADDR)
        FRAM_params_ptr = (unsigned char *) FRAM_START_INF_ADDR;
}

void write_FRAM_params(struct fram_params_struct * struct_ptr) {
    find_FRAM_params_ptr();
    unsigned char * prev_params_ptr = FRAM_params_ptr;

    increment_FRAM_params_ptr();                                                // znajdz miejsce na nowa strukture
    memcpy(FRAM_params_ptr, struct_ptr, sizeof(struct fram_params_struct));     // zapisz struktur�
    unsigned char * ptr = FRAM_params_ptr;
    //for (ptr = FRAM_params_ptr; ptr < (FRAM_params_ptr + sizeof(struct fram_params_struct)); ptr++) {
    //    * ptr = * (struct_ptr + ptr - FRAM_params_ptr);
    //}
/*
    int i = 0;
    unsigned char * src = (unsigned char *) struct_ptr;
    for (i = 0; i < sizeof(struct fram_params_struct); i++) {
        * (ptr + i) = *(src + i);
    }
*/
 //   * ptr = 0x00;

    // wykasuj informacje o poprzedniej strukturze:
    * prev_params_ptr = 0xFF;
}


void read_FRAM_params(struct fram_params_struct * struct_ptr) {
    find_FRAM_params_ptr();
    memcpy(struct_ptr, FRAM_params_ptr, sizeof(struct fram_params_struct));
}


void clear_FRAM_params_space() {
    unsigned int i = FRAM_START_INF_ADDR;
    for (i; i < FRAM_END_INF_ADDR; i++) {
        *((unsigned char *) i) = 0xFF;
    }
}


