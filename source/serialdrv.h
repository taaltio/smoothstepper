/******************************************************************************
 * file: serialdrv.h
 * project: SmoothStepper 
 * Tuomas Aaltio 2005
 * serialdrv.h
 *
 * 
 *****************************************************************************/
#ifndef __SERIALDRV_H
#define __SERIALDRV_H

#ifdef __SERIALDRV_C
#define EXTERN_SERIALDRV
#else
#define EXTERN_SERIALDRV extern
#endif

#define MAX_SEND_STR_LEN    80
#define TX_ERR_MSG_CHAR '!'

///////////////////////////////////////////////////////////////////////////////
// code is optimized for power of 2 buffer sizes (16, 32, 64, 128, ...)
// NOTE: the buffers are only used if the respective interrupt mode is
// enabled
#define USART0_RX_BUFFER_SIZE 64        // usart0 receive buffer size
#define USART0_TX_BUFFER_SIZE 64       // usart0 transmit buffer size
#define USART1_RX_BUFFER_SIZE 16        // usart1 receive buffer size
#define USART1_TX_BUFFER_SIZE 16        // usart1 transmit buffer size

///////////////////////////////////////////////////////////////////////////////
// BRCLKn may be driven by UCLKn, ACLK, or SMCLK
#define USART0_UCLK       0.0           // External Clock frequency (if used)

// uncomment ONE of the following pairs for USART0
//#define USART0_BRCLK      UCLK0
//#define USART0_BRSEL      (0)

//#define USART0_BRCLK      ACLK
//#define USART0_BRSEL      (SSEL0)

#define USART0_BRCLK      SMCLK
#define USART0_BRSEL      (SSEL1)

//////////////////////////////////////////////////////////////////////////////
// use the following macros to determine the 'baudDiv' parameter values
// for usartInit0() and usartInit1()
// CAUTION - 'baud' SHOULD ALWAYS BE A CONSTANT or
// a lot of code will be generated.
#define USART0_BAUD_DIV(baud) (uint16_t)(USART0_BRCLK / (baud))
#define USART1_BAUD_DIV(baud) (uint16_t)(USART1_BRCLK / (baud))

