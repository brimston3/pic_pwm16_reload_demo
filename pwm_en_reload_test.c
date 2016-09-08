/*****************************
 File:   pwm_en_reload_test.c
 Author: aed.20160906
 Project: N/A
 Copyright (C) 2016 Andrew Domaszek, All Rights Reserved.

 Demonstrate PWM reload behavior when toggling PWM EN.
 
 The PIC16F1579 uses the PIC 16-bit PWM silicon. These PWMs are not like those
 found in CCP/ECCP modules as they have independent timers, 16-bit compare
 registers, phase/offset adjustments, and can run while the device is sleeping.
 
 They are the same style as the 16-bit PWMs on the PIC16F1779 (PWM5/6/11/12)
*****************************/

/*****************************
Dependencies:   xc.h
Processor:      PIC16F1579
Compiler:       XC8 v1.38, free mode
(Other compilers will probably work, but 1.38 is what aed used.)
*****************************/


#pragma config WDTE=OFF, LVP=OFF, MCLRE=ON, CP=OFF, FOSC=INTOSC
// , PLLEN=ON

#include <xc.h>
#include <stdint.h>

/*****************************
 Program Configuration.
 
 NO_PWM_MANGLING              - disable switching PWM modes.
 PWM_CLEAR_TMR_ON_MODE_CHANGE - zero timer count registers when switching.
                                not recommended except when bypassing buffered
                                registers (DISABLE_PWM_FOR_UPDATE).
 EXTERNAL_PULLUP_RA2          - external pull-up applied to RA2, don't pull up
                                internally.
 RC0_ALL_INTERRUPTS           - toggle RC0 on every interrupt, not just unhandled
                                interrupts.
 BOTH_ON_DUTY_CYCLE           - change mode either way on duty cycle interrupt
                                for testing symmetry.
                                default is 0->1 on DC, 1->0 on PR
 DISABLE_PWM_FOR_UPDATE       - turn PWM off for updates, bypass double-buffered
                                registers.
                                Usually also want PWM_CLEAR_TMR_ON_MODE_CHANGE.
*****************************/
//#define NO_PWM_MANGLING
//#define PWM_CLEAR_TMR_ON_MODE_CHANGE
#define EXTERNAL_PULLUP_RA2
#define RC0_ALL_INTERRUPTS
//#define BOTH_ON_DUTY_CYCLE
//#define DISABLE_PWM_FOR_UPDATE

/*****************************
 Port Initializations.
*****************************/

// Port A: PWM output.
void PortAInitialize()
{
    ANSELA = 0b11001011; // digital mode on RA2,4-5
    TRISA = 0b11001111;  // output mode on RA4-5
    //RA4PPS = 0x00; // LATA4 output, default
    RA5PPS = 0x03;   // PWM1_out
    
#if !defined(EXTERNAL_PULLUP_RA2)
    // Apply weak pull up to RA2 to make it easier to switch (by grounding)
    WPUAbits.WPUA2 = 1;
    OPTION_REGbits.nWPUEN = 0; // Global Pull-Up enable
#endif
}

void PortCInitialize()
{
    ANSELC = 0b11111110; // digital RC0
    TRISC =  0b11111110; // output RC0
}

uint8_t g_mode = 0;
uint8_t c0flip = 0; // For toggling rc0 a bit.

const struct {
    uint8_t prh;
    uint8_t dch;
} pwm1_modes[] = {
    {100,50},
    {24,12}
};

void Pwm1Initialize()
{
    //PWM1CONbits.MODE = 0; // standard (period) mode, the default
    //PWM1LDCONbits.LDT = 0; // immediate reload, default
    //PWM1OFCONbits.OFM = 0; // independent run mode, default
    
    PWM1PRH = pwm1_modes[g_mode].prh;
    PWM1PRL = 0;
    
    PWM1DCH = pwm1_modes[g_mode].dch;
    PWM1DCH = 0;
    
    PWM1PHH = 0;
    PWM1PHL = 0;
    
    PWM1TMRH = 0;
    PWM1TMRL = 0;
    
    PWM1LDCONbits.LDA = 1; // start a load.
    
    PWM1CLKCONbits.PS = 0;    // CLK/1, no prescale.
    PWM1CLKCONbits.CS = 0b01; // HFINTOSC
    
    PWM1INTEbits.PRIE = 1; // enable period interrupt.
    PWM1INTEbits.DCIE = 1; // enable duty cycle interrupt.

    PIE3bits.PWM1IE = 1; // PWM1 interrupts on.
}

void update_pwm1(uint8_t ra2bit)
{
#if !defined(NO_PWM_MANGLING)
    g_mode = ra2bit;
    
#if defined(DISABLE_PWM_FOR_UPDATE)
    PWM1CONbits.EN = 0;
#endif
            
    PWM1PRH = pwm1_modes[g_mode].prh;
    PWM1PRL = 0;
    PWM1DCH = pwm1_modes[g_mode].dch;
    PWM1DCL = 0;
            
#if defined(PWM_CLEAR_TMR_ON_MODE_CHANGE)
    PWM1TMRH = 0;
    PWM1TMRL = 0;
#endif
            
    PWM1LDCONbits.LDA = 1;
            
    LATA4 = g_mode;
#if defined(DISABLE_PWM_FOR_UPDATE)
    PWM1CONbits.EN = 1;
#endif
#endif
}

void interrupt tc_int (void)        // interrupt function 
{
    uint8_t flag = 1;
    uint8_t ra2bit = PORTAbits.RA2;
    if (PWM1INTFbits.PRIF && PWM1INTEbits.PRIE)
    {
        PWM1INTFbits.PRIF = 0;
        flag = 0;
#if !defined(BOTH_ON_DUTY_CYCLE)
        if (ra2bit == 0 && g_mode == 1)
        {
            update_pwm1(ra2bit);
        }
#endif
    }
    if (PWM1INTFbits.DCIF && PWM1INTEbits.DCIE)
    {
        PWM1INTFbits.DCIF = 0;
        flag = 0;
#if defined(BOTH_ON_DUTY_CYCLE)
        if (ra2bit != g_mode)
#else
        if (ra2bit == 1 && g_mode == 0)
#endif
        {
            update_pwm1(ra2bit);
        }
    }
    
#if !defined(NO_PWM_MANGLING) && !defined(RC0_ALL_INTERRUPTS)
    if (flag)
#endif
    {
        c0flip = !c0flip;
        LATC0 = c0flip;
    }
}

void main(void) {
    OSCCONbits.IRCF = 13; // 4MHz clock
    OSCCONbits.SCS = 2; // int osc

    PortAInitialize();
    PortCInitialize();
    Pwm1Initialize();

    INTCONbits.PEIE = 1; // Peripheral interrupts on
    INTCONbits.GIE = 1;  // Global interrupt enable
    
    PWM1CONbits.EN = 1; // Get this party started.
    
    
    for(;;)
    {
        CLRWDT(); // just reset the watchdog in a loop.
    }
    //return;
}
