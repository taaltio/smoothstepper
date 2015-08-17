/*
**
**      Used originally timer A, so many names refer to timerA.
**                  Timer changed, because asyncronous operation with timer A is no longer recommended by TI.
** measured 6.6.2005, pcb vA: 100 timerB ticks = 36us. jitter 5us.
*****************************************************************************/
#define TIMER_EXPR_MACRO_MODE 1

#ifndef __SYS_CLOCK_H
#define __SYS_CLOCK_H

#ifdef __SYS_CLOCK_C
#define EXTERN_SYSC
#else
#define EXTERN_SYSC extern
#endif


/*************************************************************************
**  GLOBAL MACROS                                                       **
*************************************************************************/
/*Using timerb hw timer*/
/*timerb clock source must be sybchronized with cpu clock.*/
#define ENABLE_TIMER_INT(_id)  TBCCTL##_id |= CCIE
#define CLEAR_PENDING_TIMER_INT(_id)  TBCCTL##_id &=~CCIFG
#define SET_IFG_TIMER_INT(_id)  TBCCTL##_id |= CCIFG
#define DISABLE_TIMER_INT(_id) TBCCTL##_id &=~CCIE
#define INT_TIMER_INCREASE(_id,_delay) TBCCR##_id=TBCCR##_id+(uint16_t)(_delay)
#define INT_TIMER_START(_id)  TBCCR##_id=TBR; /*sets next interval to maximum lenght. now the next TIMER_INCREASE works.  */
/* SyscTimePast is not a function, hw does it. expired time causes call to timerb isr and TBIV flag to be set.
 */ 
 
/*Using software timer*/
/*use this macro to start a Slow-timer with "mytimer" variable, then use function
  b_SyscSlowTimerExpired( my_time_limit, mytimer) to find out has "my_time_limit" expired.
*/
#define SYSC_TIMER_START( var_name_ )\
{                                           \
    	var_name_ = g_u16_SyscTimer;         \
}                                           /*end of macro*/ 

/*DOESN'T WORK if start time is past current time!*/
#define b_SyscTimerExpired(  _delay , _start_time )((uint16_t)(g_u16_SyscTimer - (uint16_t)(_start_time)) >=(uint16_t) ( _delay ) )

/* timer_increase moves "timer" forward from previous position. used to get accurate timer interval measuremen.
Timer must be initialized first with SET_TIMER_START . after reset and if timer has not been used and has overflown.*/
#define SYSC_TIMER_INCREASE( timer_name_, _incr ) ((timer_name_) =(timer_name_)+(int16_t)(_incr))
/* maxval_i32 means here "negative" */
#define b_SyscTimePast(  _timer )( (uint16_t)((uint16_t)( _timer )- (g_u16_SyscTimer)) >(uint16_t) ( MAXVAL_I16 ) )
//this doesn't work: _b_SyscTimePast(  _timer )( ((uint16_t)( _timer )- (g_u16_SyscTimer)) >(uint16_t) ( MAXVAL_I16 ) )

/*************************************************************************
** SyscGo()
**                  use after halt
**  Remarks:       0.5ms interval. Belongs conceptually to sys_clock module.
*************************************************************************/
#define SYSC_GO \
{\
TBCTL &= ~(MC0 + MC1);\
TBCTL |= MC1;/*counting up*/\
} /* end of macro*/
/*************************************************************************
**  Function:     SyscHalt ( void )
**  Description:   stop timer (no more timer interrupts)
**  Remarks:       you must halt timer, if timer interrupt cant' be served in time
**                 If timer counter 'overflows' it takes n.1s to retunr to correct operation
*************************************************************************/
#define SYSC_HALT TBCTL &= ~(MC0 + MC1)
/* end of macro*/


/* use with SyscTimer. Note that when using const.func. argument must be cast to u32! */
//   893 us= 100 timerb ticks
// 89400 us= 10 000 timerticks.
// f=0,111857 MHz, tick= 8,94 us.
#define SYSC_UNIT_100mS 11186 /*11185,68233*/
#define SYSC_UNIT_10mS 1119 
#define SYSC_UNIT_1mS 112 
#define SYSC_UNIT_100uS 11

/*************************************************************************
**  GLOBAL CONSTANTS                                                    **
*************************************************************************/
/*These are not really global, but timer_B interrupt handler needs these. 
 Interrupt handler is inside "main" module, and brakes structure, but otherwise 
 would be complicated.
*/
/*clock source (XT1) frequency is divided, when used as source to timer_B */
#define A_SOURCE_DIVIDER 4
/*for normal operation*/
#define INT_A_FREQ1  100000L /*unit = Hz */
#define INTA_INTERVAL1 4  /*~(32768/A_SOURCE_DIVIDER)*(1.0/INT_A_FREQ2), unit=timer_b  counts, Note that this must be integer.*/

#define TAR_MS_DIVIDER 8  /*~(32.768/A_SOURCE_DIVIDER),     TAR / tar_ms_divider = time in milliseconds. */
/*************************************************************************
**  GLOBAL VARIABLES                                                    **
*************************************************************************/
/*EXTERN_SYSC uint16_t g_u16_SyscTimer;*/  
#define g_u16_SyscTimer TBR
/*********************************************************************** **
**  GLOBAL FUNCTIONS                                                    **
*************************************************************************/
/* system clock handling, 'operating system' */
EXTERN_SYSC void SyscStartTimerB( void );
EXTERN_SYSC void SyscStartClocks (void);
/*
EXTERN_SYSC void SyscGo ( void );  replaced with equivalent macros, less stack usage. 
EXTERN_SYSC void SyscHalt ( void );
*/

/* SyscTimer functions, interval measurement in application, 0.1ms - 7s */
/* removed. use SyscSlowTimer-functions*/

/* SyscSlowTimer functions, interval measurement in application, 0.5ms - 500 hours */
EXTERN_SYSC uint32_t u32_SyscTimeSince( uint32_t start );    /* difference of "now"-timervalue, resolution timer-a_count.*/
#if !TIMER_EXPR_MACRO_MODE
EXTERN_SYSC Boolean b_SyscTimerExpired( uint32_t u32_delay , uint32_t u32_start_time ); /*has "delay" time expired since call of "
																							SYSC_SET_TIMER_START( var_name_ )" ?*/
#endif 

#endif

