#include <other_funs.h>
#include <fram_storage.h>

//#define PARAMS_TABLE_SIZE       100
/*
struct params_struct {
    // last use flag:
    unsigned char if_used_before_reset;     // 0 - nie, 1 - tak

    // signal params:
    unsigned int harmonic_gain;
    unsigned int impulsive_gain;
    unsigned int signal_type;

    // date params:
    unsigned int day_of_week;       // 1-7
    unsigned int hour;              // 0-23
    unsigned int minute;            // 0 59
};

#if defined(__TI_COMPILER_VERSION__)
#pragma PERSISTENT(params_table)
volatile struct params_struct params
#elif defined(__IAR_SYSTEMS_ICC__)
__persistent struct params_struct params
#elif defined(__GNUC__)
struct params_struct __attribute__((persistent)) params
#else
#error Compiler not supported!
#endif
= {.0, .0, .0, .0, .0, .0, .0};

*/
/*
 * INDEKS PIERWSZEGO ZAJ�TEGO ELEMENTU:
 */ /*
#if defined(__TI_COMPILER_VERSION__)
#pragma PERSISTENT(first_used_params_index)
volatile unsigned int first_used_params_index
#elif defined(__IAR_SYSTEMS_ICC__)
__persistent unsigned int first_used_params_index
#elif defined(__GNUC__)
unsigned int __attribute__((persistent)) first_used_params_index
#else
#error Compiler not supported!
#endif
= 0;           // 0 - PARAMS_TABLE_SIZE - 1




#if defined(__TI_COMPILER_VERSION__)
#pragma PERSISTENT(first_free_params_index)
volatile unsigned int first_free_params_index
#elif defined(__IAR_SYSTEMS_ICC__)
__persistent unsigned int first_free_params_index
#elif defined(__GNUC__)
unsigned int __attribute__((persistent)) first_free_params_index
#else
#error Compiler not supported!
#endif
= 0;           // 0 - PARAMS_TABLE_SIZE - 1
*/

/*
 * Tablica struktur parametrow - bufor cykliczny
 */


/*
void save_params(struct params_struct * params_ptr) {
    params_table[first_free_params_index] = * params_ptr;

    if (first_free_params_index == first_used_params_index) {       // Jesli dogonilem najwczesniej zapisany
        first_used_params_index++;                                  // to znaczy, ze najwczesniej zapisany jest nadpisany
                                                                    // a nowy najwczesniej zaisany jest kolejny
        if (first_used_params_index == PARAMS_TABLE_SIZE)
            first_used_params_index = 0;
    }

    first_free_params_index++;                                      // nastepny wolny jest kolejny
    if (first_free_params_index == PARAMS_TABLE_SIZE)
        first_free_params_index = 0;
}

struct params_struct read_params(unsigned int back_shift) {        // back_shift - przesuniecie o back_shift w tyl, tzn jesli back_shift = 0 to ostatnio zapisane, jesli 1 to jeden wstecz itd
    int index = first_free_params_index - (back_shift + 1);
    if (index < 0)
        index = PARAMS_TABLE_SIZE + first_free_params_index - (back_shift + 1);

    return params_table[index];
} */

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
  /*__delay_cycles(50000000);
  int i = 0;
  for (i = 0; i < PARAMS_TABLE_SIZE; i++) {
      send_12bit_via_uart(params_table[i].harmonic_gain);
  } */

  struct params_struct params;
  params.if_used_before_reset = 1;
  params.harmonic_gain = 1;
  params.impulsive_gain = 1;
  params.signal_type = 1;
  params.day_of_week = 1;
  params.hour = 1;
  params.minute = 1;

  clear_FRAM_params_space();

  save_params(&params);
  save_params(&params);

  params.if_used_before_reset = 2;
  params.harmonic_gain = 2;
  params.impulsive_gain = 2;
  params.signal_type = 2;
  params.day_of_week = 2;
  params.hour = 2;
  params.minute = 2;

  save_params(&params);

  struct params_struct tmp;
  get_params(&tmp);

  clear_FRAM_params_space();


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

        //SAC0DAT = DAC_data & 0xFFF;

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
  //int i = 0;
  //struct params_struct tmp_struct;
  switch(__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
        P1OUT ^= BIT0;
       /* for (i = 0; i < PARAMS_TABLE_SIZE + 5; i++) {
            tmp_struct.harmonic_gain = i;
            tmp_struct.impulsive_gain = i;
            tmp_struct.signal_type = i;
            tmp_struct.day_of_week = i;

            save_params(&tmp_struct);
        }*/

      __no_operation();
      break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
  }
}

