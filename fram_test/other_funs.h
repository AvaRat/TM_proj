#ifndef OTHER_FUNS_H_
#define OTHER_FUNS_H_

#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>

#define SAMPLING_PERIOD 10          // ms
#define SMCLK_115200    0
#define SMCLK_9600      1
#define ACLK_9600       2

#define UART_MODE       ACLK_9600 //SMCLK_115200 //ACLK_9600//SMCLK_115200//

void init_UART();
void init_reference_module();
void init_GPIO();
void init_SAC();
void init_clock_to_16mhz();
inline void send_12bit_via_uart(int number);
inline void receive_12bit_via_uart();




#endif /* OTHER_FUNS_H_ */
