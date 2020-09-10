#include <stdio.h>
#include <stdlib.h>
#include <msp430.h>

#define SAMPLING_PERIOD 10          // ms

unsigned int DAC_data = 0;
char DAC_data_str[6] = {0};


#define SMCLK_115200    0
#define SMCLK_9600      1
#define ACLK_9600       2

#define UART_MODE       ACLK_9600

void init_UART()
{
    // Configure USCI_A0 for UART mode
    UCA0CTLW0 |= UCSWRST;                      // Put eUSCI in reset
#if UART_MODE == SMCLK_115200

    UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
    // Baud Rate Setting
    // Use Table 21-5
    UCA0BRW = 8;
    UCA0MCTLW |= UCOS16 | UCBRF_10 | 0xF700;   //0xF700 is UCBRSx = 0xF7

#elif UART_MODE == SMCLK_9600

    UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
    // Baud Rate Setting
    // Use Table 21-5
    UCA0BRW = 104;
    UCA0MCTLW |= UCOS16 | UCBRF_2 | 0xD600;   //0xD600 is UCBRSx = 0xD6

#elif UART_MODE == ACLK_9600

    UCA0CTLW0 |= UCSSEL__ACLK;               // CLK = ACLK
    // Baud Rate calculation
    // 32768/(9600) = 3.4133
    // Fractional portion = 0.4133
    // Use Table 24-5 in Family User Guide
    UCA0BRW = 3;
    UCA0MCTLW |= 0x9200;    //0x9200 is UCBRSx = 0x92

#else
    # error "Please specify baud rate to 115200 or 9600"
#endif

    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE; // | UCTXCPTIE;             // Enable USCI_A0 RX interrupt | //transmit buffer empty ie
}

void init_reference_module() {
    // Configure reference module
    PMMCTL0_H = PMMPW_H;                      // Unlock the PMM registers
    PMMCTL2 = INTREFEN | REFVSEL_2;           // Enable internal 2.5V reference

    while(!(PMMCTL2 & REFGENRDY));            // Poll till internal reference settles
}

void init_GPIO()
{
    // ustawienie diody:
    P1OUT &= ~BIT0;                           // P1.0 clear
    P1DIR |= BIT0;                            // P1.0 output

    // Configure GPIO
    P1SEL1 &= ~(BIT6 | BIT7);                 // USCI_A0 UART operation
    P1SEL0 |= BIT6 | BIT7;

    P1SEL0 |= BIT1;                           // Select P1.1 as OA0O function
    P1SEL1 |= BIT1;                           // OA is used as buffer for DAC

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;
}

void init_SAC() {

    SAC0DAC = DACSREF_1 + DACLSEL_2 + DACIE;  // Select int Vref as DAC reference
    SAC0DAT = DAC_data;                       // Initial DAC data
    SAC0DAC |= DACEN;                         // Enable DAC

    SAC0OA = NMUXEN + PMUXEN + PSEL_1 + NSEL_1;//Select positive and negative pin input
    SAC0OA |= OAPM;                            // Select low speed and low power mode
    SAC0PGA = MSEL_1;                          // Set OA as buffer mode
    SAC0OA |= SACEN + OAEN;                    // Enable SAC and OA

    // Use TB2.1 as DAC hardware trigger

    TB2CCTL1 = OUTMOD_6;                       // TBCCR1 toggle/set // by³o 6
    TB2CCR0 =  (2000) * SAMPLING_PERIOD;       // 2000 = 16000 / 8
    TB2CCR1 =  (1000) * SAMPLING_PERIOD;       // TBCCR1 PWM duty cycle

    TB0CTL |= ID_3;                            // dzielenie przez 8

    TB2CTL = TBSSEL__SMCLK | MC_1 | TBCLR;     // SMCLK, up mode, clear TBR


}

void init_clock_to_16mhz()
{
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    __bis_SR_register(SCG0);    // disable FLL
    CSCTL3 |= SELREF__REFOCLK;  // Set REFO as FLL reference source
    CSCTL0 = 0;                 // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);     // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_5;        // Set DCO = 16MHz
    CSCTL2 = FLLD_0 + 487;      // set to fDCOCLKDIV = (FLLN + 1)*(fFLLREFCLK/n)
                                //                   = (487 + 1)*(32.768 kHz/1)
                                //                   = 16 MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                        // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));      // FLL locked

#if UART_MODE == ACLK_9600
    P2SEL1 |= BIT6 | BIT7;                  // P2.6~P2.7: crystal pins
    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);      // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);               // Test oscillator fault flag

    __bis_SR_register(SCG0);                // Disable FLL
    CSCTL3 = SELREF__XT1CLK;                // Set XT1 as FLL reference source
    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;  // set ACLK = XT1CLK = 32768Hz
                                               // DCOCLK = MCLK and SMCLK source
