/******************************************************************************
 * file: motor_drv.c
 * project: SmoothStepper
 * Tuomas Aaltio 2005
 *
 * u16_timerSegmentSwap doesn't generate interrupts so it has same x in isr and sw-timer code.
 * motor timers have different ralization in isr-version, in sw-mode they work like timerSegmentSwap.
*/
#define __MOTOR_DRV_C
#include "types.h"
#include "smooth_globals.h"
#include <stdlib.h>
#if !PC_EMULATED_ENVIROMENT
#include <msp430x14x.h>
#else
#include "pc_emulation.h"
#endif
#include <stdio.h>
#include "sys_clock.h"
#include "motor_drv.h"
#include "serialdrv.h"
#include "hw_config.h"

/*private functions*/

void UpdateMotToNextSeg( int16_t i16_id );

// motor_list contain motos speed per line.
// motor driver reads instructions form the list, and drives according to instructions
// once a line is list is done, tis' deleted' by increasing pointer motdMotorDrvLst_extract
// motors are not independent, their drive-list must be updatet at same rate.

/*common */

/*	for each motor*/
#if 0
struct motor_item
{
	/*common */
	int16_t i16_mode;	/*see MODE_* .direction, 0=not moving. +1=fwd -1=rev etc.*/
	uint16_t u16_timer;	 /* */
	int16_t i16_stepPos; /* position, in range of 0..11 , cyclic*/
	int16_t i16_n;		/* "n" in equation (eq.13), increses at each step.*/
	int16_t i16_accelerate; /* flag, acc/deacc*/
	/*for constant acceleration mode*/
	uint32_t u32_ci;		/*16.16 format, ci step #i delay, in timer ticks., changes for each step in acceleration-mode*/
	/* for const.speed-mode. */
	int16_t i16_v0;		/* [rad/s 11.5] speed. fixed point.*/
} ;
#else
/*direct addressing by macro,  */
#endif

/*timer b usage:*/
/* 1-4 for motors 0..3*/
#define TIMER_SEGSWAP 5



/*replaced by global var. gai32_MotorAbsPos int16_t ai_motor_pos[MOTOR_COUNT_MAX];motor absolut position*/

/*map motors to msp430 ports (pin). its assumed that all 8 bits in port are reserved for motor control*/
/*#define MOTOR_OUTPORT0	P1OUT
#define MOTOR_OUTPORT1	P2OUT
#define MOTOR_OUTPORT2	P5OUT
#define MOTOR_OUTPORT3	P6OUT
#define MOTOR_OUTPORT( _mot_id ) MOTOR_OUTPORT##_mot_id*/
//int16_t ia_motdPort[]={P1OUT,P2OUT,P5OUT,P6OUT};


void motdInitialize()
{
    int16_t mot,j;
    #if !SW_TIMER_MODE
    DISABLE_TIMER_INT(1);
    DISABLE_TIMER_INT(2);
    DISABLE_TIMER_INT(3);
    DISABLE_TIMER_INT(4);
    #endif
	i16_MotDrvEnable = 0;
	gi16_MotorDrvBusy = 0;
	i16_motdSegmentTime = SYSC_UNIT_100mS/2 ;
	motdMotorCurrentLevel( i16_motdCurrentLevelHalt );
	gai32_MotorAbsPos0 = 0;
	gai32_MotorAbsPos1 = 0;
	gai32_MotorAbsPos2 = 0;
	gai32_MotorAbsPos3 = 0;
	m0i16_stepPos=0;
	m1i16_stepPos=0;
	m2i16_stepPos=0;
	m3i16_stepPos=0;
    //this is not required, it's only to make debugging simpler
    for(mot=0;mot<MOTOR_COUNT_MAX;mot++){
        for(j=0;j<MOT_DRIVELIST_SIZE;j++){
            s_mot_list[mot][j].i16_v0=0;
            s_mot_list[mot][j].i16_v1=1;
        }
    }
    DBG0_to0;    DBG1_to0;    DBG2_to0;    DBG3_to0;

}

void motdMotorCurrentLevel( int16_t i16)
{
		TACCR1 = i16; /* t_on*/
}

