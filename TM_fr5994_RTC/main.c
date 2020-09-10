#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driverlib.h"

//flaga, ze chcemy nadpisac RTC nowa data
#define SET_CLOCK 1

#define WDT_SETUP 0x5A2B // 1 sekunda, zegar ACLK
#define STOP_WATCHDOG 0x5A80 // Stop the watchdog

// TimerA0 UpMode Configuration Parameter
Timer_A_initUpModeParam initUpParam=
{
        TIMER_A_CLOCKSOURCE_ACLK,              // ACLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_10,          // ACLK/4 = 2MHz
        100,                                  // 15ms debounce period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR,                       // Clear value
        true                                    // Start Timer
};


void transmitString(char *str);
void init_uartA3(void);
char *day_string(int dow);
void init_clock(void);
void init_RTC(void);
void blink_green_LED(void);
void blink_red_LED(void);


// * main.c

int main(void)
{
    WDTCTL = WDT_SETUP;
    //WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    init_clock();


    // GPIOs for RTC module trigger (P1.3 -> RTC_module,     P2.7 -> FR6989 trigger)
    // RTC_module sends uart data to fr6989's USCI_A0 (P2.1) when P2.7 transition from HIGH to LOW
    GPIO_selectInterruptEdge(GPIO_PORT_P1, GPIO_PIN3, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_clearInterrupt(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN3);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN1);

    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);

    init_uartA3();
    init_RTC();

#ifdef SET_CLOCK
    RTCCTL13 = RTCBCD | RTCMODE | RTCHOLD;  // RTC enable, BCD mode, RTC hold
    RTCYEAR = 0x2020;                       // Year = 0x2010
    RTCMON = 0x9;                           // Month = 0x04 = April
    RTCDAY = 0x9;                          // Day = 0x05 = 5th
    RTCDOW = 0x03;                          // Day of week = 0x01 = Monday
    RTCHOUR = 0x0;                         // Hour = 0x10
    RTCMIN = 0x1E;                          // Minute = 0x32
    RTCSEC = 0x00;                          // Seconds = 0x45
#else
    RTCCTL13 = RTCBCD | RTCMODE;
#endif
    RTCCTL13 &= ~(RTCHOLD);                 // Start RTC

    while(1)
    {
        __bis_SR_register(LPM3_bits | GIE);         // enter LPM3 (execution stops)
        __no_operation();
        WDTCTL = WDT_SETUP;
        blink_red_LED();
    }
}






void init_RTC(void)
{
     // Disable the GPIO power-on default high-impedance mode to activate
     // previously configured port settings
     PM5CTL0 &= ~LOCKLPM5;

     // Configure RTC_C
     RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
     RTCCTL0_L = RTCTEVIE_L | RTCRDYIE_L; ;    // enable RTC read ready interrupt

     RTCCTL13 = RTCBCD | RTCHOLD | RTCMODE;  // RTC enable, BCD mode, RTC hold
}

void init_clock(void)
{
    PJSEL0 = BIT4 | BIT5;                   // Initialize LFXT pins
    PM5CTL0 &= ~LOCKLPM5;

    CSCTL0_H = CSKEY_H;                     // Unlock CS registers

    CSCTL4 &= ~LFXTOFF;                     // Enable LFXT
    // Startup clock system with max DCO setting ~8MHz
    CSCTL1 = DCOFSEL_3 | DCORSEL;           // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
    CSCTL4 &= ~LFXTOFF;                     // Enable LFXT

    // Configure LFXT 32kHz crystal
    do
    {
      CSCTL5 &= ~LFXTOFFG;                  // Clear LFXT fault flag
      SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);              // Test oscillator fault flag
    CSCTL0_H = 0;                           // Lock CS registers
}