///////////////////////////////////////////////////////////////////////////////
// the following are used to calculate the UMOD values at compile time
#define CMOD_0(baud)    ((USART0_BRCLK / (baud)) - USART0_BAUD_DIV(baud))
#define CMOD_1(baud)    ((USART1_BRCLK / (baud)) - USART1_BAUD_DIV(baud))
#define M0_0(baud)      (CMOD_0(baud) + CMOD_0(baud))
#define M0_1(baud)      (CMOD_1(baud) + CMOD_1(baud))
#define M1_0(baud)      (M0_0(baud) + CMOD_0(baud))
#define M1_1(baud)      (M0_1(baud) + CMOD_1(baud))
#define M2_0(baud)      (M1_0(baud) + CMOD_0(baud))
#define M2_1(baud)      (M1_1(baud) + CMOD_1(baud))
#define M3_0(baud)      (M2_0(baud) + CMOD_0(baud))
#define M3_1(baud)      (M2_1(baud) + CMOD_1(baud))
#define M4_0(baud)      (M3_0(baud) + CMOD_0(baud))
#define M4_1(baud)      (M3_1(baud) + CMOD_1(baud))
#define M5_0(baud)      (M4_0(baud) + CMOD_0(baud))
#define M5_1(baud)      (M4_1(baud) + CMOD_1(baud))
#define M6_0(baud)      (M5_0(baud) + CMOD_0(baud))
#define M6_1(baud)      (M5_1(baud) + CMOD_1(baud))
#define M7_0(baud)      (M6_0(baud) + CMOD_0(baud))
#define M7_1(baud)      (M6_1(baud) + CMOD_1(baud))
#define C0_0(baud)      (uint8_t)((int16_t)M0_0(baud) ? BIT0 : 0)
#define C0_1(baud)      (uint8_t)((int16_t)M0_1(baud) ? BIT0 : 0)
#define C1_0(baud)      (uint8_t)(((int16_t)M1_0(baud) - (int16_t)M0_0(baud)) ? BIT1 : 0)
#define C1_1(baud)      (uint8_t)(((int16_t)M1_1(baud) - (int16_t)M0_1(baud)) ? BIT1 : 0)
#define C2_0(baud)      (uint8_t)(((int16_t)M2_0(baud) - (int16_t)M1_0(baud)) ? BIT2 : 0)
#define C2_1(baud)      (uint8_t)(((int16_t)M2_1(baud) - (int16_t)M1_1(baud)) ? BIT2 : 0)
#define C3_0(baud)      (uint8_t)(((int16_t)M3_0(baud) - (int16_t)M2_0(baud)) ? BIT3 : 0)
#define C3_1(baud)      (uint8_t)(((int16_t)M3_1(baud) - (int16_t)M2_1(baud)) ? BIT3 : 0)
#define C4_0(baud)      (uint8_t)(((int16_t)M4_0(baud) - (int16_t)M3_0(baud)) ? BIT4 : 0)
#define C4_1(baud)      (uint8_t)(((int16_t)M4_1(baud) - (int16_t)M3_1(baud)) ? BIT4 : 0)
#define C5_0(baud)      (uint8_t)(((int16_t)M5_0(baud) - (int16_t)M4_0(baud)) ? BIT5 : 0)
#define C5_1(baud)      (uint8_t)(((int16_t)M5_1(baud) - (int16_t)M4_1(baud)) ? BIT5 : 0)
#define C6_0(baud)      (uint8_t)(((int16_t)M6_0(baud) - (int16_t)M5_0(baud)) ? BIT6 : 0)
#define C6_1(baud)      (uint8_t)(((int16_t)M6_1(baud) - (int16_t)M5_1(baud)) ? BIT6 : 0)
#define C7_0(baud)      (uint8_t)(((int16_t)M7_0(baud) - (int16_t)M6_0(baud)) ? BIT7 : 0)
#define C7_1(baud)      (uint8_t)(((int16_t)M7_1(baud) - (int16_t)M6_1(baud)) ? BIT7 : 0)

///////////////////////////////////////////////////////////////////////////////
// use the following macros to determine the 'baudMod' parameter values
// for usartInit0() and usartInit1()
// CAUTION - 'baud' SHOULD ALWAYS BE A CONSTANT or
// a lot of code will be generated.
#define USART0_BAUD_MOD(baud) (uint8_t)(C7_0(baud) + C6_0(baud) + C5_0(baud) + \
                                        C4_0(baud) + C3_0(baud) + C2_0(baud) + \
                                        C1_0(baud) + C0_0(baud))

#define USART1_BAUD_MOD(baud) (uint8_t)(C7_1(baud) + C6_1(baud) + C5_1(baud) + \
                                        C4_1(baud) + C3_1(baud) + C2_1(baud) + \
                                        C1_1(baud) + C0_1(baud))

///////////////////////////////////////////////////////////////////////////////
// use the following macros to determine the 'mode' parameter values
// for usartInit0() and usartInit1()
#define USART_NONE  (0) 
#define USART_EVEN  (PENA + PEV)
#define USART_ODD   (PENA)
#define USART_1STOP (0)
#define USART_2STOP (SPB)
#define USART_7IBIT  (0)
#define USART_8BIT  (CHAR)

// Definitions for typical USART 'mode' settings
#define USART_8N1   (uint8_t)(USART_8BIT + USART_NONE + USART_1STOP)
#define USART_7N1   (uint8_t)(USART_7BIT + USART_NONE + USART_1STOP)
#define USART_8N2   (uint8_t)(USART_8BIT + USART_NONE + USART_2STOP)
#define USART_7N2   (uint8_t)(USART_7BIT + USART_NONE + USART_2STOP)
#define USART_8E1   (uint8_t)(USART_8BIT + USART_EVEN + USART_1STOP)
#define USART_7E1   (uint8_t)(USART_7BIT + USART_EVEN + USART_1STOP)
#define USART_8E2   (uint8_t)(USART_8BIT + USART_EVEN + USART_2STOP)
#define USART_7E2   (uint8_t)(USART_7BIT + USART_EVEN + USART_2STOP)
#define USART_8O1   (uint8_t)(USART_8BIT + USART_ODD  + USART_1STOP)
#define USART_7O1   (uint8_t)(USART_7BIT + USART_ODD  + USART_1STOP)
#define USART_8O2   (uint8_t)(USART_8BIT + USART_ODD  + USART_2STOP)
#define USART_7O2   (uint8_t)(USART_7BIT + USART_ODD  + USART_2STOP)

