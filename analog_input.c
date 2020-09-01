/* Program działa mniej więcej tak:
 *  - timer ustawia bit, który automatycznie budzi ADC?
 *  - ADC jest skonfigurowane tak, że jest budzone przez timer
 *  - DMA po zakończeniu pracy wysyła przerwanie, którego obsługa polega na zapaleniu - lub nie - diody
 *  - jednocześnie w przerwaniu nakładany jest na wszystko sinus zgodnie z tablicą z pliku sine_table.h
 */

#include <stdio.h>
#include <msp430.h>
#include <sine_table.h>

#define SMCLK_11500     0
#define SMCLK_9600      1
#define ACLK_9600       2

#define UART_MODE       SMCLK_9600

#define PULSE_BITS      5

volatile unsigned int pot_voltage;               // temperature measurement result storage variable
volatile unsigned int sine_counter = 0;
volatile float scale = 1;                        // skala funkcji sinus. Przy równej 1: max = 4095, min = 0
volatile int xd = 0;
int current_pulse_bit = PULSE_BITS - 1;
unsigned int pulse_amplitude = 0;
unsigned int pulse_width = 1000; // ms
unsigned int rn = 0;
unsigned long A0_ticks = 0;


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

// Konfiguracja timera, żeby co określony czas budził ADC
void timerB0_init(void)
{
    // smclk: 1048575 = 1 sec. Spowalniam pod koniec funkcji /64, więc 16384 = 1 sek // w aclk: 32 767 - tyle powinno być cykli w sekundzie, a chyba nie jest .-.
    TB0CCR0 = 16384 / 100;    //0x64;
    TB0CCR1 = 16384 / 200;        //0x32;               // to z tym na górze to znaczy chyba, że jak doliczy do 0x8000 to ustawia jakiś bit, a jak do 0xFFFE to resetuje

    TB0CCTL1 = OUTMOD_3;                       // CCR1 set/reset mode
    TB0CTL = TBSSEL_2 + MC_1 + TBCLR;          // SMCLK, Up-Mode, clear

    // można spowolnić ten timer jeszcze:
    // TB0CTL |= ID_3;                            // /8 divider
    //TB0EX0 |= TBIDEX_7;                        // /8 divider

}
void timerA_RNG_init() {
    TA0CCTL0 = CAP | CM_1; // | CCSIS_1; // timer SMCLK do rng
    TA0CTL = TASSEL_2; // MC_2 - z tym trybem ma to działać ale ustawiam to w przerwaniach

    TA1CCR0 = 120; // timer ACLK do rng
    TA1CCTL0 = CCIE | OUTMOD_3;
    TA1CTL = TASSEL_1; // MC_1 - z tym trybie ma to działać, ale ustawiam to w przerwaniach

    TA2CCR0 = 33 * pulse_width; // timer ACLK budzący wcześniejsze dwa, by na nowo wylosować liczbę, działa co pulse_width
    TA2CCTL0 = CCIE | OUTMOD_3;
    TA2CTL = TASSEL_1 | MC_1;
}
/*

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
}*/

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
    }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

    CSCTL0_H = 0;                             // Lock CS registerss
}

void send_uart_msg(int msg)
{
    //int i = PULSE_BITS - 1;
    UCA0TXBUF = (msg & 0x7) + '0';// (msg & 0xF) + '0';
    /*
    for (i = PULSE_BITS; i != 0 ; i--) {
        UCA0TXBUF = (msg & (1 << i)) + '0';
    }
    UCA0TXBUF = ' ';*/
}
// Konfiguracja przetwornika ADC
void ADC12_init(void)
{
    ADC12CTL0 = ADC12SHT0_10 | ADC12MSC | ADC12ON;  // Sampling time, multiple SC, ADC12 on
    ADC12CTL1 = ADC12SHS_3+ADC12CONSEQ_2;     // Use sampling timer; ADC12MEM0
                                              // Sample-and-hold source = CCI0B =
                                              // TBCCR1 output
                                              // Repeated-single-channel
    ADC12MCTL0 = ADC12VRSEL_0 | ADC12INCH_11;     // V+=AVcc V-=AVss, A11 channel - do niego mam podpięty potencjometr
    ADC12CTL0 |= ADC12ENC;                  // Enable conversion
}