void init_uartA3(void)
{
    // Configure GPIO
    P6SEL1 &= ~(BIT0 | BIT1);
    P6SEL0 |= (BIT0 | BIT1);                // USCI_A3 UART operation on P6.0-> TX    P6.1->RX

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Configure USCI_A3 for UART mode
    UCA3CTLW0 = UCSWRST;                    // Put eUSCI in reset
    UCA3CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK
    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // User's Guide Table 21-4: UCBRSx = 0x04
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA3BRW = 52;                           // 8000000/16/9600
    UCA3MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
    UCA3CTLW0 &= ~UCSWRST;                  // Initialize eUSCI
    UCA3IE |= UCRXIE;                       // Enable USCI_A3 RX interrupt
}

// function from OutOfTheBox example
// Transmits string buffer through EUSCI UART
void transmitString(char *str)
{
    unsigned int i = 0;
    for(i = 0; i < strlen(str); i++)
    {
        if (str[i] != 0)
        {
            // Transmit Character
            while (EUSCI_A_UART_queryStatusFlags(EUSCI_A3_BASE, EUSCI_A_UART_BUSY));
            EUSCI_A_UART_transmitData(EUSCI_A3_BASE, str[i]);
        }
    }
}


// Port 1 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt void port1_isr_handler(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) port1_isr_handler (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(P1IV, P1IV__P1IFG7))
    {
        case P1IV__NONE:    break;          // Vector  0:  No interrupt
        case P1IV__P1IFG0:  break;          // Vector  2:  P1.0 interrupt flag
        case P1IV__P1IFG1:  break;               // Vector  4:  P1.1 interrupt flag
        case P1IV__P1IFG2:  break;          // Vector  6:  P1.2 interrupt flag
        case P1IV__P1IFG3:           // Vector  8:  P1.3 interrupt flag
            // this part should be on RTC board
            //P1OUT ^= BIT0;  // trigger red LED
            blink_green_LED();
            while(!RTCRDY) {};
            int hour = RTCHOUR;
            int min = RTCMIN;
            int dow = RTCDOW;
            char ch[20];
            sprintf(ch, "%02d%02d%s\r\n", hour, min, day_string(dow));
            transmitString(ch);
        break;
        case P1IV__P1IFG4:  break;          // Vector  10:  P1.4 interrupt flag
        case P1IV__P1IFG5:  break;          // Vector  12:  P1.5 interrupt flag
        case P1IV__P1IFG6:  break;          // Vector  14:  P1.6 interrupt flag
        case P1IV__P1IFG7:  break;          // Vector  16:  P1.7 interrupt flag
        default: break;
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_C_VECTOR
__interrupt void RTC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(RTC_C_VECTOR))) RTC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(RTCIV, RTCIV__RT1PSIFG))
    {
        case RTCIV__NONE:      break;       // No interrupts
        case RTCIV__RTCOFIFG:  break;       // RTCOFIFG
        case RTCIV__RTCRDYIFG:              // RTCRDYIFG
            __bic_SR_register_on_exit(LPM3_bits);            // exit LPM3
            break;
        case RTCIV__RTCTEVIFG:              // RTCEVIFG
            __no_operation();               // Interrupts every minute - SET BREAKPOINT HERE
            break;
        case RTCIV__RTCAIFG:   break;       // RTCAIFG
        case RTCIV__RT0PSIFG:  break;       // RT0PSIFG
        case RTCIV__RT1PSIFG:  break;       // RT1PSIFG
        default: break;
    }
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

void blink_green_LED()
{
    P1OUT |= BIT0;
    Timer_A_initUpMode(TIMER_A3_BASE, &initUpParam); //start timer

}

#pragma vector = TIMER3_A0_VECTOR
__interrupt void TIMER3_A_ISR (void)
{
    Timer_A_stop(TIMER_A3_BASE);
    P1OUT &= ~BIT0;
}
void blink_red_LED()
{
    P1OUT |= BIT1;
    Timer_A_initUpMode(TIMER_A2_BASE, &initUpParam); //start timer

}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void TIMER2_A_ISR (void)
{
    Timer_A_stop(TIMER_A2_BASE);
    P1OUT &= ~BIT1;
}
