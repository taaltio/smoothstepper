// tuomaksen ajastin 2004. 2015

#ifndef HARDWARE_H
#define HARDWARE_H

#include <io.h>
#include <signal.h>
#include <iomacros.h>


//PINS
//PORT1
#define LCD_RS          BIT1
#define LCD_RW          BIT2
#define LCD_E           BIT3
#define LCD_D4          BIT4
#define LCD_D5          BIT5
#define LCD_D6          BIT6
#define LCD_D7          BIT7

//PORT2
#define LED             BIT5

//for the lcd.c module
#define LCDOUT          P1OUT
#define LCDDIR          P1DIR
#define LCDIN           P1IN
//customize this if your MSP runs at different speed.
#define LCDDELAY1MS     3*750                     // 750 for ~1ms MCLK@750kHz, , now cpu 3-4MHZ


//Port Output Register 'P1OUT, P2OUT':
#define P1OUT_INIT      0                       // Init Output data of port1
#define P2OUT_INIT      0                       // Init Output data of port2

//Port Direction Register 'P1DIR, P2DIR':
#define P1DIR_INIT      0xff                    // Init of Port1 Data-Direction Reg (Out=1 / Inp=0)
#define P2DIR_INIT      0xff // Init of Port2 Data-Direction Reg (Out=1 / Inp=0)

#define P4DIR_INIT      0xff // Init of Port2 Data-Direction Reg (Out=1 / Inp=0)

//Selection of Port or Module -Function on the Pins 'P1SEL, P2SEL'
#define P1SEL_INIT      0                       // P1-Modules:
#define P2SEL_INIT      0                       // P2-Modules:

//Interrupt capabilities of P1 and P2
#define P1IE_INIT       0                       // Interrupt Enable (0=dis 1=enabled)
#define P2IE_INIT       0                       // Interrupt Enable (0=dis 1=enabled)
#define P1IES_INIT      0                       // Interrupt Edge Select (0=pos 1=neg)
#define P2IES_INIT      0                       // Interrupt Edge Select (0=pos 1=neg)

#define IE_INIT         0
#define WDTCTL_INIT     WDTPW|WDTHOLD

#endif //HARDWARE_H