/*
client must send "start" command before sending move-commands
start-command starts the clocks, so no pauses are allowed after this.
with out start-command motor driver timers are not set up correctly. When client stops sending commands,
it must use "end"-command.
Do not call this during move-sequence!
*/
void motdStart(void)
{
    int16_t mot,j;
    i16_MotDrvEnable = 0 ; /* not yet set. see motdListRowWritten() */
   	b_MotorDriverStart = 1; /* move to 1st segment immediately*/
    gi16_MotorDrvBusy = 1 ;
	i16_HasSwappedFlags = 0;
	motdMotorDrvLst_insert = 0;  /*althought motors will not move before ListRowWritten, the insert pointer is used before it*/
    SYSC_TIMER_START(u16_timerSegmentSwap); /*uses timer so that it doesn't gnenerate interrupts*/
	
	for(mot=0;mot<MOTOR_COUNT_MAX;mot++){
		for(j=0;j<MOT_DRIVELIST_SIZE;j++){
            s_mot_list[mot][j].i16_mode = MODE_STOP; /*otherways unused motor would take processing time.*/
        }
    }
    m0u32_ci=50; /*prevents errors in case motorUpdate is called for motor, which is in STOP.???? This happens in start-sequence.*/
    m1u32_ci=50;
    m2u32_ci=50;
    m3u32_ci=50;
    
	motdMotorCurrentLevel( i16_motdCurrentLevelRun );
    
	#if SW_TIMER_MODE
	for(mot=0;mot<MOTOR_COUNT_MAX;mot++){
		SYSC_TIMER_START( s_motor[mot].u16_timer );
	}
	#else
    #endif
}
/*
End command is placed to motor list. otherwise it could stop before last movements are finished.
END-command is placed only for motor 0 but applies to all motors.
no "new line"-cmd needed from client after "end"-command.
*/
void motdEnd(void)
{
	int16_t mot;
    #define IT s_mot_list[ mot ][motdMotorDrvLst_insert]

	/*must stop all, so motorDriver will not move motors in end. motorUpdate executes one time after END signal is processed. */
	for(mot=0;mot<MOTOR_COUNT_MAX;mot++){
		IT.i16_mode = MODE_END;
	}
	#undef IT
	motdListRowWritten(); /*special, no new line cmd needed from client after "end"-command.*/
}
/* note that after STOP-mode a special timer reset must be done, as the motor timer is in unknown position*/

/* called at background,if(MotDrvEnable) some time after all motors have changed to new segmant
must be called before the next segment swap . The 1st update after start is not done here! it is in motdListRowWritten()
   because is requires some special handling. */
void motdSegmentUpdate(void)
{
	int16_t i16_tmp;
	#define IT s_motor[mot]
 	if ( i16_HasSwappedFlags == BIT_PATTERN_N_ONES( MOTOR_COUNT_MAX ) && b_SyscTimePast( u16_timerSegmentSwap ) )
		{ /*all motors have swapped segment (by thenm self) . s_motor[] contains
		the data for this new segment, and motors are already running it. We only need to move extract-ptr and handle STOP-mode */
DBG0_to1;	
/*rSendStr(">");
usart0WriteI16(TBR);
serSendStr("<\n");*/
		SYSC_TIMER_INCREASE( u16_timerSegmentSwap, i16_motdSegmentTime);
		i16_tmp = motdMotorDrvLst_xtrnext ;
		if( i16_tmp>=MOT_DRIVELIST_SIZE ) i16_tmp=0;
		if( s_mot_list[ 0 ][ i16_tmp ].i16_mode == MODE_END ) /* xtrnext is increased normally, so here +1 is needed*/
		{
			__disable_interrupt();
			gi16_MotorDrvBusy = 0 ;
			i16_MotDrvEnable = 0;
			DISABLE_TIMER_INT(1);
			DISABLE_TIMER_INT(2);
			DISABLE_TIMER_INT(3);
			DISABLE_TIMER_INT(4);
			motdMotorCurrentLevel( i16_motdCurrentLevelHalt );
			__enable_interrupt();
			serSendStr("%");  /*just indicates that end was processed.*/
			/*BUG? maybe not good system.*/
		}
		else
		{
			i16_tmp= P3IN | (BIT4+BIT5+BIT6+BIT7);  /*opto fork signals 1...4.  bits 4...7 = 1111 */
			while( usart0Putch( (char)i16_tmp )<0 )
				{
		           __no_operation(); //if tx buffer is full, halt task, until buffer has space
				} //request next move command set from client.
			/*if the mode in current (starts now) segment is STOP, it has not been updatet to new segment.
			  also the SwappedFlag must be set. the timers will go out of sync, but that is covered by UpdateMotToNextSeg()
			  the current segmanet is not yet updated to motor, if last mode was STOP,
			  the current segment is updated to motor, if last mode was not STOP. */
			/*__disable_interrupt();*/
			/*i16_HasSwappedFlags=0;*/
	  		/* UpdateMotToNextSeg calls will set swap-flag if necessary. (stop-mode continues) */
	  		
	  		#define flaghandle(_n,_m)\
	  		if(m##_n##i16_mode==MODE_STOP){\
	  			UpdateMotToNextSegStop(_n);\
		  	}else{\
	  			BIT_CLR(i16_HasSwappedFlags, _n);\
	  		} /*eom*/
	  		
			flaghandle(0,1)
			flaghandle(1,2)
			flaghandle(2,3)
			flaghandle(3,4)
			#undef flaghandle
			
			__disable_interrupt();
			motdMotorDrvLst_extract = motdMotorDrvLst_xtrnext ; /*release (better late than newer)
																 the old line for previous segment.*/
							/*dont chekc buffer underrun here. the extraxt pointer is  allowed to point to empty line
							because data is not read yet.   */
			motdMotorDrvLst_xtrnext++;
			if( motdMotorDrvLst_xtrnext>=MOT_DRIVELIST_SIZE ) motdMotorDrvLst_xtrnext=0;
			__enable_interrupt();
   		} /*end if( end) ... else... */
    } /* end of if( segment swap).. */
	#undef IT
DBG0_to0;
} /* end of function */

