#include <msp430.h>

// Konfiguracja timera, ¿eby co okreœlony czas budzi³ ADC - niech bêdzie T = 0.001 s
void init_timerB0_for_ADC()
{
    // smclk: 16 000 000 = 1 sek:
    TB0CCR0 = (16000 / 8) * SAMPLING_PERIOD;
    TB0CCR1 = (8000 / 8) * SAMPLING_PERIOD;          // to z tym na górze to znaczy chyba, ¿e jak doliczy do 0x8000 to ustawia jakiœ bit, a jak do 0xFFFE to resetuje

    TB0CCTL1 = OUTMOD_3;                       // CCR1 set/reset mode
    TB0CTL = TBSSEL_2 + MC_1 + TBCLR;          // SMCLK, Up-Mode, clear

    // spowolnienie timera, by mieœciæ siê w jego rejestrze:
    TB0CTL |= ID_3;                            // dzielenie przez 8
}

// Konfiguracja przetwornika ADC
void init_ADC12(void)
{
    ADC12CTL0 = ADC12SHT0_10 | ADC12MSC | ADC12ON;  // Czas próbkowania, multiple SC, ADC12 on
    ADC12CTL1 = ADC12SHS_3 | ADC12CONSEQ_2;         // Use sampling timer; ADC12MEM0
                                                    // Sample-and-hold source = CCI0B =
                                                    // TBCCR1 output
                                                    // Repeated-single-channel
    ADC12MCTL0 = ADC12VRSEL_0 | ADC12INCH_11;       // V+=AVcc V-=AVss, A11 channel - do niego mam podpiêty potencjometr
    ADC12CTL0 |= ADC12ENC;                          // Enable conversion
}


void init_DMA(volatile unsigned int * input_signal_ptr)
{
    DMACTL0 = DMA0TSEL_26; //DMA Trigger Assignments:26==ADC12 end of conversion
    DMACTL4 = DMARMWDIS;
    DMA0CTL &= ~DMAIFG;
    __data20_write_long((unsigned long) &DMA0SA, (unsigned long) &ADC12MEM0);
    __data20_write_long((unsigned long) &DMA0DA, (unsigned long) input_signal_ptr); // Destination single address
    DMA0CTL = DMADT_4 | DMASRCINCR_0 | DMADSTINCR_0 | DMAIE;  // repeated single transfer | constant source address | constant dest add | interrupt enable
    DMA0SZ = 1; // Block size - one word
    DMA0CTL |= DMAEN;
}

