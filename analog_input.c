/* Program działa mniej więcej tak:
 *  - timer co zadany czas wyzwala przerwanie
 *  - obsługa przerwania timera polega na umożliwieniu odczytu ADC, który następuje od razu
 *  - zakończenie odczytu ADC wyzwala przerwanie, którego obsługa zapala - lub nie - diodę
 * Może się mylę :) Trochę rzeczy muszę przemyśleć jeszcze na pewno co i jak powinno tu chodzić, bo nie wszystko rozumiem
 */

#include <stdio.h>
#include <msp430.h>


volatile unsigned int in_temp; // temperature measurement result storage variable

// Konfiguracja timera, żeby co określony czas wyzwalał przerwanie, które obudzi ADC
void config_timerA2(void)
{
    TA2CTL = TASSEL_1 + MC_1 + ID_0; // ACLK, divider of 1, up mode
    TA2CCR0 = 32764 / 100;             // 32 764 ACLK periods = 1 second
    TA2CCTL0 = CCIE;                 // Enable interrupt on Timer A2
}

// Konfiguracja przetwornika ADC
void config_ADC12(void) // Jeszcze muszę tu ogarnąć trochę dlaczego tak a nie inaczej
{
    /* ***** Core configuration ***** */
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;  // Sampling time, S&H=16, ADC12 on
    ADC12CTL1 = ADC12SHP;               // Use sampling timer
    ADC12CTL2 |= ADC12RES_2;            // 12-bit conversion results
    ADC12IER0 |= ADC12IE0;              // Enable ADC conv complete interrupt

    /* ***** Channel configuration ***** */

    // Use MCTL0 for conversion, configure reference = V_REF (1.5V)
    // Input channel = 10 => Interlan temperature sensor
    ADC12MCTL0 = ADC12INCH_10; // | ADC12SREF_1;  // ADC i/p ch A10 = temp sense
                                              // ACD12SREF_1 = internal ref = 1.5v

    ADC12IER0 |= ADC12IE0;

    __delay_cycles(100);                    // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;                  // Enable conversion
}

// Tu zacząłem próbować coś z DMA, ale jeszcze mi nic nie wyszło
/*
void config_DMA0(){
    DMACTL0 = DMA0TSEL__ADC12IFG; //DMA Trigger Assignments:26==ADC12 end of conversion
    DMACTL4 = DMARMWDIS;
    DMA0CTL = DMADT_4 | DMASRCINCR_0 | DMADSTINCR_0 | DMAIE;  // repeated single transfer | constant source address | constant dest add | interrupt enable
    DMA0SZ = 1; // Block size - one word
    __data16_write_addr((unsigned short) &DMA0SA, (unsigned long) &ADC12MEM0);
    __data16_write_addr((unsigned short) &DMA0DA, (unsigned long) &in_temp); // Destination single address
    DMA0CTL |= DMAEN;
}
*/

// Timer A2 ISR - obsługa przerwania timera
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER2_A0_VECTOR))) Timer_A2_ISR (void)
#else
#error Compiler not supported!
#endif
{
    //timer++; // You probably still want to keep track of time - jeszcze nie zastanawiałem się po co to xD Skopiowałem i zostawiam zakomentowane

    ADC12CTL0 |= ADC12SC; // Ustawienie bitu pozwalającego na pracę konwertera ADC. Po tej zmianie powinien automatycznie zacząć pracę.

    // w oryginalnym kodzie był tu komentarz o tym, że w zależności od konfiguracji przerwań można tu sprawdzać bit zajętości (busy bit)
}

// ADC, po zakończonej konwersji, wyzwala przerwnie - to kod jego obsługi.
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
    // Move the results into global variable
    in_temp = ADC12MEM0 & 0x0FFF;

    if (in_temp > 2070 /*2060*/) // Ta wartość u mnie działa tak, że jak przykładam palec do procesora do dioda zaczyna mrugać xd
        P1OUT |= BIT0; // P1.0 = 1 czyli zapalenie diody
    else
        P1OUT &= ~BIT0; // zgaszenie diody
}


int main(void)
{

  WDTCTL = WDTPW + WDTHOLD;      // Stop WDT

  config_timerA2();
  //config_DMA0();
  config_ADC12();

  // ustawienie diody:
  P1OUT &= ~BIT0; // Clear LED to start
  P1DIR |= BIT0; // Set P1.0/LED to output


  // Disable the GPIO power-on default high-impedance mode to activate
  // previously configured port settings
  PM5CTL0 &= ~LOCKLPM5;

  __enable_interrupt(); // Globally enable interrupts -- Nie wiem, czy to konieczne

  ADC12CTL0 |= ADC12ENC; // Zezwolenie na konwersję ADC
}
