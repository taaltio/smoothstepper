/******************************************************************************
 * file: motor_drv.h
 * project: SmoothStepper
 * Tuomas Aaltio 2005
 * MOTOR_DRV.h
 *
 *
 *****************************************************************************/

#ifndef __MOTOR_DRV_H
#define __MOTOR_DRV_H

#ifdef __MOTOR_DRV_C
#define EXTERN_MOTOR_DRV
#else
#define EXTERN_MOTOR_DRV extern
#endif


EXTERN_MOTOR_DRV void motdMoveMotors(void); /*used in polled mode*/
EXTERN_MOTOR_DRV void motdSegmentUpdate(void); /* used in isr-mode*/
EXTERN_MOTOR_DRV void I_UpdateMotToNextSeg( int16_t id);
EXTERN_MOTOR_DRV void UpdateMotToNextSegStop( int16_t i16_id );
/*EXTERN_MOTOR_DRV void motdMotorUpdate(int16_t _m); replaced by macro in .h file */ /*usen in isr-mode*/
EXTERN_MOTOR_DRV void motdSettings( int16_t i16_SegTime );
EXTERN_MOTOR_DRV void motdMoveToListCAcc( int16_t i16_id, int16_t i16_c0 ,int16_t i16_n , int16_t i16_mode);
EXTERN_MOTOR_DRV void motdMoveToListCSpe( int16_t i16_id, int16_t i16_speed0 );
EXTERN_MOTOR_DRV void motdMoveToListStop( int16_t i16_id );
EXTERN_MOTOR_DRV void motdMoveToListCont( int16_t i16_id );
EXTERN_MOTOR_DRV void motdListRowWritten(void);
EXTERN_MOTOR_DRV void motdInitialize();
EXTERN_MOTOR_DRV void motdPrintList( void );
EXTERN_MOTOR_DRV void motdStart( void );
EXTERN_MOTOR_DRV void motdEnd( void );
EXTERN_MOTOR_DRV void motdMotorCurrentLevel( int16_t );


