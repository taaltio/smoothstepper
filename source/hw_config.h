/******************************************************************************
 * SmoothStepper 
 * hw definitions: pins ,
 *****************************************************************************/
#ifndef INC_CONFIG_H
#define INC_CONFIG_H

//#include <io.h>
//#include <sys/inttypes.h>

// Handy Macros
// service the watchdog timer
#define WDOG()              (WDTCTL = (WDTCTL & 0xFF) + WDTPW + WDTCNTCL)

// Oscillator Frequencies
#define LFXT1CLK            (32e3) //not used
#define XT2CLK              (7.15909e6)
#define DCOCLK              (750e3)     // nominal Rsel=4, DCO=3, MOD=0, DCOR=0

// Clock Frequencies
#define ACLK                (LFXT1CLK / 4)
#define MCLK                XT2CLK
#define SMCLK               (XT2CLK / 8)

/*output pins*/ 
                                    // PORT|=BIT
#define PWM1_to1 P4OUT |= BIT0
#define PWM2_to1 P4OUT |= BIT1
#define PWM3_to1 P4OUT |= BIT3
#define PWM4_to1 P4OUT |= BIT2
#define DBG0_to1 P4OUT |= BIT4
#define DBG1_to1 P4OUT |= BIT5
#define DBG2_to1 P4OUT |= BIT6
#define DBG3_to1 P4OUT |= BIT7
                                    // PORT&= ~(BIT)
#define PWM1_to0 P4OUT&= ~(BIT0)
#define PWM2_to0 P4OUT&= ~(BIT1)
#define PWM3_to0 P4OUT&= ~(BIT3)
#define PWM4_to0 P4OUT&= ~(BIT2)
#define DBG0_to0 P4OUT&= ~(BIT4)
#define DBG1_to0 P4OUT&= ~(BIT5)
#define DBG2_to0 P4OUT&= ~(BIT6)
#define DBG3_to0 P4OUT&= ~(BIT7)

/*input pins*/
#define P_FORK1 (P3IN & BIT0)
#define P_FORK2 (P3IN & BIT1)
#define P_FORK3 (P3IN & BIT2)
#define P_FORK4 (P3IN & BIT3)

/*pin direction, usage: PIN_DIR_OUT( P2, BIT0 )*/
#define PIN_DIR_OUT( port_, pin_ ) port_##DIR |= pin_
#define PIN_DIR_IN( port_, pin_ ) port_##DIR &= ~pin_

#endif