#else
    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK;
#endif
}

/*
 * UART z przerwanianmi: zmienne globalne i funkcja
 */

char msg[6];
volatile unsigned int msg_send_counter = 0;

inline void start_sending_12bit_via_uart(int number) {

    number = number & 0xFFF;
    sprintf(msg, "%d\r\n", number);

    //if_to_receive_next_UCTXI = 1;

    if (!(UCTXIFG & UCA0IFG)) {           // jeœli nie mo¿na wys³aæ to zezwol na rpzerwania
        UCA0IE |= UCTXIE;                // to nic, wysle jak przyjdzie przerwanie o zwolnieniu bufora
        return;
    }
    else {
        UCA0TXBUF = msg[msg_send_counter];
        msg_send_counter++;         // wyslij pierwszy znak
        UCA0IE |= UCTXIE;           // umozliw przerwania
    }
}

inline void send_12bit_via_uart(int number)
{
    number = number & 0xFFF;    // int -> 12 bits
    char msg[6];
    sprintf(msg, "%d\r\n", number);

    unsigned int i = 0;
    for(i = 0; i < 6; i++)
    {
        if (msg[i] != 0)
        {
            // Transmit Character
            while(!(UCTXIFG & UCA0IFG)); // Wait for TX buffer to be ready for new data

            UCA0TXBUF = msg[i]; //Write the character at the location specified by the pointer
            __no_operation();
        }
    }
    __no_operation();
}

inline void receive_12bit_via_uart() {
    static unsigned int i = 0;
    DAC_data_str[i] = UCA0RXBUF; // & 0xFF; 0123'\r''\n'
    __no_operation();
    i++;
    if (i == 6 || DAC_data_str[i-1] == '\n') {
        i = 0;
        DAC_data = atoi(DAC_data_str) & 0xFFF;      // ensure that number is 12 bit
        __no_operation();
        for (i = 0; i < 7; i++) {
            DAC_data_str[i] = 0;
        }
        i = 0;
        //P1OUT ^= BIT0;
    }
}


int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watch dog timer

  init_GPIO();
  init_reference_module();
  init_clock_to_16mhz();
  init_UART();
  init_SAC();

  //UCA0TXBUF = 0x03;
  // bit parity

  PM5CTL0 &= ~LOCKLPM5;                     // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

  //__enable_interrupt();
  //__delay_cycles(100000);

  //start_sending_12bit_via_uart(1234);

#if UART_MODE == ACLK_9600
    __bis_SR_register(LPM3_bits + GIE);       // Since ACLK is source, enter LPM3, interrupts enabled
#else
    __bis_SR_register(LPM0_bits + GIE);       // Since SMCLK is source, enter LPM0, interrupts enabled
#endif
  __no_operation();                         // For debugger
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = SAC0_SAC2_VECTOR
__interrupt void SAC0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(SAC0_SAC2_VECTOR))) SAC0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(SAC0IV,SACIV_4))
  {
    case SACIV_0: break;
    case SACIV_2: break;
    case SACIV_4:

        SAC0DAT = DAC_data & 0xFFF;

        break;
    default: break;
  }
  __no_operation();
}

//******************************************************************************
// UART Interrupt ***********************************************************
//******************************************************************************

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
    case USCI_UART_UCRXIFG:               // przyjscie wiadomosci uart
      while(!(UCA0IFG&UCTXIFG));
      // P1OUT ^= BIT0;
      receive_12bit_via_uart();

      __no_operation();
      break;
    case USCI_UART_UCTXIFG:               // zwolnienie bufora

        UCA0TXBUF = msg[msg_send_counter];
        msg_send_counter++;

        if (msg_send_counter == 6) {
            msg_send_counter = 0;
                                    // nie chce przerwania po tym wyslaniu
            UCA0IE &= ~UCTXIE;      // zabron kolejnych takich przerwan jesli wyslano taka liczbe
        }
        break;

    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;

  }
}


