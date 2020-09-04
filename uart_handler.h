#define SMCLK_11500     0
#define SMCLK_9600      1
#define ACLK_9600       2

#define UART_MODE       SMCLK_9600

void initUART()
{
    // Configure USCI_A0 for UART mode
    UCA0CTLW0 = UCSWRST;                      // Put eUSCI in reset
#if UART_MODE == SMCLK_115200

    UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
    // Baud Rate calculation
    // 16000000/(16*115200) = 8.6805
    // Fractional portion = 0.6805
    // Use Table 24-5 in Family User Guide
    UCA0BR0 = 8;                               // 16000000/16/9600
    UCA0BR1 = 0x00;
    UCA0MCTLW |= UCOS16 | UCBRF_10 | 0xF700;   //0xF700 is UCBRSx = 0xF7

#elif UART_MODE == SMCLK_9600

    UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
    // Baud Rate calculation
    // 16000000/(16*9600) = 104.1667
    // Fractional portion = 0.1667
    // Use Table 24-5 in Family User Guide
    UCA0BR0 = 104;                            // 16000000/16/9600
    UCA0BR1 = 0x00;
    UCA0MCTLW |= UCOS16 | UCBRF_2 | 0xD600;   //0xD600 is UCBRSx = 0xD6

#elif UART_MODE == ACLK_9600

    UCA0CTLW0 |= UCSSEL__ACLK;               // CLK = ACLK
    // Baud Rate calculation
    // 32768/(9600) = 3.4133
    // Fractional portion = 0.4133
    // Use Table 24-5 in Family User Guide
    UCA0BR0 = 3;                             // 32768/9600
    UCA0BR1 = 0x00;
    UCA0MCTLW |= 0x9200;    //0x9200 is UCBRSx = 0x92

#else
    # error "Please specify baud rate to 115200 or 9600"
#endif

    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}


void send_uart_msg(char * str)
{

    unsigned int i = 0;
    for(i = 0; i < strlen(str); i++)
    {
        if (str[i] != 0)
        {
            // Transmit Character
            //while (EUSCI_A_UART_queryStatusFlags(EUSCI_A1_BASE, EUSCI_A_UART_BUSY));
            //EUSCI_A_UART_transmitData(EUSCI_A1_BASE, str[i]);
            while(!(UCTXIFG & UCA0IFG)); // Wait for TX buffer to be ready for new data

            UCA0TXBUF = str[i]; //Write the character at the location specified py the pointer
        }
    }
        //int i = PULSE_BITS - 1;
    //UCA0TXBUF = msg;
}


/****************************************************************
 * Przerwanie UART - odebranie sygna³u
 ***************************************************************/
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
      while(!(UCA0IFG&UCTXIFG));
      // xd = 1;
      //P1OUT |= BIT0;
      __no_operation();
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}
