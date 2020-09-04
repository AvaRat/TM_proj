#include <msp430.h>
#include <driverlib.h>
#include "hal_LCD.h"

#include <line_table.h>
#include <string.h>
#include <input_handler.h>
#include <uart_handler.h>
#include <noise.h>
#include <stdio.h>

#include "app_logic.h"
#include "uart.h"
//#include "rtc.h"

#define SAMPLING_PERIOD 10         // ms

// dostępne tryby:
#define HARMONICAL      0
#define ECG             1

#define HARMONICAL_NOISES    (1 << 0)
#define PULSE_NOISES         (1 << 1)

volatile int mode = 0;

volatile int noise_max = 7;      // max wielkość szumów jako wykladnik potegi 2 (np jesli 7 to 2^7 = 128)
volatile int noise_period_ms = 100;


volatile int noises = 0;
unsigned char sign = 0;
volatile int noise_step = 0; // uzyskuje wartość w set_params()



volatile unsigned char app_state = STARTUP;

volatile unsigned int counter = 0;
volatile int centisecond = 0;

int debug_wave_counter = 0;
int debug_ktora_fala = 0;
int debug_ktora_liczba = 0;
int debug_harmonized_signal = 0;
int debug_current_noise = 0;
int debug_current_pulse_noise = 0;
int debug_current_harmonical_noise = 0;
int debug_sign = 0;



#define ADC_BITS        12

#define NOISE_MAX       16

#define PULSE_BITS      5
#define NOISE_AMPLIFY   10

// niech na razie wszystko będzie w ms

volatile unsigned int input_signal;               // temperature measurement result storage variable
//volatile unsigned int sine_counter = 0;
//volatile float scale = 1;                        // skala funkcji sinus. Przy równej 1: max = 4095, min = 0
//volatile int xd = 0;

unsigned int wave_counter = 0;

int current_pulse_bit = PULSE_BITS - 1;
int noise_counter = 0;
//unsigned int noise_amplifier = NOISE_AMPLIFY;                          // skala impulsów - jeśli równa 1, to impulsy są w zakresie 0-15
//int noise_amplitude = NOISE_MAX * NOISE_AMPLIFY;


unsigned int next_pulse_amplitude = 0;
unsigned int tmp_next_pulse_amplitude = 0;


unsigned int rn = 0;
unsigned long A0_ticks = 0;


Calendar currentTime;

void Init_GPIO(void);

int main(void)
{
    // czesc aplikacji
    // Stop watchdog timer
    WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);

    PMM_unlockLPM5();
    Init_LCD();



    // czesc michala (systemowa)
    set_params();

    //scale_sine_tab(noise_max);

    init_timerB0_for_ADC();
    init_DMA(&input_signal);
    init_ADC12();
    init_pin();
    initClockTo16MHz();
    initUART();
    init_timerA_for_RNG();

    ADC12CTL0 |= ADC12ENC; // Zezwolenie na konwersję ADC


/*
    // Configure RTC_C
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
    RTCCTL0_L = RTCTEVIE | RTCRDYIE;        // enable RTC read ready interrupt
                                            // enable RTC time event interrupt

    RTCCTL1 = RTCBCD | RTCHOLD | RTCMODE;   // RTC enable, BCD mode, RTC hold

    RTCYEAR = 0x2010;                       // Year = 0x2010
    RTCMON = 0x4;                           // Month = 0x04 = April
    RTCDAY = 0x05;                          // Day = 0x05 = 5th
    RTCDOW = 0x01;                          // Day of week = 0x01 = Monday
    RTCHOUR = 0x10;                         // Hour = 0x10
    RTCMIN = 0x32;                          // Minute = 0x32
    RTCSEC = 0x01;                          // Seconds = 0x45

    RTCCTL1 &= ~(RTCHOLD);                  // Start RTC
    */


