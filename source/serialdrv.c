/*
*****************************************************************************
 * file: serialdrv.c
 * project: SmoothStepper 
 * Tuomas Aaltio 2005
 * serial port interface. uses FreeRtos. 
 *
 * Receiver
 * Received characters are stored to circular buffer.
 * When a complete command is received, that is a string which ends with '#'. 
 * Receiver isr sends a message to client using a queue. Isr puts a pointer to Que.
 * This pointer(points to rx buffer) tells where message ends. '#' it self is not stored.
 * Client reads command from buffer using methods serCmdGetI16, serCmdGetChar...
 * Client must know what kind of data is available next, based on syntax.
 * Methods serCmdGet* remove characters from rx-buffer. If they are not called often enough
 * rx-buffer will overflow. If they are called too ofter, rx-buffer will underflow.
 *
 * Transmitter
 * Client can used function serSendStr() to sebd strign to serial port. buffered tx and rx.
 * 
 * This module provides interface routines to the MSP430 USART in uart
 * mode.
 *
 * Originally based on "buffered uart", Copyright 2001, R O SoftWare
 * No guarantees, warrantees, or promises, implied or otherwise.
 * May be used for hobby or commercial purposes provided copyright
 * notice remains intact.
 *****************************************************************************/
#include "types.h"
#include "smooth_globals.h"
#define __SERIALDRV_C
	#if !PC_EMULATED_ENVIROMENT
#include <msp430x14x.h>
#include "hw_config.h"
#include <limits.h>
   #else
#include "pc_emulation.h"
#include <conio.h>
/*here*/
	#endif
#include <stdio.h>

#include "serialdrv.h"



/* === local vars ===================================================== */
#if !PC_EMULATED_ENVIROMENT
// Special Function Registers
#ifdef __msp430x12x
  #define U0IFG IFG2                    // USART0 RX & TX Interrupt Flag Register
  #define U0IE  IE2                     // USART0 RX & TX Interrupt Enable Register
  #define U0ME  ME1                     // USART0 RX & TX Module Enable Register
  #define U1IFG IFG2                    // USART1 RX & TX Interrupt Flag Register
  #define U1IE  IE2                     // USART1 RX & TX Interrupt Enable Register
  #define U1ME  ME2                     // USART1 RX & TX Module Enable Register
#else
  #define U0IFG IFG1                    // USART0 RX & TX Interrupt Flag Register
  #define U0IE  IE1                     // USART0 RX & TX Interrupt Enable Register
  #define U0ME  ME1                     // USART0 RX & TX Module Enable Register
  #define U1IFG IFG2                    // USART1 RX & TX Interrupt Flag Register
  #define U1IE  IE2                     // USART1 RX & TX Interrupt Enable Register
  #define U1ME  ME2                     // USART1 RX & TX Module Enable Register
#endif
#endif
/*
usart0_rx_buffer: buffer for incoming characters
There are 2 pointers to Buffer it self, "insert" and "extract" 
reader : extract++
write : insert++
buffer empty if extract=insert , when reading
buffer overrun, if next_extract=insert, when writing to the buffer.
*/
/*max lenght of single command*/
#define MAX_CMD_LEN 20

#if PC_EMULATED_ENVIROMENT
void usart0XmtIsr(void);
void usart0RcvIsr(void);
#else
__interrupt void usart0XmtIsr(void);
__interrupt void usart0RcvIsr(void);
#endif
/******************************************************************************
 *
 * Function Name: usart0Init()
 *
 * Description:  
 *    This function initializes the USART for async mode
 *
 * Calling Sequence: 
 *    baudrate divisor - use USART0_BAUD_DIV0 macro in .h file
 *    baudrate modulation - use USART0_BAUD_MOD macro in .h file
 *    mode - see typical modes in .h file
 *
 * Returns:
 *    void
 *
 * NOTE: usart0Init(USART0_BAUD_DIV(9600), USART0_BAUD_MOD(9600), USART_8N1);
 *
 *****************************************************************************/
void usart0Init(uint16_t baudDiv, uint8_t baudMod, uint8_t mode)
{
#if !PC_EMULATED_ENVIROMENT
  // enable USART0 module
  U0ME |= UTXE0 + URXE0;

  // set Port 3 pins for USART0
  P3SEL |= BIT4 + BIT5;

  // reset the USART
  UCTL0 = SWRST;

  // set the number of characters and other
  // user specified operating parameters
  UCTL0 = mode;

  // select the baudrate generator clock
  UTCTL0 = USART0_BRSEL;

  // load the modulation & baudrate divisor registers
/*UBR00=0x3E;
UBR10=0x00;
UMCTL0=0x08;
 uart0 7159090Hz 115200bps (115283bps)
*/
  UMCTL0 = baudMod;
  UBR10 = (uint8_t)(baudDiv >> 8);
  UBR00 = (uint8_t)(baudDiv >> 0);

  // init receiver contol register
  URCTL0 = 0;
   // enable receiver interrupts
  U0IE |= URXIE0;

#endif
	serResetRxBuffers();
	usart0_tx_extract_idx = usart0_tx_insert_idx = 0;
	usart0_tx_running = 0;
 }
