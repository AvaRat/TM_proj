#include <msp430.h>
#include <driverlib.h>
#include <stdio.h>
#include <string.h>


#include "hal_LCD.h"
#include "app_logic.h"
#include "uart.h"
#include "system.h"
#include "system_headers.h"

//application global variables
volatile unsigned char app_state = STARTUP;


//system global variables
volatile int mode;  // HARMONICAL OR ECG
volatile int noise_max = 7;      // max wielkość szumów jako wykladnik potegi 2 (np jesli 7 to 2^7 = 128)
volatile int noise_period_ms = 100;



Calendar currentTime;

void Init_GPIO(void);

int main(void)
{
    // czesc aplikacji
    // Stop watchdog timer
    WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);

    PMM_unlockLPM5();
   // Init_Clock();
    Init_LCD();

/*
    // Configure RTC_C
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
    RTCCTL0_L = RTCTEVIE | RTCRDYIE;        // enable RTC read ready interrupt
                                            // enable RTC time event interrupt

    RTCCTL1 = RTCBCD | RTCHOLD | RTCMODE;   // RTC enable, BCD mode, RTC hold

    RTCYEAR = 0x2010;                       // Year = 0x2010
    RTCMON = 0x4;                           // Month = 0x04 = April
    RTCDAY = 0x05;                          // Day = 0x05 = 5th
    RTCDOW = 0x01;                          // Day of week = 0x01 = Monday
    RTCHOUR = 0x10;                         // Hour = 0x10
    RTCMIN = 0x32;                          // Minute = 0x32
    RTCSEC = 0x01;                          // Seconds = 0x45

    RTCCTL1 &= ~(RTCHOLD);                  // Start RTC
    */


/*
    Calendar currentTime;
    //Setup Current Time for Calendar
    currentTime.Seconds    = 0x00;
    currentTime.Minutes    = 0x26;
    currentTime.Hours      = 0x13;
    currentTime.DayOfWeek  = 0x03;
    currentTime.DayOfMonth = 0x20;
    currentTime.Month      = 0x07;
    currentTime.Year       = 0x2011;
    //check if restart occured
    RTC_C_initCalendar(RTC_C_BASE,
        &currentTime,
        RTC_C_FORMAT_BCD);
    RTC_C_startClock(RTC_C_BASE);
*/
    //newTime.Seconds = RTCSEC;

    //no restart detected
    //show greetings + quick manual

    displayScrollText("ELO MORDO"); //"WELCOME IN TMPROJ DEMO.     LEFT BUTTON : SETTINGS     RIGHT BUTTON : SAVE PARAMS    HAVE FUN"

    Init_GPIO();
    //init_clock();
    initClockTo16MHz();
    init_uartA0();
    volatile int smclk_freq = CS_getSMCLK();
    volatile int aclk_freq = CS_getACLK();
    volatile int mclk_freq = CS_getMCLK();


    // czesc michala (systemowa)
    set_params(noise_period_ms);
    init_DMA(&input_signal);
    //scale_sine_tab(noise_max);
    init_ADC12();
    //init_pin();
    //initClockTo16MHz();
    //initUART();
    init_timerA_for_RNG(noise_period_ms);

    __enable_interrupt();

    while(1)
    {
        switch(app_state)
        {
        case STARTUP:
            display_params(10,10, EKG);
            //
            break;
        case NORMAL:
            // init ADC12
            ADC12CTL0 |= ADC12ENC;                          // Enable conversion
            init_timerB0_for_ADC();
            while(app_state==NORMAL)
                display_params(params.periodic_gain, params.impulsive_gain, params.periodic_interf_type);
            ADC12CTL0 &= ~ADC12ENC;
            break;
        case SETTINGS:
            init_settings();
            break;
        }
    }
    /* wait for button press:
     * s1 -> open settings
     * s2 -> save params
     */
}



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
    GPIO_setAsOutputPin(GPIO_PORT_P9, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

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

    // set pin9.3 as input for for ADC12 (A11 channel)
    GPIO_setAsPeripheralModuleFunctionInputPin(
           GPIO_PORT_P9,
           GPIO_PIN3,
           GPIO_TERNARY_MODULE_FUNCTION
           );
    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
}



/*
void init_pin(void)
{
    // ustawienie diody:
    P1OUT &= ~BIT0;                           // P1.0 clear
    P1DIR |= BIT0;                            // P1.0 output

    // zegar:
    P1SEL1 &= ~BIT4;                          // P1.4/TB1 option select
    P1SEL0 |= BIT4;                           // As above
    P1DIR |= BIT4;                            // Output direction of P1.4

    // uart:
    P2SEL1 |= BIT0 | BIT1;                    // USCI_A0 UART operation
    P2SEL0 &= ~(BIT0 | BIT1);

    // Configure PJ.5 PJ.4 for external crystal oscillator
    PJSEL0 |= BIT4 | BIT5;                    // For XT1

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

}
*/

void initClockTo16MHz()
{
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    // Clock System Setup
    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
    CSCTL1 = DCORSEL | DCOFSEL_4;             // Set DCO to 16MHz
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers

    CSCTL4 &= ~LFXTOFF;
    do
    {
        CSCTL5 &= ~LFXTOFFG;                      // Clear XT1 fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

    CSCTL0_H = 0;                             // Lock CS registers
}


