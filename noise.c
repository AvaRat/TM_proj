#include "system.h"
#include "line_table.h"


unsigned int next_pulse_amplitude;
unsigned int tmp_next_pulse_amplitude;
unsigned int wave_counter=0;

volatile int noise_step; // uzyskuje wartość w set_params()
volatile int noises;    // uzyskuje wartość w set_params()
unsigned char sign;     // uzyskuje wartość w set_params()

unsigned long A0_ticks=0;
int current_pulse_bit = PULSE_BITS - 1;
int noise_counter=0;

void set_params(int noise_perios_ms) {
    //noise_period_ms = 100;
    noise_step = 1000 * SAMPLING_PERIOD / noise_period_ms; //nie : noise_period_ms / samplig_period;
    noises |= PULSE_NOISES | HARMONICAL_NOISES; //| PULSE_NOISES;
    //mode = HARMONICAL; // ekg albo sinus
    //noise_max = 7;      // max wielkość szumów jako wykladnik potegi 2 (np jesli 7 to 2^7 = 128)
}

void init_timerA_for_RNG(int noise_period_ms) {
    TA0CCTL0 = CAP | CM_1;  // timer SMCLK do rng
    TA0CTL = TASSEL_2;      // MC_2 - z tym trybem ma dzia³aæ ale ustawiam to w przerwaniach - gdy chcê go w³¹czyæ, tzn zacz¹æ losowaæ liczbê

    TA1CCR0 = 120;          // timer ACLK do rng
    TA1CCTL0 = CCIE | OUTMOD_3;
    TA1CTL = TASSEL_1;      // MC_1 - z tym trybem ma dzia³aæ, ale ustawiam to w przerwaniach - gdy chcê go w³¹czyæ, tzn zacz¹æ losowaæ liczbê

    TA2CCR0 = 33 * noise_period_ms; // 33 - wynika z f_aclk = ~33kHz. Timer ACLK budz¹cy wczeœniejsze dwa, by na nowo wylosowaæ liczbê. Dzia³a co noise_period_ms
    TA2CCTL0 = CCIE | OUTMOD_3;
    TA2CTL = TASSEL_1 | MC_1;
}

/****************************************************************
 * Przerwanie timera odliczaj¹cego pulse_witdh - w³¹cza losowanie impulsu
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
    //P1OUT ^= BIT0;
}



/****************************************************************
 * Przerwanie timera losuj¹cego pulse_amplitude
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
        tmp_next_pulse_amplitude |= (1 << current_pulse_bit);
        current_pulse_bit--;
    }
    else                // if not save LSB = 0 to left-shifted// register rn
    {
        //pulse_amplitude &= (0 << current_pulse_bit);
        current_pulse_bit--;
    }


    if (current_pulse_bit == -1) // wylosowa³em ca³¹ PULSE_BITSow¹ liczbê
    {
        //send_uart_msg(pulse_amplitude);
        current_pulse_bit = PULSE_BITS - 1;

        TA0CTL &= ~MC_2;        // Zatrzymanie timerów losowania
        TA1CTL &= ~MC_1;


        next_pulse_amplitude = tmp_next_pulse_amplitude;
        tmp_next_pulse_amplitude = 0;

        //P1OUT = (A0_ticks & (1 << 0));
        //P1OUT ^= BIT0;
    }
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
  int harmonized_signal;
  //unsigned int current_pulse_height = 0;
  int current_noise = 0;
  switch(__even_in_range(DMAIV, 16))
  {
    case 0: break;
    case 2:                                                 // DMA0IFG = DMA Channel 0
      if (noises & HARMONICAL_NOISES) {
          current_noise += sine_tab[noise_counter - noise_step] >> (12 - params.harmonic_gain);
      }

      if (noises & PULSE_NOISES && noise_counter == noise_step) {
          sign = (next_pulse_amplitude & (1 << 0));                 // jako znak traktuję najmłodszy bit
      } else if (noises & PULSE_NOISES) {
          if (sign == 1) {
              current_noise += next_pulse_amplitude << (params.impulsive_gain - PULSE_BITS);
          }
          else {
              current_noise -= next_pulse_amplitude << (params.impulsive_gain - PULSE_BITS);
          }
      }
      noise_counter += noise_step;

      if (noise_counter > 1000)
          noise_counter = noise_step;

      if (params.signal_type == SIN)
          harmonized_signal = (int) (((long) ((long) input_signal * (long) sine_tab[wave_counter - SAMPLING_PERIOD])) >> ADC_BITS) + current_noise;
      else
          harmonized_signal = (int) (((long) ((long) input_signal * (long) ecg_tab[wave_counter - SAMPLING_PERIOD])) >> ADC_BITS) + current_noise;

      wave_counter += SAMPLING_PERIOD;

      if (wave_counter > 1000) {
          wave_counter = SAMPLING_PERIOD;
      }
      char str[6]; //'12\r\n'; "130\r\n"
      //sprintf(str, harmonized_signal);
      sprintf(str, "%d\r\n", harmonized_signal); // harmonized_signal
      transmitString(str);

      // TU WYSLIJ SYGNAL DALEJ


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

