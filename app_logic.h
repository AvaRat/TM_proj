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

enum periodic_interf_type {SIN, EKG};

void display_params(int pg, int ig, int);



#endif /* APP_LOGIC_H_ */
