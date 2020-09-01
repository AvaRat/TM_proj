/*
 * app_logic.c
 *
 *  Created on: Aug 31, 2020
 *      Author: marce
 */
#include "app_logic.h"

// TimerA0 UpMode Configuration Parameter
Timer_A_initUpModeParam initUpParam_display =
{
        TIMER_A_CLOCKSOURCE_ACLK,              // ACLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // ACLK/4 = 2MHz
        60000,                                  // 15ms debounce period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR,                       // Clear value
        true                                    // Start Timer
};


/*
 * pg -> periodic interference gain (0-1)
 * ig -> impulsive interference gain (0-1)
 */
void display_params(int pg, int ig, int periodic_interf_type)
{
    diplayScrollText("PERIODIC INTERFERENCE      ");
    displayScrollText("TYPE"); //PERIODIC INTERFERENCE TYPE
    if(periodic_interf_type==SIN)
    {
        showChar('S', pos1);
        showChar('I', pos2);
        showChar('N', pos3);
    }else
    {
        showChar('E', pos1);
        showChar('K', pos2);
        showChar('G', pos3);
    }
    Timer_A_initUpMode(TIMER_A1_BASE, &initUpParam_display); //start timer
    __bis_SR_register(LPM3_bits | GIE);         // enter LPM3 (execution stops)
    __no_operation();

    displayScrollText("GAIN"); //PERIODIC INTERFERENCE
    if(pg==10)
    {
        showDigit(1, pos1);
        LCDM11 |= 0x01;
        showDigit(0, pos2);
     }
    else
    {
        showDigitt(0, pos1);
        showDigit(pg)
        LCDM11 |= 0x01;
    }




    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE); //start timer
    __bis_SR_register(LPM3_bits | GIE);         // enter LPM3 (execution stops)
    __no_operation();

}// change mode at the and!!

#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER0_A1_ISR (void)
{
    Timer_A_stop(TIMER_A1_BASE);
    __bic_SR_register_on_exit(LPM3_bits);                // exit LPM3
}
