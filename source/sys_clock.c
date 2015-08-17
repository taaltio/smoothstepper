/*
**  sys_clock.c
**           processor clock oscillator setup etc., timers available to others
*****************************************************************************/

#define __SYS_CLOCK_C

/*************************************************************************
**  INCLUDE FILES                                                       **
*************************************************************************/
#include "types.h"
#include "smooth_globals.h"
#if !PC_EMULATED_ENVIROMENT
#include <msp430x14x.h>
#include <assert.h>
#else
#include "pc_emulation.h"
#endif
#include <stdio.h>
#include <stdarg.h>

#include "sys_clock.h"
/*************************************************************************
**  GLOBAL VARIABLES                                                    **
*************************************************************************/


/*************************************************************************
**  LOCAL MACROS                                                        **
*************************************************************************/

/*************************************************************************
**  LOCAL CONSTANTS                                                     **
*************************************************************************/

/*************************************************************************
**  LOCAL TYPES AND STRUCTURES                                          **
*************************************************************************/

/*************************************************************************
**  LOCAL STATIC FUNCTIONS     Used only in this file                   **
*************************************************************************/

/*************************************************************************
**  LOCAL VARIABLES                                                     **
*************************************************************************/

#if !TIMER_EXPR_MACRO_MODE

/*************************************************************************
**
**  Output:     */ Boolean /*
**
**  Function:    */ b_SyscTimerExpired(  uint32_t u32_delay , uint32_t u32_start_time )/*
**
**  Description:    has "delay" time expired since call of "SYSC_SET_SLOWTIMER_START( var_name_ )" ?
**					DOESN'T WORK if start time is past current time!
**  Remarks:        unit 1/2048 s."u16_start_time" start time is a static variable in module, where you use timer.
**                  multiple timers can be used at sime time, as long as they use their own static variables
**
*************************************************************************/ 
{
uint32_t u32_temp;
u32_temp = g_u32_SyscTimer - u32_start_time; /*its critical to calculate overflow situation right. do not modify in any way!*/
if( u32_temp >= ( u32_delay ) )
	{
	return ( TRUE );
	}
else
	{
	return ( FALSE );
	}
}
#endif

/*
 TBCCTL1-4 are used. TBCCTL0 is not used for motor control.

*************************************************************************/
void SyscStartTimerB (void) 
{
#if !PC_EMULATED_ENVIROMENT

TACTL = TASSEL_2 | ID_3 |TACLR| TAIE; /*timer A setup, halted timerA int enabled*/
TACCR0 = CURRENT_LEVEL_PWM_PERIOD; /* period, 2ms gives with 1k/47uF 5% ripple. */
TACCR1 = 1*SYSC_UNIT_1mS; /* t_on*/
TACCTL0 = CCIE; /* 2 interrupts, pwm set and reset. pulse width=ccr1, period=ccr0 */
TACCTL1 = CCIE;
TACTL |= MC_1 ; /*timerA go, up-mode*/

/*                   16bit   smclk       /8  , timer is cleared and halted.    */
TBCTL = TBCLGRP_0 | CNTL_0 | TBSSEL_2 | ID_3| TBCLR;             
TBCCTL1= SCS;
TBCCTL2= SCS;
TBCCTL3= SCS;
TBCCTL4= SCS;
DISABLE_TIMER_INT(1);
DISABLE_TIMER_INT(2);
DISABLE_TIMER_INT(3);
DISABLE_TIMER_INT(4);
TBCTL |= MC1;              /* Start Timer_B in continuous mode */
#endif
}


/*************************************************************************
**
**  Output:      */ uint32_t /*
**
**  Function:    */ u32_SyscTimeSince ( uint32_t u32_start ) /*
*************************************************************************/

{
uint32_t u32_t;
u32_t = g_u16_SyscTimer - u32_start;
return( u32_t );
}
void SyscStartClocks (void)
{
   register volatile int16_t delay;
  // wait for selected oscillator to come up.clear OSC fault flag & set delay count
  // exit loop when no fault for delay count
  for (IFG1 &= ~OFIFG, delay = 255; delay; )
    {
    if (IFG1 & OFIFG)                   // check OSC fault flag
      {
      // it's set
      IFG1 &= ~OFIFG;                   // clear OSC fault flag
      delay = 255;                      // reset delay count
      }
    else
      --delay;                          // clear, decrement delay
    }
    BCSCTL2 = SELM1 | DIVM_0 | SELS | DIVS_3; // MCLK=XT2, MCLK divider=1, SMCLK source=xt2, SMCLK divider=8

}