void serResetRxBuffers(void)
{
  g_i16_ReceivedCmds = 0;
  // initialize data queues
  usart0_rx_extract_idx = usart0_rx_insert_idx = 0;
}

/******************************************************************************
 *
 * Function Name: usart0Putch()
 *
 * Description:  
 *    This function puts a character into the USART output queue for
 *    transmission.
 *
 * Calling Sequence: 
 *    character to be transmitted
 *
 * Returns:
 *    ch on success, -1 on error (queue full)
 *
 *****************************************************************************/
int16_t usart0Putch(char ch)
{
#if PC_EMULATED_ENVIROMENT
    putch(ch);
    return(1);
#else
    
  uint16_t temp;

  temp = usart0_tx_insert_idx+1;
  if( temp >= USART0_TX_BUFFER_SIZE) temp = 0;

  if (temp == usart0_tx_extract_idx)
    return -1;
                         // no room

  U0IE &= ~UTXIE0 ;
                    // disable TX interrupts

  // check if in process of sending data
  if (usart0_tx_running)
    {
    // add to queue
    usart0_tx_buffer[usart0_tx_insert_idx] = (uint8_t)ch;
    usart0_tx_insert_idx = temp;
    }
  else
    {
    // set running flag and write to output register
    usart0_tx_running = 1;
    TXBUF0 = ch;
    }

  U0IE |= UTXIE0;
                      // enable TX interrupts
    return (uint8_t)ch;
#endif
}

/******************************************************************************
 *
 * Function Name: usart0Space()
 *
 * Description:
 *    This function gets the available space in the transmit queue
 *
 * Calling Sequence: 
 *    void
 *
 * Returns:
 *    available space in the transmit queue
 *
 *****************************************************************************/
uint16_t usart0Space(void)
{
  int16_t space;

  if ((space = (usart0_tx_extract_idx - usart0_tx_insert_idx)) <= 0)
    space += USART0_TX_BUFFER_SIZE;

  return (uint16_t)(space - 1);
}

/******************************************************************************
 *
 * Function Name: usart0Puts()
 *
 * Description:  
 *    This function writes a NULL terminated 'string' to the USART output
 *    queue, returning a pointer to the next character to be written.
 *
 * Calling Sequence: 
 *    address of the string
 *
 * Returns:
 *    a pointer to the next character to be written
 *    (\0 if full string is written)
 *
 *****************************************************************************/
const char *usart0Puts(const char *string)
{
  register char ch;

  while ((ch = *string) && (usart0Putch(ch) >= 0))
    string++;

  return string;
}
#define I_TO_CHR if( ch > 9 ) ch+='A'-10; else ch+='0';
#define SEND_HEX  {while( usart0Putch(ch) < 0){	  /*nop*/  }};  /*retry if buffer is full. blocks background*/
/*writes I16 as hex (no prefix). if tx buffer is full. blocks */
void usart0WriteI16( int16_t i16 )
{
  register char ch;
#define GET_NIBBLE(_n) ch= ( i16 >>( 4 * _n ))& 0x0f ;
#if 1
    GET_NIBBLE(3) I_TO_CHR	SEND_HEX
    GET_NIBBLE(2) I_TO_CHR	SEND_HEX
    GET_NIBBLE(1) I_TO_CHR	SEND_HEX
    GET_NIBBLE(0) I_TO_CHR	SEND_HEX    
#endif
#undef GET_NIBBLE
	return;
}

