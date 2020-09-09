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
        TIMER_A_CLOCKSOURCE_DIVIDER_10,          // ACLK/4 = 2MHz
        6000,                                  // 15ms debounce period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR,                       // Clear value
        true                                    // Start Timer
};


// TimerB0 UpMode Configuration Parameter
// debounce timer params
Timer_B_initUpModeParam initUpParam_B0 =
{
        TIMER_B_CLOCKSOURCE_ACLK,              // SMCLK Clock Source
        TIMER_B_CLOCKSOURCE_DIVIDER_10,          // SMCLK/4 = 2MHz
        50,                                  // 15ms debounce period
        TIMER_B_TBIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_B_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_B_DO_CLEAR,                       // Clear value
        true                                    // Start Timer
};

/*
 * pg -> periodic interference gain (0-10)
 * ig -> impulsive interference gain (0-10)
 */
void display_params(int pg, int ig, int periodic_interf_type)
{
    int old_state = app_state;

    //displayScrollText("SAVED TIME");
    //newTime = RTC_C_getCalendarTime(RTC_C_BASE);
    /*
    int actual_time = newTime.Seconds;
    unsigned int number[2];
    number[0] = actual_time/10;
    number[1] = actual_time%10;
    showDigit(number[0], pos5);
    showDigit(number[1], pos6);
    */

    LPM3_delay();

    WDTCTL = WDT_SETUP;
    displayScrollText("SIGNAL TYPE");
    if(app_state != old_state)
        return;
    if(periodic_interf_type==SIN)
    {
        displayWord("SIN", 3);
    }else
    {
       displayWord("EKG", 3);
    }
    LPM3_delay();

    WDTCTL = WDT_SETUP;
    displayScrollText("HARMONIC NOISE GAIN"); //PERIODIC INTERFERENCE GAIN
    if(app_state != old_state)
        return;
    displayNumber(pg, pos5, pos6);
    LPM3_delay();

    WDTCTL = WDT_SETUP;
    displayScrollText("IMPULSIVE NOISE GAIN");
    if(app_state != old_state)
        return;
    displayNumber(ig, pos5, pos6);
    LPM3_delay();

}// change mode at the and!!

void init_settings()
{
    displayScrollText("SETTINGS"); // LEFT BUTTON ACCEPT   RIGHT BUTTON NEXT VALUE
    //displayScrollText("CHOOSE PERIODIC INTERFERENCE TYPE");
    app_state = SETTINGS_PERIODIC_SIN;
    gain_value = 0;
    show_text = 1;

    while(app_state != NORMAL && app_state != WATCHDOG_TEST)
    {
        switch(app_state)
        {
            case SETTINGS_PERIODIC_SIN:
            {
                clearLCD();
                displayWord("SIN", 3);
                break;
            }
            case SETTINGS_PERIODIC_EKG:
            {
                displayWord("EKG", 3);
                break;
            }
            case SETTINGS_WATCHDOG:
            {
                displayWord("WATCHD", 6);
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
                displayNumber(gain_value, pos5, pos6);
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
                displayNumber(gain_value, pos5, pos6);
                break;
            }
        }

        WDTCTL = WDT_SETUP;
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
                        params.signal_type = app_state;
                        app_state = SETTINGS_PERIODIC_GAIN;
                        break;
                    case SETTINGS_PERIODIC_EKG:
                        params.signal_type = app_state;
                        app_state = SETTINGS_PERIODIC_GAIN;
                        break;
                    case SETTINGS_WATCHDOG:
                        app_state = WATCHDOG_TEST;
                        break;
                    case SETTINGS_PERIODIC_GAIN:
                        params.harmonic_gain = gain_value;
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
                Timer_B_initUpMode(TIMER_B0_BASE, &initUpParam_B0);
                //Timer_B_startCounter(TIMER_B0_BASE, TIMER_B_UP_MODE);
                __bic_SR_register_on_exit(LPM3_bits);            // exit LPM3
            }
            break;
        case P1IV_P1IFG2 :    // Button S2 pressed
            P9OUT |= BIT7;    // Turn LED2 On
            if ((S2buttonDebounce) == 0)
            {
                S2buttonDebounce = 1;
                //char str[50];
                //sprintf(str, "signalFreq:%d\n", 99);
                //transmitString(str);

                switch(app_state)
                {
                    case NORMAL:
                        app_state = RTC_TRIGGER;
                        break;
                    case STARTUP:
                        break;
                    case SETTINGS_PERIODIC_SIN:
                        app_state = SETTINGS_PERIODIC_EKG;
                        break;
                    case SETTINGS_PERIODIC_EKG:
                        //app_state = SETTINGS_PERIODIC_SIN;
                        app_state = SETTINGS_WATCHDOG;
                        break;
                    case SETTINGS_WATCHDOG:
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
                Timer_B_initUpMode(TIMER_B0_BASE, &initUpParam_B0);
                //Timer_B_startCounter(TIMER_B0_BASE, TIMER_B_UP_MODE);
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
#pragma vector = TIMER0_B0_VECTOR
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
    //Timer_B_stop(TIMER_B0_BASE);
    if(app_state==NORMAL)
        init_timerB0_for_ADC();
}

void LPM3_delay()
{
    Timer_A_initUpMode(TIMER_A3_BASE, &initUpParam_display); //start timer
    __bis_SR_register(LPM3_bits | GIE);         // enter LPM3 (execution stops)
    __no_operation();
}

char *day_string(int dow)
{
    switch(dow)
    {
    case 1:
        return "MON";
    case 2:
        return "TUE";
    case 3:
        return "WED";
    case 4:
        return "THU";
    case 5:
        return "FRI";
    case 6:
        return "SAT";
    case 7:
        return "SUN";
    }
    return NULL;
}
