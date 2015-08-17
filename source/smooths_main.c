/******************************************************************************
 * file: smooths_main.c
 * project: SmoothStepper , advanced serial port controlled stepper motor controller, using MSP430F149
 * Copyright (C) Tuomas Aaltio 2005
 *
 *
 *****************************************************************************/
#define __SMOOTHS_MAIN_C

/* Standard includes. */
#include <msp430x14x.h>
#include <stdlib.h>
//#include <signal.h>
/*application includes*/
#include "types.h"
#include "serialdrv.h"
#include "smooth_globals.h"
#include "hw_config.h"
#include "sys_clock.h"
#include "command_i.h"
#include "motor_drv.h"

/*************************************************************************
**  LOCAL STATIC FUNCTIONS     Used only in this file                   **
*************************************************************************/
static void prvSetupHardware( void );
/*************************************************************************
**  LOCAL VARIABLES                                                     **
*************************************************************************/

void test1 ( int i );
/*
 * Start the demo application tasks - then start the real time scheduler.
 */
void main( void )
{
     /* Setup the hardware */
	prvSetupHardware();
    DBG0_to0; /*indicate reset part 2/2 */
    usart0Init(USART0_BAUD_DIV(57600),USART0_BAUD_MOD(57600), USART_8N1);
    DBG0_to1; /*indicate reset part 2/2 */
       
    g_i16_UsartEcho = 1;
	SyscStartTimerB();
    motdInitialize();
    
	i16_motdCurrentLevelRun= MAX_CURRENT_LEVEL_TON_HALT;
	i16_motdCurrentLevelHalt= MAX_CURRENT_LEVEL_TON_HALT;
	
    __enable_interrupt();
    DBG0_to0; /*indicate reset part 2/2 */

    usart0Puts("reset\n\r");

/* TimerB demo. 3 different frequencies.*/
  /*  test1 (1 );
    test1 (2 );
    test1 (3 );
    */
    while(1)
    {
        vTaskCommandI();
		if( i16_MotDrvEnable )
		{
			motdSegmentUpdate();
		}
        __no_operation();		
    }
}

void test1 ( int i )
{
    INT_TIMER_INCREASE( 1 ,i*100); /*timer_id, delay_lenght*/
}

#pragma vector = TIMERA0_VECTOR
__interrupt void aTimerA0Int (void)
{	
		PWM1_to1;
		PWM2_to1;
		PWM3_to1;
		PWM4_to1;
	
} /*end of function*/

#pragma vector = TIMERA1_VECTOR
__interrupt void aTimerA1Int (void)
{
   switch( TAIV )
    {
        case 2: /* CCR1 */
		PWM1_to0;
		PWM2_to0;
		PWM3_to0;
		PWM4_to0;
        break;
    }
}

#pragma vector=TIMERB1_VECTOR
__interrupt void aTimerbInt (void)
{
	ACCESS_MOTORUPDATE_VARS
	uint16_t usart_temp;
	uint8_t usart_dummy;

     /*add here __enable_interrupt() to enable nester UART interrupt. */
   switch( TBIV )
    {
        case 0: /*no interrupt*/
           __no_operation();
            break;
        case 2:
			/* serSendStr("o");usart0WriteI16( i16_HasSwappedFlags);*/
			motdMotorUpdate(0,0); /*motor id, debug mode*/
           break;
        case 4:
			motdMotorUpdate(1,0);
          break;
            /******************************************************************************************/
           
        case 6:
            //motdMotorUpdate(2);
            break;
        case 8:
            //motdMotorUpdate(3);
            break;
        case 0x0a:
           __no_operation();
            break;
        case 0x0c: /*TBCCR6*/
           __no_operation();
            break;
        case 0x0e: /*timer overflow*/
           __no_operation();
        break;
    }
}

static void prvSetupHardware( void )
{
     //system clock setup, XT2 is used, crystal is 7.15909MHz
    WDTCTL = WDTPW + WDTHOLD;             // stop Watchdog
    BCSCTL1 = 0;   // XT2 Osc is on , ACLK div=1,LFXT1 lf mode
    //aclk=lf-mode, ~32kHz(?), divider 1 , smclk= xt2/8
/*motor control ports*/
    // setup ports while waiting for (LF)XT1 to come up
    P1SEL = (0x00);     // (BIT4=MCLK@pin1.4.;all pins are port pins
    P1OUT = MOTOR_PORT_CURRENT_OFF;     // motors off
    P1DIR = (0xFF);     // all pins are outputs
    // Port 2 Inits
    P2SEL = (0x00);      // all pins are port pins
    P2OUT = MOTOR_PORT_CURRENT_OFF;
    P2DIR = (0xFF);     // all pins are outputs
        // Port 5 Inits
    P5SEL = (0x00);     // all pins are port pins
    P5OUT = MOTOR_PORT_CURRENT_OFF;
    P5DIR = (0xFF);     // all pins are outputs
    // Port 6 Inits
    P6SEL = (0x00);     // all pins are port pins
    P6OUT = MOTOR_PORT_CURRENT_OFF;
    P6DIR = (0xFF);     // all pins are outputs
/*  */
    #define HOST_TX_BIT         BIT4       // SCI0 TX output
    #define HOST_RX_BIT         BIT5       // SCI0 RX input
    #define HOST_UART_BITS      (HOST_TX_BIT + HOST_RX_BIT)
    // Port 3 Inits
    P3SEL = (HOST_UART_BITS);// enable SCI0 UART
    P3OUT = (0x00);     // start with all output pins LOW
    P3DIR = (~HOST_RX_BIT);// set I/O direction bits
    //FORK signals
    PIN_DIR_IN(P3,BIT0);
    PIN_DIR_IN(P3,BIT1);
    PIN_DIR_IN(P3,BIT2);
    PIN_DIR_IN(P3,BIT3);
    // Port 4 Inits
    P4SEL = (0x00);      // all pins are port pins
    P4DIR = (0xFF);      // all pins are outputs
    //PWM signals
    PIN_DIR_OUT(P4,BIT0);
    PIN_DIR_OUT(P4,BIT1);
    PIN_DIR_OUT(P4,BIT2);
    PIN_DIR_OUT(P4,BIT3);
    //debug signals
    PIN_DIR_OUT(P4,4);
    PIN_DIR_OUT(P4,5);
    PIN_DIR_OUT(P4,6);
    PIN_DIR_OUT(P4,7);
    PWM1_to0; /*indicate reset part 1/2 */

    SyscStartClocks();

#ifdef RELEASE
  // LAST step is re-enable the watchdog timeruse tSMCLK * 2e15 (approx 350mSec)
  WDTCTL = WDTPW + WDTCNTCL;
#endif
    }
