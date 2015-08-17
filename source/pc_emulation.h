/*emulate msp430 enviroment on PC, registers and constants
This file should not be included if not using pc-emulation, as this
repelaces normal definations.
*/
#define interrupt void
#define portTickType int16_t
#define portCHAR char
#define pdFALSE 1
#define pdTRUE	0
#define URCTL0 1
#define RXERR 0
#define P1OUT 1
#define P2OUT 2
#define P3OUT 3
#define P4OUT 4
#define P5OUT 5
#define P6OUT 6
#define P7OUT 7

#define BIT0 1
#define BIT1 2
#define BIT2 4
#define BIT3 8
#define BIT4 16
#define BIT5 32
#define BIT6 64
#define BIT7 128
#define BIT8 256
/* emulated variables global , which are defided normally in a module not present in test setup.
Note! do not add here variables, which are present in test setup. they are in xxxxxtest.c.
*/
#ifdef _MAIN_C
char RXBUF0;
int16_t	queue_empty;
int16_t	queue_val;
int16_t xQSerCmd=1;
int16_t gMotorDriverSync;
FILE *stream[4];
#else
extern char RXBUF0;
extern int16_t	queue_empty;
extern int16_t	queue_val;
extern int16_t xQSerCmd;
extern int16_t gMotorDriverSync;
#include <stdio.h>
extern FILE *stream[4];
#endif

/* definitions for freeRtos-emulations */
typedef void * xTaskHandle;
#define portCHAR char

//#define  xTaskHandle int16_t

// functions defined in test_main.c
int16_t cQueueSendFromISR ( int16_t que,void * ptr,int16_t taskwoken );
signed portCHAR cQueueReceive( int16_t xQueue, void *pcBuffer, int16_t xTicksToWait );
void taskYIELD (void);
void nop(void);

// funtion protos for funcs which are in the module under test, but normally private to the module.
void usart0RcvIsr(void);