/*writes I32 as hex (no prefix). if tx buffer is full. blocks */
void usart0WriteI32( int32_t i32 )
{
  register char ch;
//if(ch<'A') *p_i16+=ch-'0';else *p_i16+=ch-'A'+10;
  /*retry if buffer is full. blocks background process*/
#define GET_NIBBLE(_n) ch=ch= ( i32 >>( 4 * _n ))& 0x0f ;

#if 1
	GET_NIBBLE(7) I_TO_CHR	SEND_HEX
    GET_NIBBLE(6) I_TO_CHR	SEND_HEX
    GET_NIBBLE(5) I_TO_CHR	SEND_HEX
    GET_NIBBLE(4) I_TO_CHR	SEND_HEX
    GET_NIBBLE(3) I_TO_CHR	SEND_HEX
    GET_NIBBLE(2) I_TO_CHR	SEND_HEX
    GET_NIBBLE(1) I_TO_CHR	SEND_HEX
    GET_NIBBLE(0) I_TO_CHR	SEND_HEX    
#endif
#undef GET_NIBBLE
	return;
}
#undef I_TO_CHR
#undef SEND_HEX
/*
sends string to uart. NOTE! Halts, until there is space in the tx buffer.
*/
void serSendStr(const char *string)
{
    int16_t i;
    i=0;
    while (1){
        if( *string == 0 ) break;
//end of string
        while( usart0Putch(*string) <0 ) //is there room in tx buffer?
        {
            __no_operation();
     //if tx buffer is full, halt task, until buffer has space
        }
        string++;
        i++;

        if( i > MAX_SEND_STR_LEN){
            usart0Putch(TX_ERR_MSG_CHAR);
            break;
            }
    }
    return;
   
}
void serSendI8( int16_t i16 )
{
	if( i16>9) i16=9;
	usart0Putch( i16 + '0');
}
/******************************************************************************
 *
 * Function Name: usart0Write()
 *
 * Description:  
 *    This function writes 'count' characters from 'buffer' to the USART
 *    output queue.
 *
 * Calling Sequence: 
 *    
 *
 * Returns:
 *    0 on success, -1 if insufficient room, -2 on error
 *    NOTE: if insufficient room, no characters are written.
 *
 *****************************************************************************/
int16_t usart0Write(const char *buffer, uint16_t count)
{
  if (count > usart0Space())
    return -1;
  while (count && (usart0Putch(*buffer++) >= 0))
    count--;

  return (count ? -2 : 0);
}

/******************************************************************************
 *
 * Function Name: usart0TxEmpty()
 *
 * Description:
 *    This function returns the status of the USART transmit data
 *    registers.
 *
 * Calling Sequence: 
 *    void
 *
 * Returns:
 *    TRUE - if both the tx holding & shift registers are empty
 *    FALSE - either the tx holding or shift register is not empty
 *
 *****************************************************************************/
int16_t usart0TxEmpty(void)
{
#if !PC_EMULATED_ENVIROMENT
  return (UTCTL0 & TXEPT) == TXEPT;
#else
  return(0);
#endif
}

/******************************************************************************
 *
 * Function Name: usart0TxFlush()
 *
 * Description:
 *    This function removes all characters from the USART transmit queue
 *    (without transmitting them).
 *
 * Calling Sequence: 
 *    void
 *
 * Returns:
 *    void
 *
 *****************************************************************************/
void usart0TxFlush(void)
{
#if !PC_EMULATED_ENVIROMENT
  /* "Empty" the transmit buffer. */
  U0IE &= ~UTXIE0 ;
                    // disable TX interrupts
  usart0_tx_insert_idx = usart0_tx_extract_idx = 0;
  U0IE |= UTXIE0;
                      // enable TX interrupts
#endif
}


/******************************************************************************
 *
 * Function Name: usart0Getch()
 *
 * Description:  
 *    This function gets a character from the USART receive queue
 *
 * Calling Sequence: 
 *    void
 *
 * Returns:
 *    character on success, -1 if no character is available
 *
 *****************************************************************************/
