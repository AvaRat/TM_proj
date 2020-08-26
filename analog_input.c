/* Program działa mniej więcej tak:
 *  - timer wyzwala przerwanie, które automatycznie budzi ADC?
 *  - ADC po konwersji wyzwala przerwanie, które automatycznie budzi DMA?
 *  - DMA po zakończeniu pracy wysyła przerwanie, którego obsługa polega na zapaleniu - lub nie - diody
 *
 *  Ogólnie robiłem na podstawie przykładowego kodu: https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/166/7215.MSP430F55xx_5F00_dma_5F00_04.c
 */

#include <stdio.h>
#include <msp430.h>

volatile unsigned int in_temp; // temperature measurement result storage variable

// Konfiguracja timera, żeby co określony czas wyzwalał przerwanie, które obudzi ADC
void config_timerB0(void)
{
    TBCCR0 = 0xFFFE;                          // Jeszcze nie wiem co to robi dokładnie, ale się dowiem
    TBCCR1 = 0x8000;
    TBCCTL1 = OUTMOD_3;                       // CCR1 set/reset mode
    TBCTL = TBSSEL_2+MC_1+TBCLR;              // SMCLK, Up-Mode
}

// Konfiguracja przetwornika ADC
void config_ADC12(void)
{
    ADC12CTL0 = ADC12SHT0_10 | ADC12MSC | ADC12ON;  // Sampling time, multiple SC, ADC12 on
    ADC12CTL1 = ADC12SHS_3+ADC12CONSEQ_2;           // Use sampling timer; ADC12MEM0
                                                    // Sample-and-hold source = CCI0B =
                                                    // TBCCR1 output
                                                    // Repeated-single-channel
    ADC12MCTL0 = ADC12VRSEL_0 | ADC12INCH_11;       // V+=AVcc V-=AVss, A11 channel - do niego mam podpięty potencjometr
    ADC12CTL0 |= ADC12ENC;                          // Enable conversion
}

void config_DMA0(){
    DMACTL0 = DMA0TSEL_26;       // DMA Trigger Assignments:26==ADC12 end of conversion
    DMACTL4 = DMARMWDIS;         // Zabronienie na pracę DMA, gdy procesor jest w trakcie read/modify/write
    DMA0CTL &= ~DMAIFG;
    __data20_write_long((unsigned long) &DMA0SA, (unsigned long) &ADC12MEM0); // Source address - przypisanie adresów, skąd DMA ma brać dane i gdzie zapisywać
    __data20_write_long((unsigned long) &DMA0DA, (unsigned long) &in_temp);   // Destination address
    DMA0CTL = DMADT_4 | DMASRCINCR_0 | DMADSTINCR_0 | DMAIE;  // repeated single transfer | constant source address | inc dest add | interrupt enable
    DMA0SZ = 1;                 // Block size - one word
    DMA0CTL |= DMAEN;           // DMA enable
}

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
  switch(__even_in_range(DMAIV,16))
  {
    case 0: break;
    case 2:                                 // DMA0IFG = DMA Channel 0
      //P1OUT ^= BIT0;                      // Toggle P1.0 - PLACE BREAKPOINT HERE AND CHECK DMA_DST VARIABLE
      if (in_temp > 1500)                   // Ta wartość u mnie działa tak, że jak przykładam palec do procesora do dioda zaczyna mrugać xd
          P1OUT |= BIT0;                    // P1.0 = 1 czyli zapalenie diody
      else
          P1OUT &= ~BIT0; // zgaszenie diody
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


  config_timerB0();
  config_DMA0();
  config_ADC12();

  // ustawienie diody:

  P1OUT &= ~BIT0;                           // P1.0 clear
  P1DIR |= BIT0;                            // P1.0 output

  P1SEL1 &= ~BIT4;                            // P1.4/TB1 option select
  P1SEL0 |= BIT4;                             // As above
  P1DIR |= BIT4;                            // Output direction of P1.4

  ADC12CTL0 |= ADC12ENC; // Zezwolenie na konwersję ADC

  __bis_SR_register(LPM0_bits + GIE);       // LPM0 w/ interrupts
  __no_operation();                         // used for debugging
}