void DMA0_init(void)
{
    DMACTL0 = DMA0TSEL_26; //DMA Trigger Assignments:26==ADC12 end of conversion
    DMACTL4 = DMARMWDIS;
    DMA0CTL &= ~DMAIFG;
    __data20_write_long((unsigned long) &DMA0SA, (unsigned long) &ADC12MEM0);
    __data20_write_long((unsigned long) &DMA0DA, (unsigned long) &pot_voltage); // Destination single address
    DMA0CTL = DMADT_4 | DMASRCINCR_0 | DMADSTINCR_0 | DMAIE;  // repeated single transfer | constant source address | constant dest add | interrupt enable // DMADSTINCR_3
    DMA0SZ = 1; // Block size - one word
    DMA0CTL |= DMAEN;
}

void pin_init(void)
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





int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;      // Stop WDT

  scale_sine_tab(scale);

  timerB0_init();
  DMA0_init();
  ADC12_init();
  pin_init();
  initClockTo16MHz();
  initUART();
  timerA_RNG_init();

  ADC12CTL0 |= ADC12ENC; // Zezwolenie na konwersję ADC

#if UART_MODE == ACLK_9600
    __bis_SR_register(LPM3_bits + GIE);       // Since ACLK is source, enter LPM3, interrupts enabled
#else
    __bis_SR_register(LPM0_bits + GIE);       // Since SMCLK is source, enter LPM0, interrupts enabled
#endif
  __no_operation();                         // used for debugging
}

/****************************************************************
 * Przerwanie DMA - zakończenie zapisu próbki czujnika
 ***************************************************************/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=DMA_VECTOR
__interrupt void DMA_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(DMA_VECTOR))) DMA_ISR (void)
#else
#error Compiler not supported!
#endif
{
  unsigned short noised_signal;
  int lols = 0;
  switch(__even_in_range(DMAIV, 16))
  {
    case 0: break;
    case 2:                                 // DMA0IFG = DMA Channel 0
      noised_signal = pot_voltage + scaled_sine_tab[sine_counter];
/*
      if (noised_signal > 4000) //
          P1OUT |= BIT0; // P1.0 = 1 czyli zapalenie diody;
      else
          P1OUT &= ~BIT0; // zgaszenie diody


      sine_counter++;
      if (sine_counter >= NUMBER_OF_SINE_POINTS) {
          sine_counter = 0;
          lols++;
      }

      if (lols == 10) {
          send_byte();
      } */

      break;
    case 4: break;                          // DMA1IFG = DMA Channel 1
    case 6: break;                          // DMA2IFG = DMA Channel 2
    case 8: break;                          // DMA3IFG = DMA Channel 3
    case 10: break;                         // DMA4IFG = DMA Channel 4
    case 12: break;                         // DMA5IFG = DMA Channel 5
    case 14: break;                         // DMA6IFG = DMA Channel 6
    case 16: break;                         // DMA7IFG = DMA Channel 7
    default: break;
  }
}

/****************************************************************
 * Przerwanie UART - odebranie sygnału
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

/****************************************************************
 * Przerwanie timera losującego pulse_amplitude
 ***************************************************************/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_A0_VECTOR))) TIMER1_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    A0_ticks = TA0R;      // save number of ticks
    TA0R = 0;  // reset cnt. of SMCLK ticks

    if (A0_ticks & (1 << 0))   // comp. if LSB is 1
    {                   // if yes save LSB = 1 to left-shifted // register rn
        pulse_amplitude |= (1 << current_pulse_bit);
        current_pulse_bit--;
    }
    else                // if not save LSB = 0 to left-shifted// register rn
    {
        //pulse_amplitude &= (0 << current_pulse_bit);
        current_pulse_bit--;
    }


    if (current_pulse_bit == -1)
    {
        send_uart_msg(pulse_amplitude);
        current_pulse_bit = PULSE_BITS - 1;
        TA0CTL &= ~MC_2;        // Zatrzymanie timerów losowania
        TA1CTL &= ~MC_1;

        pulse_amplitude = 0;

        //P1OUT = (A0_ticks & (1 << 0));
        P1OUT ^= BIT0;
    }
}

/****************************************************************
 * Przerwanie timera odliczającego pulse_witdh - włącza losowanie impulsu
 ***************************************************************/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER2_A0_VECTOR))) TIMER2_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    TA0CTL |= MC_2;
    TA1CTL |= MC_1;
}
