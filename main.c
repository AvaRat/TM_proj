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
volatile char time_string[20];


//system global variables
volatile int mode;  // HARMONICAL OR ECG
volatile int harmonic_noise_max;      // max wielkość szumów jako wykladnik potegi 2 (np jesli 7 to 2^7 = 128)
volatile int impulsive_noise_max;     // max wielkość szumów jako wykladnik potegi 2 (np jesli 7 to 2^7 = 128)
volatile int noise_period_ms = 100;



Calendar currentTime;

void Init_GPIO(void);

int main(void)
{
    // czesc aplikacji
    // Stop watchdog timer
    //WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);
    WDTCTL = WDT_SETUP; // 1 sekunda, zegar ACLK

    PMM_unlockLPM5();
   // Init_Clock();
    Init_LCD();


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
            if(SYSRSTIV == 0x16)   //sysrstiv pokazuje przyczyne ostatniego resetu
             {
                displayScrollText("WATCHDOG RESET DETECTED");
                //WDTCTL = STOP_WATCHDOG;
             }
            displayScrollText("NORMAL STARTUP");
            display_params(10,10, EKG);
            //
            break;
        case NORMAL:
            // init ADC12
            ADC12CTL0 |= ADC12ENC;                          // Enable conversion
            init_timerB0_for_ADC();
            if(params.signal_type==SIN)
            {
                showChar('S', pos1);
                showChar('I', pos2);
            }
            else
            {
                showChar('E', pos1);
                showChar('K', pos2);
            }
            displayNumber(params.harmonic_gain, pos3, pos4);
            displayNumber(params.impulsive_gain, pos5, pos6);
            __bis_SR_register(LPM3_bits | GIE);         // enter LPM3 (execution stops)
            __no_operation();

            /*
            while(app_state==NORMAL)
                display_params(params.harmonic_gain, params.impulsive_gain, params.signal_type);
                */
            ADC12CTL0 &= ~ADC12ENC;
            break;
        case SETTINGS:
            init_settings();
            break;
        case WATCHDOG:
            while(1){}; // test, if watchdog timer detects infinite loop, and restarts device
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


