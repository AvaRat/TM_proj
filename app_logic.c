/*
 * app_logic.c
 *
 *  Created on: Aug 31, 2020
 *      Author: marce
 */
#include "app_logic.h"

volatile unsigned char S1buttonDebounce = 0;
volatile unsigned char S2buttonDebounce = 0;
volatile unsigned int holdCount = 0;

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


// TimerA0 UpMode Configuration Parameter
Timer_A_initUpModeParam initUpParam_A0 =
{
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // SMCLK/4 = 2MHz
        30000,                                  // 15ms debounce period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR,                       // Clear value
        true                                    // Start Timer
};

/*
 * pg -> periodic interference gain (0-10)
 * ig -> impulsive interference gain (0-10)
 */
void display_params(int pg, int ig, int periodic_interf_type)
{
    int old_state = app_state;

    displayScrollText("SAVED TIME");
    //newTime = RTC_C_getCalendarTime(RTC_C_BASE);
    int actual_time = newTime.Seconds;
    unsigned int number[2];
    number[0] = actual_time/10;
    number[1] = actual_time%10;
    showDigit(number[0], pos5);
    showDigit(number[1], pos6);

    LPM3_delay();

    displayScrollText("PERIODIC INTERFERENCE TYPE");
    if(periodic_interf_type==SIN)
    {
        displayWord("SIN", 3);
    }else
    {
       displayWord("EKG", 3);
    }
    if(app_state != old_state)
        return;
    LPM3_delay();

    displayScrollText("GAIN"); //PERIODIC INTERFERENCE GAIN
    displayNumber(pg);
    if(app_state != old_state)
        return;
    LPM3_delay();

    displayScrollText("IMPULSIVE INTERFERENCE GAIN");
    displayNumber(ig);
    if(app_state != old_state)
        return;
    LPM3_delay();

}// change mode at the and!!

void init_settings()
{
    displayScrollText("SETTINGS"); // LEFT BUTTON ACCEPT   RIGHT BUTTON NEXT VALUE
    //displayScrollText("CHOOSE PERIODIC INTERFERENCE TYPE");
    app_state = SETTINGS_PERIODIC_SIN;
    gain_value = 0;
    show_text = 1;

    while(app_state != NORMAL)
    {
        switch(app_state)
        {
            case SETTINGS_PERIODIC_SIN:
            {
                displayWord("SIN", 3);
                break;
            }
            case SETTINGS_PERIODIC_EKG:
            {
                displayWord("EKG", 3);
                break;
            }
            case SETTINGS_PERIODIC_GAIN:
            {
                clearLCD();
                if(show_text)
                {
                    displayWord("P GAIN", 6);
                    LPM3_delay();
                    show_text=0;
                }
                clearLCD();
                displayNumber(gain_value);
                break;
            }
            case SETTINGS_IMPULSIVE_GAIN:
            {
                if(show_text)
                {
                    displayWord("I GAIN", 6);
                    LPM3_delay();
                    show_text=0;
                }
                clearLCD();
                displayNumber(gain_value);
                break;
            }
        }
        __bis_SR_register(LPM3_bits | GIE);         // enter LPM3 (execution stops)
        __no_operation();
    }

    displayScrollText("SAVED");
}


#pragma vector = TIMER3_A0_VECTOR
__interrupt void TIMER3_A_ISR (void)
{
    Timer_A_stop(TIMER_A3_BASE);
    __bic_SR_register_on_exit(LPM3_bits);                // exit LPM3
}


/*
 * PORT1 Interrupt Service Routine
 * Handles S1 and S2 button press interrupts
 */
#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    switch(__even_in_range(P1IV, P1IV_P1IFG7))
    {
        case P1IV_NONE : break;
        case P1IV_P1IFG0 : break;
        case P1IV_P1IFG1 :    // Button S1 pressed
            P1OUT |= BIT0;    // Turn LED1 On
            if ((S1buttonDebounce) == 0)
            {
                // Set debounce flag on first high to low transition
                S1buttonDebounce = 1;

                switch(app_state)
                {
                    case STARTUP:
                        app_state = SETTINGS;
                        break;
                    case NORMAL:
                        app_state = SETTINGS;
                        break;
                    case SETTINGS_PERIODIC_SIN:
                        params.periodic_interf_type = app_state;
                        app_state = SETTINGS_PERIODIC_GAIN;
                        break;
                    case SETTINGS_PERIODIC_EKG:
                        params.periodic_interf_type = app_state;
                        app_state = SETTINGS_PERIODIC_GAIN;
                        break;
                    case SETTINGS_PERIODIC_GAIN:
                        params.periodic_gain = gain_value;
                        gain_value=0;
                        show_text = 1;
                        app_state = SETTINGS_IMPULSIVE_GAIN;
                        break;
                    case SETTINGS_IMPULSIVE_GAIN:
                        params.impulsive_gain = gain_value;
                        gain_value=0;
                        app_state = NORMAL; // end of settings
                        break;
                }
                // Start debounce timer
                Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam_A0);
                __bic_SR_register_on_exit(LPM3_bits);            // exit LPM3
            }
            break;
        case P1IV_P1IFG2 :    // Button S2 pressed
            P9OUT |= BIT7;    // Turn LED2 On
            if ((S2buttonDebounce) == 0)
            {
                // Set debounce flag on first high to low transition
                S2buttonDebounce = 1;

                switch(app_state)
                {
                    case NORMAL:
                        //newTime = RTC_C_getCalendarTime(RTC_C_BASE);
                        newTime.Seconds = RTCSEC;
                        break;
                    case STARTUP:
                        //newTime = RTC_C_getCalendarTime(RTC_C_BASE);
                        newTime.Seconds = RTCSEC;
                        break;
                    case SETTINGS_PERIODIC_SIN:
                        app_state = SETTINGS_PERIODIC_EKG;
                        break;
                    case SETTINGS_PERIODIC_EKG:
                        app_state = SETTINGS_PERIODIC_SIN;
                        break;
                    case SETTINGS_PERIODIC_GAIN:
                        if(gain_value<10)
                            gain_value++;
                        else
                            gain_value = 0;
                        break;
                    case SETTINGS_IMPULSIVE_GAIN:
                        if(gain_value<10)
                            gain_value++;
                        else
                            gain_value = 0;
                        break;
                }

                // Start debounce timer
                Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam_A0);
                __bic_SR_register_on_exit(LPM3_bits);            // exit LPM3
            }
            break;
        case P1IV_P1IFG3 : break;
        case P1IV_P1IFG4 : break;
        case P1IV_P1IFG5 : break;
        case P1IV_P1IFG6 : break;
        case P1IV_P1IFG7 : break;
    }
}


/*
 * Timer A0 Interrupt Service Routine
 * Used as button debounce timer
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void)
{
    // Button S1 released
    if (P1IN & BIT1)
    {
        S1buttonDebounce = 0;                                   // Clear button debounce
        P1OUT &= ~BIT0;
    }

    // Button S2 released
    if (P1IN & BIT2)
    {
        S2buttonDebounce = 0;                                   // Clear button debounce
        P9OUT &= ~BIT7;
    }
    //Timer_A_stop(TIMER_A0_BASE);
}

void LPM3_delay()
{
    Timer_A_initUpMode(TIMER_A3_BASE, &initUpParam_display); //start timer
    __bis_SR_register(LPM3_bits | GIE);         // enter LPM3 (execution stops)
    __no_operation();
}