/*
used only if running in polled mode. not used in isr-mode.
*/
void motdMoveMotors(void)
{
	#define IT s_motor[mot]
    if ( !i16_MotDrvEnable ) return;
	motdSegmentUpdate();

	/*for each motor, m=0..max_motors*/
	#if SW_TIMER_MODE
	for(mot=0; mot <MOTOR_COUNT_MAX; mot++)
		{
		if( !(IT.i16_mode == MODE_END || IT.i16_mode == MODE_STOP))
		else{
			motdMotorUpdate(mot);
		}  /* isr will do the call in isr-mode. */
		}
	#endif
	#undef IT
}/*end of funct*/

/*
		 stop mode handling:
go->go		normal
go->stop	motor timer isr disable
stop->stop	no change
stop->go	timer isr enable, 1st int at once

*/

/*called only duríng normal segment change, the current mode is not STOP*/
/* called from isr. enable_int not allowd */
void I_UpdateMotToNextSeg( int16_t i16_id )
{
#if PC_EMULATED_ENVIROMENT
	printf("m%d=",i16_id);
#endif
/*	#define MOT s_motor[i16_id]*/
	#define	LIST s_mot_list[i16_id][motdMotorDrvLst_xtrnext ]
    /*because of array of motors is not an array, we must hard-code the "indexing" (motor[N].mode -> mNmode) */
	#define Assign(_mot,_item,_listitem) m##_mot##_item=s_mot_list[_mot][motdMotorDrvLst_xtrnext]. _listitem
//serSendStr("\nI "); serSendI8(motdMotorDrvLst_xtrnext);
	switch( LIST.i16_mode )	/*new mode */
	{
	case MODE_CONTINUE:
		/*do nothing*/
		break;
	case MODE_STOP:
		switch( i16_id) /* go->stop. motor interrupt disabled. it will be re-enabled by UpdateMotToNextSegStop() */
			{/*i16_n is moved for debug purposes. not used for anything in STOP mode.*/
			case 0: DISABLE_TIMER_INT(1); CLEAR_PENDING_TIMER_INT( 1 );m0i16_mode = MODE_STOP;Assign(0,i16_n,i16_n);  break;
			case 1: DISABLE_TIMER_INT(2); CLEAR_PENDING_TIMER_INT( 2 );m1i16_mode = MODE_STOP;Assign(1,i16_n,i16_n);  break;
			case 2: DISABLE_TIMER_INT(3); CLEAR_PENDING_TIMER_INT( 3 );m2i16_mode = MODE_STOP;Assign(2,i16_n,i16_n);  break;
			case 3: DISABLE_TIMER_INT(4); CLEAR_PENDING_TIMER_INT( 4 );m3i16_mode = MODE_STOP;Assign(3,i16_n,i16_n);  break;
			}
		//usart0Putch( i16_id + '0' );
		break;
	default: /*all normal movements*/
		//      motor <- list
		switch( i16_id)
			{
			case 0: 
			Assign(0,i16_n,i16_n); 
			Assign(0,u32_ci,u32_c0); 
			Assign(0,i16_mode,i16_mode); 			
			break;
			case 1: 
			Assign(1,i16_n,i16_n); 
			Assign(1,u32_ci,u32_c0); 
			Assign(1,i16_mode,i16_mode); 			
			break;
			case 2: 
			Assign(2,i16_n,i16_n); 
			Assign(2,u32_ci,u32_c0); 
			Assign(2,i16_mode,i16_mode); 			
			break;
			case 3: 
			Assign(3,i16_n,i16_n); 
			Assign(3,u32_ci,u32_c0); 
			Assign(3,i16_mode,i16_mode); 			
			break;
			}
/*
		MOT.i16_n=LIST.i16_n;
		MOT.u32_ci=LIST.u32_c0;
		MOT.i16_mode=LIST.i16_mode;
*/
		break;
	} /*end of switch case*/
#if PC_EMULATED_ENVIROMENT
	printf("%d ",MOT.i16_mode);
#endif
	#undef MOT
	#undef LIST
}
/*called only segment change from STOP-mode  */
void UpdateMotToNextSegStop( int16_t i16_id )
{
	#define	LIST s_mot_list[i16_id][motdMotorDrvLst_xtrnext ]
    /*because of array of motors is not an array, we must hard-code the "indexing" (motor[N].mode -> mNmode) */
	#define Assign(_mot,_item,_listitem) m##_mot## _item=s_mot_list[_mot][motdMotorDrvLst_xtrnext]. _listitem
	/*if motor was before at STOP, the timer is out of sync and must be restarted. this affects to the length of the 1st step.*/
//if(i16_id==0) {serSendStr("\ns"); serSendI8(motdMotorDrvLst_xtrnext);}
	switch( LIST.i16_mode ) /*new mode*/
	{
	case MODE_CONTINUE:
	case MODE_STOP:
		BIT_SET(i16_HasSwappedFlags, i16_id ); /*current and new mode is stop. No timerb in will run during this segment.*/
		switch( i16_id)	 /*debug use only*/
		{
		case 0: Assign(0,i16_n,i16_n); break;
		case 1: Assign(1,i16_n,i16_n); break;
		case 2: Assign(2,i16_n,i16_n); break;
		case 3: Assign(3,i16_n,i16_n); break;
		}
		break;
	default: /*all normal movements*/
		/*new mode is non-stop. timerb isr will start running, in swap-flag reminains 0 . Swap-flag was cleared in SegmentUpdate() */
		//      motor <- list
		//serSendStr("d");
		BIT_CLR(i16_HasSwappedFlags, i16_id );
		switch( i16_id)
			{
			case 0: 
			Assign(0,i16_n,i16_n); 
			Assign(0,u32_ci,u32_c0); 
			Assign(0,i16_mode,i16_mode); 			
			break;
			case 1: 
			Assign(1,i16_n,i16_n); 
			Assign(1,u32_ci,u32_c0); 
			Assign(1,i16_mode,i16_mode); 			
			break;
			case 2: 
			Assign(2,i16_n,i16_n); 
			Assign(2,u32_ci,u32_c0); 
			Assign(2,i16_mode,i16_mode); 			
			break;
			case 3: 
			Assign(3,i16_n,i16_n); 
			Assign(3,u32_ci,u32_c0); 
			Assign(3,i16_mode,i16_mode); 			
			break;
			}
		switch( i16_id)
			{
			case 0: {SET_IFG_TIMER_INT(1);INT_TIMER_START(1);ENABLE_TIMER_INT(1);} break;
			case 1: {SET_IFG_TIMER_INT(2);INT_TIMER_START(2);ENABLE_TIMER_INT(2);} break;
			case 2: {SET_IFG_TIMER_INT(3);INT_TIMER_START(3);ENABLE_TIMER_INT(3);} break;
			case 3: {SET_IFG_TIMER_INT(4);INT_TIMER_START(4);ENABLE_TIMER_INT(4);} break;

			}
/*
		MOT.i16_n=LIST.i16_n;
		MOT.u32_ci=LIST.u32_c0;
		MOT.i16_mode=LIST.i16_mode;
*/
		break;
	} /*end of switch case*/
	#undef MOT
	#undef LIST
}