/*
    Calendar currentTime;
    //Setup Current Time for Calendar
    currentTime.Seconds    = 0x00;
    currentTime.Minutes    = 0x26;
    currentTime.Hours      = 0x13;
    currentTime.DayOfWeek  = 0x03;
    currentTime.DayOfMonth = 0x20;
    currentTime.Month      = 0x07;
    currentTime.Year       = 0x2011;
    //check if restart occured
    RTC_C_initCalendar(RTC_C_BASE,
        &currentTime,
        RTC_C_FORMAT_BCD);
    RTC_C_startClock(RTC_C_BASE);
*/
    //newTime.Seconds = RTCSEC;

    //no restart detected
    //show greetings + quick manual

    displayScrollText("ELO MORDO"); //"WELCOME IN TMPROJ DEMO.     LEFT BUTTON : SETTINGS     RIGHT BUTTON : SAVE PARAMS    HAVE FUN"

    Init_GPIO();
    init_clock();
    __enable_interrupt();

    while(1)
    {
        switch(app_state)
        {
        case STARTUP:
            display_params(10,10, EKG);
            //
            break;
        case NORMAL:
            display_params(params.periodic_gain, params.impulsive_gain, params.periodic_interf_type);
            break;
        case SETTINGS:
            init_settings();
            break;
        }
    /* wait for button press:
     * s1 -> open settings
     * s2 -> save params
     */
    }
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
    GPIO_setAsOutputPin(GPIO_PORT_P9, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

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

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
}

void set_params() {
    noise_period_ms = 100;
    noise_step = 1000 * SAMPLING_PERIOD / noise_period_ms; //nie : noise_period_ms / samplig_period;
    noises |= PULSE_NOISES | HARMONICAL_NOISES; //| PULSE_NOISES;
    mode = HARMONICAL;
    noise_max = 7;      // max wielkość szumów jako wykladnik potegi 2 (np jesli 7 to 2^7 = 128)
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
          current_noise += sine_tab[noise_counter - noise_step] >> (12 - noise_max);
          debug_current_harmonical_noise = current_noise;
      }

      if (noises & PULSE_NOISES && noise_counter == noise_step) {
          sign = (next_pulse_amplitude & (1 << 0));                 // jako znak traktuję najmłodszy bit
          debug_sign = sign;
      } else if (noises & PULSE_NOISES) {
          if (sign == 1) {
              current_noise += next_pulse_amplitude << (noise_max - PULSE_BITS);
              debug_current_pulse_noise += next_pulse_amplitude << (noise_max - PULSE_BITS);
          }
          else {
              current_noise -= next_pulse_amplitude << (noise_max - PULSE_BITS);
              debug_current_pulse_noise -= next_pulse_amplitude << (noise_max - PULSE_BITS);
          }
      }
      noise_counter += noise_step;

      if (noise_counter > 1000)
          noise_counter = noise_step;

      if (mode == HARMONICAL)
          harmonized_signal = (int) (((long) ((long) input_signal * (long) sine_tab[wave_counter - SAMPLING_PERIOD])) >> ADC_BITS) + current_noise;
      else
          harmonized_signal = (int) (((long) ((long) input_signal * (long) ecg_tab[wave_counter - SAMPLING_PERIOD])) >> ADC_BITS) + current_noise;

      debug_harmonized_signal = harmonized_signal;

      wave_counter += SAMPLING_PERIOD;
      debug_wave_counter = wave_counter;


      if (wave_counter > 1000) {
          wave_counter = SAMPLING_PERIOD;
          debug_ktora_fala++;
      }
      char str[6]; //'12\r\n'; "130\r\n"
      //sprintf(str, harmonized_signal);
      sprintf(str, "%d\r\n", harmonized_signal); // harmonized_signal
      send_uart_msg(str);

      debug_current_harmonical_noise = 0;
      debug_current_pulse_noise = 0;
/*
      if (harmonized_signal > 1000) //
          P1OUT |= BIT0; // P1.0 = 1 czyli zapalenie diody;
      else
          P1OUT &= ~BIT0; // zgaszenie diody
*/
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
}
*/

