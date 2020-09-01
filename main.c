#include <msp430.h>
#include <driverlib.h>
#include "StopWatchMode.h"
#include "TempSensorMode.h"
#include "hal_LCD.h"

#include "app_logic.h"


#define STARTUP_MODE 0

#define SETTINGS_MODE 1
#define SETTINGS_MODE_BEGIN 2
#define SETTINGS_MODE_PERIODIC_SIN 3
#define SETTINGS_MODE_PERIODIC_EKG 4
#define SETTINGS_MODE_PERIODIC_GAIN 5
#define SETTINGS_MODE_IMPULSIVE 6
#define SETTINGS_MODE_IMPULSIVE_GAIN 7


volatile unsigned char mode = STARTUP_MODE;
volatile unsigned char stopWatchRunning = 0;
volatile unsigned char tempSensorRunning = 0;
volatile unsigned char S1buttonDebounce = 0;
volatile unsigned char S2buttonDebounce = 0;
volatile unsigned int holdCount = 0;
volatile unsigned int counter = 0;
volatile int centisecond = 0;
Calendar currentTime;

void Init_GPIO(void);
void Init_Clock(void);



int main(void)
{
    // Stop watchdog timer
    WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);
    PMM_unlockLPM5();
   // Init_Clock();
    Init_LCD();

    //check if restart occured


    //no restart detected
    //show greetings + quick manual

    displayScrollText("ELO MORDO"); //"WELCOME IN TMPROJ DEMO.     LEFT BUTTON : SETTINGS     RIGHT BUTTON : SAVE PARAMS    HAVE FUN"

    Init_GPIO();
    Init_Clock();
    __enable_interrupt();

    while(1)
    {
        switch(mode)
        {
        case STARTUP_MODE:
            display_params(1,1, EKG);
            break;
        case SETTINGS_MODE:
            displayScrollText("INTERFERANCE PARAMETERS");
        }
    /* wait for button press:
     * s1 -> open settings
     * s2 -> save params
     */
    }
}


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
 * GPIO Initialization
 */
void Init_GPIO()
{
    // Set all GPIO pins to output low to prevent floating input and reduce power consumption
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P9, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    GPIO_setAsInputPin(GPIO_PORT_P3, GPIO_PIN5);

    // Configure button S1 (P1.1) interrupt
    GPIO_selectInterruptEdge(GPIO_PORT_P1, GPIO_PIN1, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1);

    // Configure button S2 (P1.2) interrupt
    GPIO_selectInterruptEdge(GPIO_PORT_P1, GPIO_PIN2, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN2);
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN2);
    GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN2);

    // Set P4.1 and P4.2 as Secondary Module Function Input, LFXT.
    GPIO_setAsPeripheralModuleFunctionInputPin(
           GPIO_PORT_PJ,
           GPIO_PIN4 + GPIO_PIN5,
           GPIO_PRIMARY_MODULE_FUNCTION
           );

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
}
/*
 * Clock System Initialization
 */
void Init_Clock()
{
    // Set DCO frequency to default 8MHz
    CS_setDCOFreq(CS_DCORSEL_0, CS_DCOFSEL_6);

    // Configure MCLK and SMCLK to default 2MHz
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_8);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_8);

    // Intializes the XT1 crystal oscillator
    CS_turnOnLFXT(CS_LFXT_DRIVE_3);
}

/*
 * RTC Interrupt Service Routine
 * Wakes up every ~10 milliseconds to update stopwatch
 */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(RTC_VECTOR)))