/*
call this once after all motors are done motdAddToList() calls for one cycle.
*/
/*
Client calls this when all segments moves (for each motor) are send. Then motdriver puts commands into motor drive list.
New movements from client will be placed to the next segment.
When started, a lot of actions must be done. In normal run these happend at very differet way.:
1st call to this after "go"-command is detected by flag "MotorDriverStart". Then:
-Movement commands are moved to the list (updateMotNextSeg)
-Motor timers are updatet so that the next step isr will happen (motdMotorUpdate)
-Now motor shoud step, and when it does, it clears the flag "MotorDriverStart"
 */
void motdListRowWritten(void)
{
	int16_t mot;
	uint16_t usart_temp;
	uint8_t usart_dummy;

							#if PC_EMULATED_ENVIROMENT
							printf(	/*fprintf( stream[1],*/ "rowWr: ins=%d, extr=%d t=%d\r\n",motdMotorDrvLst_insert,motdMotorDrvLst_extract,g_u16_SyscTimer);
							#endif
	if( b_MotorDriverStart ) // is 1st call when starting the run?
	{
		__disable_interrupt();
		motdMotorDrvLst_insert=1;
		motdMotorDrvLst_extract=0;
		motdMotorDrvLst_xtrnext=0; /* first MotorUpdate call from thi func uses 1st line*/
	    b_MotorDriverStart =0;
		#define IT s_motor[mot]
        usart0Putch( BIT4+BIT5+BIT6+BIT7 ); //request next move command set from client. See also UpdateSegment() which does this normally
		TBCTL &= ~MC1;/*stop timer B*/
		SYSC_TIMER_START( u16_timerSegmentSwap );   /*must do before calling motorUpdate, as it relies on this timer on segment swaps.*/
        SYSC_TIMER_INCREASE( u16_timerSegmentSwap, i16_motdSegmentTime);
		CLEAR_PENDING_TIMER_INT( 1 );
		CLEAR_PENDING_TIMER_INT( 2 );
		CLEAR_PENDING_TIMER_INT( 3 );
		CLEAR_PENDING_TIMER_INT( 4 );
		for(mot=0;mot<MOTOR_COUNT_MAX;mot++){
			USART_RX_POLL
			I_UpdateMotToNextSeg( mot ); /*work also if in 1st segment mode= STOP*/
			switch( mot){ 
				case 0:	if( m0i16_mode != MODE_STOP){INT_TIMER_START( 1 ); INT_TIMER_INCREASE(1,5) ;ENABLE_TIMER_INT( 1 );} break;
				case 1:	if( m1i16_mode != MODE_STOP){INT_TIMER_START( 2 ); INT_TIMER_INCREASE(2,5) ;ENABLE_TIMER_INT( 2 );} break;
				case 2:	if( m2i16_mode != MODE_STOP){INT_TIMER_START( 3 ); INT_TIMER_INCREASE(3,5) ;ENABLE_TIMER_INT( 3 );} break;
				case 3:	if( m3i16_mode != MODE_STOP){INT_TIMER_START( 4 ); INT_TIMER_INCREASE(4,5) ;ENABLE_TIMER_INT( 4 );} break;			
			}	   /* this sets the time of the next timerb interrupt to "very soon". 5=small, non-zero time. */
			#define flagset(_m) if( m##_m##i16_mode == MODE_END || m##_m##i16_mode == MODE_STOP){BIT_SET(i16_HasSwappedFlags, _m ) ;}
			switch( mot){
			case 0: flagset(0);break;
			case 1: flagset(1);break;
			case 2: flagset(2);break;
			case 3: flagset(3);break;
			}	/*so that driver will not wait for a stopped motor*/
			#undef flagset	
		}
		#undef IT
		motdMotorDrvLst_xtrnext = 1; /* the next MotorUpdate (by interrupt) will use the next line (maybe not yet received)*/
		i16_MotDrvEnable = 1;
		TBCTL |= MC1;              /* Start Timer_B in continuous mode */
				    /*  X <---------------------from this point forward timerBint must not be disabled for too long
										or steps are missed and timer system may stumble. a symptom of this is that
										the step interval goes very long, like 100ms gap, and the operation may resume.*/		
		__enable_interrupt();
	}
	else
	{ /*normal call (not "start")*/
			__disable_interrupt();
		motdMotorDrvLst_insert++;
		if( motdMotorDrvLst_insert >= MOT_DRIVELIST_SIZE) motdMotorDrvLst_insert=0;
		if( motdMotorDrvLst_insert == motdMotorDrvLst_extract)
		{        
		SER_ERRMSG(2); /*motdListRowWritten:buffer under/overrun. underrun can be detected here or with LUR message.proper LUR checking is too complicated.*/
		}
		__enable_interrupt();
	}
}

