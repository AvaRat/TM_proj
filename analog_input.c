/* Program działa mniej więcej tak:
 *  - timer ustawia bit, który automatycznie budzi ADC?
 *  - ADC jest skonfigurowane tak, że jest budzone przez timer
 *  - DMA po zakończeniu pracy wysyła przerwanie, którego obsługa polega na zapaleniu - lub nie - diody
 *  - jednocześnie w przerwaniu nakładany jest na wszystko sinus zgodnie z tablicą z pliku sine_table.h
 */

#include <stdio.h>
#include <msp430.h>
#include <sine_table.h>


volatile unsigned int pot_voltage;               // temperature measurement result storage variable
volatile unsigned int sine_counter = 0;

// Konfiguracja timera, żeby co określony czas budził ADC
void timerB0_init(void)
{
    // smclk: 1048575 = 1 sec. Spowalniam pod koniec funkcji /64, więc 16384 = 1 sek // w aclk: 32 767 - tyle powinno być cykli w sekundzie, a chyba nie jest .-.
    TB0CCR0 = 16384 / 100;    //0x64;
    TB0CCR1 = 16384 / 200;        //0x32;               // to z tym na górze to znaczy chyba, że jak doliczy do 0x8000 to ustawia jakiś bit, a jak do 0xFFFE to resetuje

    TB0CCTL1 = OUTMOD_3;                       // CCR1 set/reset mode
    TB0CTL = TBSSEL_2 + MC_1 + TBCLR;          // SMCLK, Up-Mode, clear

    // można spowolnić ten timer jeszcze:
    TB0CTL |= ID_3;                            // /8 divider
    TB0EX0 |= TBIDEX_7;                        // /8 divider

}

// Konfiguracja przetwornika ADC
void ADC12_init(void) // Jeszcze muszę tu ogarnąć trochę dlaczego tak a nie inaczej
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
}
/*
unsigned short compute_noised_signal(unsigned int pot_voltage)
{
    return pot_voltage + (sine_tab[5 * sine_counter] / 65);
} */
//------------------------------------------------------------------------------
// DMA Interrupt Service Routine
//------------------------------------------------------------------------------
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
  switch(__even_in_range(DMAIV, 16))
  {
    case 0: break;
    case 2:                                 // DMA0IFG = DMA Channel 0
      noised_signal = pot_voltage + sine_tab[sine_counter];
      P1OUT ^= BIT0;

      if (noised_signal > 3000) //
          P1OUT |= BIT0; // P1.0 = 1 czyli zapalenie diody
      else
          P1OUT &= ~BIT0; // zgaszenie diody

      sine_counter++;
      if (sine_counter == NUMBER_OF_SINE_POINTS) {
          sine_counter = 0;
      }

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

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;      // Stop WDT
  // Disable the GPIO power-on default high-impedance mode to activate
  // previously configured port settings
  PM5CTL0 &= ~LOCKLPM5;

  timerB0_init();
  DMA0_init();
  ADC12_init();
  pin_init();

  ADC12CTL0 |= ADC12ENC; // Zezwolenie na konwersję ADC

  __bis_SR_register(LPM0_bits + GIE);       // LPM0 w/ interrupts
  __no_operation();                         // used for debugging
}
