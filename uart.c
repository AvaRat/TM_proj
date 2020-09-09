/*
 * uart.c
 *
 *  Created on: Sep 2, 2020
 *      Author: marce
 */
#include "uart.h"

bool rxStringReady = false;

void init_uartA0(void)
{
    // Configure GPIO
     P2SEL0 |= BIT0 | BIT1;                    // USCI_A0 UART operation
     P2SEL1 &= ~(BIT0 | BIT1);
     PJSEL0 |= BIT4 | BIT5;                    // For XT1

     // Disable the GPIO power-on default high-impedance mode to activate
     // previously configured port settings
     PM5CTL0 &= ~LOCKLPM5;
    // Configure USCI_A0 for UART mode
    UCA0CTLW0 = UCSWRST;                      // Put eUSCI in reset
    UCA0CTLW0 |= UCSSEL__ACLK;                // CLK = ACLK
    UCA0BR0 = 3;                              // 9600 baud
    UCA0MCTLW |= 0x5300;                      // 32768/9600 - INT(32768/9600)=0.41
                                              // UCBRSx value = 0x53 (See UG)
    UCA0BR1 = 0;
    UCA0CTL1 &= ~UCSWRST;                     // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}

void init_clock(void)
{
    // XT1 Setup
    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
    CSCTL4 &= ~LFXTOFF;
    do
    {
      CSCTL5 &= ~LFXTOFFG;                    // Clear XT1 fault flag
      SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
    CSCTL0_H = 0;                             // Lock CS registers
}

// sends virtual button on P2.7
void trigger_RTC()
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
    //LPM3_delay();
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
}

// function from OutOfTheBox example
// Transmits string buffer through EUSCI UART
void transmitString(char *str)
{
    WDTCTL = WDT_SETUP;
    unsigned int i = 0;
    for(i = 0; i < strlen(str); i++)
    {
        if (str[i] != 0)
        {
            // Transmit Character
            while (EUSCI_A_UART_queryStatusFlags(EUSCI_A0_BASE, EUSCI_A_UART_BUSY));
            EUSCI_A_UART_transmitData(EUSCI_A0_BASE, str[i]);
        }
    }
}

// function from OutOfTheBox example
// Receives strings terminated with \n
void receiveString(char data) {
    static bool rxInProgress = false;
    static unsigned int charCnt = 0;

    //volatile int num = atoi(data);

    if(!rxInProgress){
        if ((data != '\n') ){
            rxInProgress = true;
            charCnt = 0;
            rxString[charCnt] = data;
        }
    }else{ // in progress
        charCnt++;
        if((data != '\n')){
            if (charCnt >= MAX_STR_LEN){
                rxInProgress = false;
            }else{
                rxString[charCnt] = data;
            }
        }else{
            rxInProgress = false;
            rxString[charCnt] = '\0';
            // String receive complete
            rxStringReady = true;
        }
    }
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
        GPIO_toggleOutputOnPin(GPIO_PORT_P9, GPIO_PIN7);
        //char rs[20];
        //strncpy(rs, rxString, 20);
        receiveString(UCA0RXBUF);
        if(app_state == RTC_TRIGGER && rxStringReady == true)
        {
            __bic_SR_register_on_exit(LPM3_bits);            // exit LPM3
            rxStringReady = false;
        }
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A1_VECTOR))) USCI_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
        GPIO_toggleOutputOnPin(GPIO_PORT_P9, GPIO_PIN7);
//      while(!(UCA0IFG&UCTXIFG));
//      UCA0TXBUF = UCA0RXBUF;
//      __no_operation();
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}