/* continues. copies values from previous command.*/
void motdSettings( int16_t i16_SegTime )
{
    /* add calculations here.*/
	i16_motdSegmentTime = i16_SegTime;
}
/* continues. copies values from previous command.*/
void motdMoveToListCont( int16_t i16_id )
{
    /* add calculations here.*/
	#define IT s_mot_list[i16_id][motdMotorDrvLst_insert]
	if( i16_id > MOTOR_COUNT_MAX-1) return;
	IT.i16_mode = MODE_CONTINUE;
	IT.i16_n= u16_debug; u16_debug++;
	#undef IT
}
/* stop mode. */
void motdMoveToListStop( int16_t i16_id )
{
    /* add calculations here.*/
	#define IT s_mot_list[i16_id][motdMotorDrvLst_insert]
	if( i16_id > MOTOR_COUNT_MAX-1) return;
	IT.i16_mode = MODE_STOP;
	IT.i16_n= u16_debug; u16_debug++;
	#undef IT
}
/*
Adds motor-drive instructions for single motor to drive-list.
Note! you must use also motdListRowWritten() after this. otherwise list write is not carried out properly
*/
/* adds move for constant speed-mode*/
void motdMoveToListCSpe( int16_t i16_id, int16_t i16_dT )
{
    /* add calculations here.*/
	#define IT s_mot_list[i16_id][motdMotorDrvLst_insert]
	if( i16_id > MOTOR_COUNT_MAX-1) return;
	if( i16_dT < 0 )
		IT.i16_mode=MODE_REW_S;
	else
		IT.i16_mode=MODE_FWD_S;

	if( i16_dT == -32768){
		i16_dT= 32767;
		}
	else{
		i16_dT = abs( i16_dT );	 /*IAR C v2.2 fails to calculate abs(-32768) !*/
		}
	IT.i16_n= u16_debug; u16_debug++;
	IT.u32_c0 =((uint32_t) i16_dT )<< (CI32_RADIX- TACC_RADIX);  /*dT is given in 12.4 format, converted to 16.16 format*/
 	#undef IT
}
/* adds move for constant acceleration-mode
motor direction change is not allowed durinc const.accl.move. if speed goes to 0, client must use a separate command.
*/
void motdMoveToListCAcc( int16_t i16_id, int16_t i16_c0 ,int16_t i16_n , int16_t i16_mode)
{
    /* add calculations here.*/
	if( i16_id > MOTOR_COUNT_MAX-1) SER_ERRMSG(5); /*motdMoveToListCAcc:invalid motor id*/
	if( i16_c0==0 ) SER_ERRMSG(6); /*motdMoveToListCAcc: c=0*/
	if( i16_c0 > (MAXVAL_U32 >>CI32_RADIX )>>1 )
		{
		SER_ERRMSG(7); /*motdMoveToListCAcc: c too large. (see motdMotorUpdate) */
		} 
	if( i16_n > (MAXVAL_I16 >> 2 )-1 )
		{
		i16_n = (MAXVAL_I16 >> 2 )-1; SER_ERRMSG(8); /*motdMoveToListCAcc: abs(n) too large. (see motdMotorUpdate) */
		} 
	if( -i16_n > (MAXVAL_I16 >> 2 )-1 )
		{
		i16_n = -((MAXVAL_I16 >> 2 )-1); SER_ERRMSG(8); /*motdMoveToListCAcc: abs(n) too large. (see motdMotorUpdate) */
		} 
	
	#define IT s_mot_list[i16_id][motdMotorDrvLst_insert]
	IT.i16_n = i16_n;
	IT.i16_mode = i16_mode;
	IT.u32_c0 = ((uint32_t) i16_c0 ) << CI32_RADIX;
	#undef IT
}
/*
for debug, shows contents of drive list.
*/
void motdPrintList( void )
{
	int16_t mot_id,j;
    char str[40];
	serSendStr("j:\t xxx \r\n");
    for(j=0;j<MOT_DRIVELIST_SIZE;j++)
	{
		sprintf(str,"%02d:",j);
        serSendStr(str);
       for(mot_id=0;mot_id<MOTOR_COUNT_MAX;mot_id++)
	   {
	#define IT s_mot_list[mot_id][j]
		    sprintf(str,"%d,%x,%x\t",IT.i16_mode,IT.u32_c0,IT.i16_n );
            serSendStr(str);
	#undef IT
        }
	   serSendStr("\r\n");
    }
}