/*
Incorrect behaviour, if this function is called for  motors which are not moving (END or STOP-mode)
_m = motor id 
Needs a set of automatic variables, which are declared with ACCESS_MOTORUPDATE_VARS in beginning of the
calling functio_n.
*/
#define motdMotorUpdate(_m,_debug_mode)\
{\
	/*motor step time exceeded, take next step*/\
		\
	if( !( m##_m##i16_mode == MODE_FWD_S || m##_m##i16_mode == MODE_REW_S ))\
		{\
		m##_m##i16_n ++;\
		switch ( m##_m##i16_mode )\
		{\
			case MODE_FWD_ACC:\
			case MODE_REW_ACC: /*accelerate means that abs speed is increasing. no need to worry about n->0*/\
				break;\
			case MODE_FWD_DACC:\
				if( m##_m##i16_n == 0 )\
				{ /*speed reaches zero. cross to other move, slightly bumpy step.*/\
					m##_m##i16_mode=MODE_REW_ACC;\
				}\
				break;\
			case MODE_REW_DACC:\
				if( m##_m##i16_n == 0 )\
				{\
					m##_m##i16_mode=MODE_FWD_ACC;\
				}\
				break;\
		} /* end of switch */\
		if( m##_m##i16_n != 0) {\
			i16_denom = ( 4 * m##_m##i16_n )+1;\
			i32_tmp =(m##_m##u32_ci<<1) ;\
			i32_tmp = i32_tmp/ i16_denom;\
			/*USART_RX_POLL*/\
			m##_m##u32_ci= m##_m##u32_ci -	i32_tmp;\
			}\
		/*n=0 only when crossing 0-speed. can't be calculated so uses the previous c as an aproximation*/\
		if( m##_m##u32_ci < (MIN_CI_VALUE ))\
		{\
			m##_m##u32_ci = (MIN_CI_VALUE);\
		}\
		/*end of const.acceleration calculations.*/\
	} /* oef if(accelerate...) */\
	/*move counters, depending on direction, switch-case version 44us */\
	if (\
		m##_m##i16_mode ==MODE_FWD_ACC||\
		m##_m##i16_mode ==MODE_FWD_DACC ||\
		m##_m##i16_mode ==MODE_FWD_S )\
	{\
		gai32_MotorAbsPos##_m++;\
		m##_m##i16_stepPos++;\
		if( m##_m##i16_stepPos >= MOT_CYCLIC_STEPS ) m##_m##i16_stepPos = 0;\
	}\
	else{\
		gai32_MotorAbsPos##_m--;\
		m##_m##i16_stepPos--;\
		if( m##_m##i16_stepPos < 0 ) m##_m##i16_stepPos = MOT_CYCLIC_STEPS-1;\
	}\
	u16_temp = (m##_m##u32_ci >> CI32_RADIX );\
	if( m##_m##i16_n == 0) u16_temp*=2;\
	if( u16_temp <10){ u16_temp=100; SER_ERRMSG(4);}/*motdMotorUpdate: Calculated step-time interval is below the minimum safe value*/\
	if( !_debug_mode )	{\
		if( _m==  0)  INT_TIMER_INCREASE( 1, u16_temp);\
		if( _m==  1)  INT_TIMER_INCREASE( 2, u16_temp);\
		if( _m==  2)  INT_TIMER_INCREASE( 3, u16_temp);\
		if( _m==  3)  INT_TIMER_INCREASE( 4, u16_temp);\
	}\
														else	{\
														g_i32_debugTimer+=u16_temp;\
														usart0WriteI16(u16_temp);\
															}\
	if( _m==0) P1OUT=ia_motdSteps[ m##_m##i16_stepPos ];  /*assume that whole 8bit port is available.*/\
	if( _m==1) P2OUT=ia_motdSteps[ m##_m##i16_stepPos ];  /*assume that whole 8bit port is available.*/\
	if( _m==2) P5OUT=ia_motdSteps[ m##_m##i16_stepPos ];  /*assume that whole 8bit port is available.*/\
	if( _m==3) P6OUT=ia_motdSteps[ m##_m##i16_stepPos ];  /*assume that whole 8bit port is available.*/\
	/*USART_RX_POLL*/\
	if( !_debug_mode){\
	if( b_SyscTimePast( u16_timerSegmentSwap ) && ! IS_B_SET (i16_HasSwappedFlags, _m ) )\
	{	/*motor has finished this segment. set flag so we know when all motors have finished. this motors goes to next seg. now */\
		/*motors which are stopped will not enter here, for the seg.update is done in SegmentUpdate()*/\
		/*will not change segment again, until the flag us reset. but motor is usign the new segment data.*/\
		/*USART_RX_POLL*/\
		if( motdMotorDrvLst_insert == motdMotorDrvLst_xtrnext) /*is there data available from client?*/\
	        {\
	            SER_ERRMSG(3); /*motdMotorUpdate:motor drive list buffer underrun. Client did not send move commands fast enough.*/\
				gi16_MotorDrvBusy = 0 ;\
				i16_MotDrvEnable = 0;\
				DISABLE_TIMER_INT(1);\
				DISABLE_TIMER_INT(2);\
				DISABLE_TIMER_INT(3);\
				DISABLE_TIMER_INT(4);\
				motdMotorCurrentLevel( i16_motdCurrentLevelHalt );\
				serSendStr("%");  /*just indicates that end was processed.*/\
    	    }\
        I_UpdateMotToNextSeg( _m );\
		BIT_SET(i16_HasSwappedFlags, _m) ;\
    }\
    }\
													else{\
													if( g_i32_debugTimer > i16_motdSegmentTime){\
														i16_MotDrvEnable = 0;\
														}\
													} /*eof debug-mode segment end detection.*/\
	 /*end of take-step*/\
} /* end of function */
#endif