#define SER_ERRMSG(_code)\
{\
    usart0Putch('e'); usart0Putch('0'+_code); usart0Putch(';');\
}/*end of macro*/

#define USART_RX_PROCESS \
{\
    usart_dummy = RXBUF0; \
    if(usart_dummy=='+'){\
        /*reset serial drv (needed if invalid command was send, because it messes totally rx-buffer)*/\
        serResetRxBuffers();\
    }\
    else{\
        /*normal character,stored to buffer.*/\
        if(usart_dummy=='#'){\
            g_i16_ReceivedCmds++;\
        }\
        usart_temp = usart0_rx_insert_idx + 1;\
        if( usart_temp >= USART0_RX_BUFFER_SIZE) usart_temp=0;\
        \
        usart0_rx_buffer[usart0_rx_insert_idx] = usart_dummy;\
        if (usart_temp != usart0_rx_extract_idx){\
            usart0_rx_insert_idx = usart_temp;\
        }\
        else{\
            serSendStr("rx_OR");\
        }\
    }\
\
}/*end of macro*/
	
/*call this from isr so uart will not miss characters, when normal receive-int is disabled*/
#define USART_RX_POLL \
if( IFG1 & URXIFG0){\
USART_RX_PROCESS\
	}/*end of macro*/

/*global variables*/
EXTERN_SERIALDRV volatile uint8_t  usart0_rx_buffer[USART0_RX_BUFFER_SIZE];
EXTERN_SERIALDRV volatile  uint16_t usart0_rx_insert_idx;
EXTERN_SERIALDRV volatile  uint16_t  usart0_rx_extract_idx;
EXTERN_SERIALDRV volatile  uint8_t  usart0_tx_buffer[USART0_TX_BUFFER_SIZE];
EXTERN_SERIALDRV volatile  uint16_t usart0_tx_insert_idx;
EXTERN_SERIALDRV volatile  uint16_t usart0_tx_extract_idx;
EXTERN_SERIALDRV volatile  uint8_t  usart0_tx_running;

/*serial port interface*/

/*normal functions*/
EXTERN_SERIALDRV void serResetRxBuffers(void);
EXTERN_SERIALDRV void serSendStr(const char *string);
EXTERN_SERIALDRV void serSendI8( int16_t i16 );
EXTERN_SERIALDRV void serCmdGetI16 ( int16_t *p_i16 );
EXTERN_SERIALDRV void serCmdGetI4 ( int16_t *p_i16 );
EXTERN_SERIALDRV void serCmdGetChar ( unsigned char *pc_ch );
EXTERN_SERIALDRV void usart0WriteI32( int32_t i32 );
EXTERN_SERIALDRV void usart0WriteI16( int16_t i16 );

EXTERN_SERIALDRV int16_t serRxBufferCurrent ( void );

EXTERN_SERIALDRV void usart0Init(uint16_t baudDiv, uint8_t baudMod, uint8_t mode);
EXTERN_SERIALDRV int16_t usart0Putch(char ch);
EXTERN_SERIALDRV uint16_t usart0Space(void);
EXTERN_SERIALDRV int16_t usart0Write(const char *buffer, uint16_t count);
EXTERN_SERIALDRV int16_t usart0TxEmpty(void);
EXTERN_SERIALDRV void usart0TxFlush(void);
EXTERN_SERIALDRV int16_t usart0Getch(void);

/*not recommended for use*/
EXTERN_SERIALDRV const char *usart0Puts(const char *string);
#endif
