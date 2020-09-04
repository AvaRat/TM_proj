
void init_timerA_for_RNG() {
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
    P1OUT ^= BIT0;
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