int16_t usart0Getch(void)
{
  uint8_t ch;

  if (usart0_rx_insert_idx == usart0_rx_extract_idx) // check if character is available
    return -1;

  ch = usart0_rx_buffer[usart0_rx_extract_idx++];
// get character, bump pointer
	if(usart0_rx_extract_idx >= USART0_RX_BUFFER_SIZE) usart0_rx_extract_idx = 0;
// limit the pointer
  return ch;
}
int16_t serRxBufferCurrent ( void )
{
    return ( usart0_rx_extract_idx );
}
/*
copies the next character in the command to variable. Client must reserve space.
*/
void serCmdGetChar ( unsigned char *pc_ch )
{
    *pc_ch = usart0_rx_buffer[usart0_rx_extract_idx];
	usart0_rx_extract_idx++;
    if( usart0_rx_extract_idx >= USART0_RX_BUFFER_SIZE ) usart0_rx_extract_idx = 0;
    return;
} /*end of function*/
/*
copies the value of hexadecimal string in command to a variable. Client must reserve space.
example command_str="01ab" -> p_i16=0x01ab. Numbers can be interpret as negative .
in hex-string, "-" character is not allowed, instead use 2nd compliment format.
characters must lower case

if read fails for that reason, all characters from buffer are discarded. 
*/
void serCmdGetI16 ( int16_t *p_i16 )
{
	//hexadecimal to i16
    #define advance_char \
	{\
	usart0_rx_extract_idx++;\
    if( usart0_rx_extract_idx >= USART0_RX_BUFFER_SIZE ) usart0_rx_extract_idx = 0;\
    } /*end of macro*/

	#define ch (usart0_rx_buffer[usart0_rx_extract_idx])
    /*
	printf("%d=addr c=%c i=%x\r\n",usart0_rx_extract_idx,(usart0_rx_buffer[usart0_rx_extract_idx]),*p_i16);
    */
	//msb first.
	/*if (usart0_rx_insert_idx == usart0_rx_extract_idx) return -1;*/ /*check if character is available*/
	*p_i16 = 0;
    if(ch<'A') *p_i16+=ch-'0';
else *p_i16+=ch-'A'+10;
    *p_i16 <<= 4;
  
    advance_char
    if(ch<'A') *p_i16+= ch-'0';
else *p_i16+= ch-'A'+10;
    *p_i16 <<= 4;
      
    advance_char
    if(ch<'A') *p_i16+=ch-'0';
else *p_i16+=ch-'A'+10;
    *p_i16 <<= 4;
  
    advance_char
	if(ch<'A') *p_i16+= ch-'0';
else *p_i16+= ch-'A'+10;
       
    advance_char
    //advancing one char forward releases the last byte from serial rx buffer. must be done!
    
    #undef ch
    #undef advance_char
    return;
       
} /*end of function*/

/* reads one byte of hexadecimal format string. Result is converted to (4 bit) digit. 
note that target variable TYPE is i16 */
void serCmdGetI4 ( int16_t *p_i16 )
{
	//hexadecimal to i16
    #define advance_char \
	{\
	usart0_rx_extract_idx++;\
    if( usart0_rx_extract_idx >= USART0_RX_BUFFER_SIZE ) usart0_rx_extract_idx = 0;\
    } /*end of macro*/

	#define ch (usart0_rx_buffer[usart0_rx_extract_idx])
    /*
	printf("%d=addr c=%c i=%x\r\n",usart0_rx_extract_idx,(usart0_rx_buffer[usart0_rx_extract_idx]),*p_i16);
    */
	//msb first.
	if(ch<'A') *p_i16=ch-'0';
else *p_i16=ch-'A'+10;
    advance_char
    //advancing one char forward releases the last byte from serial rx buffer. must be done!
    
    #undef ch
    #undef advance_char
    return;
       
} /*end of function*/


/******************************************************************************
 *
 * Function Name: usart0RcvIsr(void)
 *
 * Description:  
 *    Smart usart0 receive isr for SmoothStepper.
*   timig: measured 6us, when char is not '#'
 *****************************************************************************/
#if PC_EMULATED_ENVIROMENT
void usart0RcvIsr(void)
#else
#pragma vector= USART0RX_VECTOR
__interrupt void usart0RcvIsr(void)
#endif
{
  uint16_t usart_temp;
  uint8_t usart_dummy;
DBG3_to1;
    // check status register for receive errors
  if (URCTL0 & RXERR)
    {
    // clear error flags by forcing a dummy read
    SER_ERRMSG(1); /*uart RcvIsr detected error, for exmaple RX register overrun.*/
    usart_dummy = RXBUF0;
    return;
    }
	USART_RX_PROCESS
DBG3_to0;
} /*end of funct*/

	

/******************************************************************************
 *
 * Function Name: usart0XmtIsr(void)
 *
 * Description:  
 *    usart0 transmit isr
 *
 * Calling Sequence: 
 *    void
 *
 * Returns:
 *    void
 *
 *****************************************************************************/
#if PC_EMULATED_ENVIROMENT
	void usart0XmtIsr(void)
   {}
#else
#pragma vector = USART0TX_VECTOR
__interrupt void usart0XmtIsr(void)
{
DBG1_to1;
  if (usart0_tx_insert_idx != usart0_tx_extract_idx)
    {
    TXBUF0 = usart0_tx_buffer[usart0_tx_extract_idx++];
    if( usart0_tx_extract_idx >= USART0_TX_BUFFER_SIZE) usart0_tx_extract_idx=0;
    }
  else
    usart0_tx_running = 0;
             // clear running flag
DBG1_to0;
}
#endif

#pragma vector =PORT1_VECTOR, ADC12_VECTOR, USART1TX_VECTOR, USART1RX_VECTOR, WDT_VECTOR, COMPARATORA_VECTOR, TIMERB0_VECTOR, NMI_VECTOR
__interrupt void TrapIsr(void)
{
  // this is a trap ISR - check for the interrupt cause here by
  // checking the interrupt flags, if necessary also clear the interrupt
  // flag
}