#endif
void RTC_ISR(void)
{
    switch(__even_in_range(RTCIV, 16))
    {
    case RTCIV_NONE: break;      //No interrupts
    case RTCIV_RTCOFIFG: break;      //RTCOFIFG
    case RTCIV_RTCRDYIFG:             //RTCRDYIFG
        counter = RTCPS;
        centisecond = 0;
        __bic_SR_register_on_exit(LPM3_bits);
        break;
    case RTCIV_RTCTEVIFG:             //RTCEVIFG
        //Interrupts every minute
        __no_operation();
        break;
    case RTCIV_RTCAIFG:             //RTCAIFG
        __no_operation();
        break;
    case RTCIV_RT0PSIFG:
        centisecond = RTCPS - counter;
        __bic_SR_register_on_exit(LPM3_bits);
        break;     //RT0PSIFG
    case RTCIV_RT1PSIFG:
        __bic_SR_register_on_exit(LPM3_bits);
        break;     //RT1PSIFG

    default: break;
    }
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
                holdCount = 0;
                if(mode == STARTUP_MODE)
                {
                    mode = SETTINGS_MODE;
                    P1OUT &= ~BIT0;
                    displayScrollText("SETTINGS    LEFT BUTTON: ACCEPT    RIGHT BUTTON: NEXT VALUE");
                }
                // Start debounce timer
                Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam_A0);
            }
            break;
        case P1IV_P1IFG2 :    // Button S2 pressed
            P9OUT |= BIT7;    // Turn LED2 On
            if ((S2buttonDebounce) == 0)
            {
                // Set debounce flag on first high to low transition
                S2buttonDebounce = 1;
                holdCount = 0;
                switch (mode)
                {
                    case STOPWATCH_MODE:
                        // Reset stopwatch if stopped; Split if running
                        if (!(stopWatchRunning))
                        {
                            if (LCDCMEMCTL & LCDDISP)
                                LCDCMEMCTL &= ~LCDDISP;
                            else
                                resetStopWatch();
                        }
                        else
                        {
                            // Use LCD Blink memory to pause/resume stopwatch at split time
                            LCDBMEM[pos1] = LCDMEM[pos1];
                            LCDBMEM[pos1+1] = LCDMEM[pos1+1];
                            LCDBMEM[pos2] = LCDMEM[pos2];
                            LCDBMEM[pos2+1] = LCDMEM[pos2+1];
                            LCDBMEM[pos3] = LCDMEM[pos3];
                            LCDBMEM[pos3+1] = LCDMEM[pos3+1];
                            LCDBMEM[pos4] = LCDMEM[pos4];
                            LCDBMEM[pos4+1] = LCDMEM[pos4+1];
                            LCDBMEM[pos5] = LCDMEM[pos5];
                            LCDBMEM[pos5+1] = LCDMEM[pos5+1];
                            LCDBMEM[pos6] = LCDMEM[pos6];
                            LCDBMEM[pos6+1] = LCDMEM[pos6+1];
                            LCDBM3 = LCDM3;

                            // Toggle between LCD Normal/Blink memory
                            LCDCMEMCTL ^= LCDDISP;
                        }
                        break;
                    case TEMPSENSOR_MODE:
                        // Toggle temperature unit flag
                        tempUnit ^= 0x01;
                        // Update LCD when temp sensor is not running
                        if (!tempSensorRunning)
                            displayTemp();
                        break;
                }

                // Start debounce timer
                Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam_A0);
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
    // Both button S1 & S2 held down
    if (!(P1IN & BIT1) && !(P1IN & BIT2))
    {
        holdCount++;
        if (holdCount == 40)
        {
            // Stop Timer A0
            Timer_A_stop(TIMER_A0_BASE);

            // Change mode
            if (mode == STARTUP_MODE)
                mode = STOPWATCH_MODE;
            else if (mode == STOPWATCH_MODE)
            {
                mode = TEMPSENSOR_MODE;
                stopWatchRunning = 0;
                // Hold RTC
                RTC_C_holdClock(RTC_C_BASE);
            }
            else if (mode == TEMPSENSOR_MODE)
            {
                mode = STOPWATCH_MODE;
                tempSensorRunning = 0;
                // Disable ADC12, TimerA1, Internal Ref and Temp used by TempSensor Mode
                ADC12_B_disable(ADC12_B_BASE);
                ADC12_B_disableConversions(ADC12_B_BASE,true);

                Timer_A_stop(TIMER_A1_BASE);
            }
            if(mode==STOPWATCH_MODE)
                __bic_SR_register_on_exit(LPM3_bits);                // exit LPM3
        }
    }

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

    // Both button S1 & S2 released
    if ((P1IN & BIT1) && (P1IN & BIT2))
    {
        // Stop timer A0
        Timer_A_stop(TIMER_A0_BASE);
    }

    __bic_SR_register_on_exit(LPM3_bits);            // exit LPM3
}

/*
 * ADC 12 Interrupt Service Routine
 * Wake up from LPM3 to display temperature
 */
#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
    switch(__even_in_range(ADC12IV,12))
    {
    case  0: break;                         // Vector  0:  No interrupt
    case  2: break;                         // Vector  2:  ADC12BMEMx Overflow
    case  4: break;                         // Vector  4:  Conversion time overflow
    case  6: break;                         // Vector  6:  ADC12BHI
    case  8: break;                         // Vector  8:  ADC12BLO
    case 10: break;                         // Vector 10:  ADC12BIN
    case 12:                                // Vector 12:  ADC12BMEM0 Interrupt
        ADC12_B_clearInterrupt(ADC12_B_BASE, 0, ADC12_B_IFG0);
        __bic_SR_register_on_exit(LPM3_bits);   // Exit active CPU
        break;                              // Clear CPUOFF bit from 0(SR)
    case 14: break;                         // Vector 14:  ADC12BMEM1
    case 16: break;                         // Vector 16:  ADC12BMEM2
    case 18: break;                         // Vector 18:  ADC12BMEM3
    case 20: break;                         // Vector 20:  ADC12BMEM4
    case 22: break;                         // Vector 22:  ADC12BMEM5
    case 24: break;                         // Vector 24:  ADC12BMEM6
    case 26: break;                         // Vector 26:  ADC12BMEM7
    case 28: break;                         // Vector 28:  ADC12BMEM8
    case 30: break;                         // Vector 30:  ADC12BMEM9
    case 32: break;                         // Vector 32:  ADC12BMEM10
    case 34: break;                         // Vector 34:  ADC12BMEM11
    case 36: break;                         // Vector 36:  ADC12BMEM12
    case 38: break;                         // Vector 38:  ADC12BMEM13
    case 40: break;                         // Vector 40:  ADC12BMEM14
    case 42: break;                         // Vector 42:  ADC12BMEM15
    case 44: break;                         // Vector 44:  ADC12BMEM16
    case 46: break;                         // Vector 46:  ADC12BMEM17
    case 48: break;                         // Vector 48:  ADC12BMEM18
    case 50: break;                         // Vector 50:  ADC12BMEM19
    case 52: break;                         // Vector 52:  ADC12BMEM20
    case 54: break;                         // Vector 54:  ADC12BMEM21
    case 56: break;                         // Vector 56:  ADC12BMEM22
    case 58: break;                         // Vector 58:  ADC12BMEM23
    case 60: break;                         // Vector 60:  ADC12BMEM24
    case 62: break;                         // Vector 62:  ADC12BMEM25
    case 64: break;                         // Vector 64:  ADC12BMEM26
    case 66: break;                         // Vector 66:  ADC12BMEM27
    case 68: break;                         // Vector 68:  ADC12BMEM28
    case 70: break;                         // Vector 70:  ADC12BMEM29
    case 72: break;                         // Vector 72:  ADC12BMEM30
    case 74: break;                         // Vector 74:  ADC12BMEM31
    case 76: break;                         // Vector 76:  ADC12BRDY
    default: break;
    }
}
