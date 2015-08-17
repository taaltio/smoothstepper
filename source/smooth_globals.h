/******************************************************************************
 * file: smooth_globals.h
 * project: SmoothStepper
 * Tuomas Aaltio 2005
 * SmoothStepper TA 2005
 . huge amount of global variables, because they are accessed by timer isrs and background tasks.
+ project global settings, like debug-flags
+ project global variables
 */
#ifndef __GLOBALS_H
#define __GLOBALS_H

#ifdef __SMOOTHS_MAIN_C
#define EXTERN_GLB
#else
#define EXTERN_GLB extern
#endif


#define PC_EMULATED_ENVIROMENT 0
#define SW_TIMER_MODE 0 /*0=uses timer isr, 1=polled mode*/

#define MOTOR_COUNT_MAX 4			//amount of motors controlled
#define MOT_DRIVELIST_SIZE 5	//buffer size for motor drive instrution list (rows)
#define	MOT_CYCLIC_STEPS 12		// number of steps in step-tabel. depends on driver hw.

#define CURRENT_LEVEL_PWM_PERIOD   30*SYSC_UNIT_100uS
#define MAX_CURRENT_LEVEL_TON_HALT 20*SYSC_UNIT_100uS /*must be smaller than ton-run.*/
#define MAX_CURRENT_LEVEL_TON_RUN  29*SYSC_UNIT_100uS

//one array for each column in the list. currently supports only v0 (speed)
// v0=speed at beginning.
struct motor_list_item
{
	int16_t	i16_mode;
	int16_t i16_v0; /*turha??? rad/s */
	int16_t i16_v1;
	 /*for const.speed mode*/
	/*for cons.acc. mode*/
	int16_t i16_n;		/* "n" in equation (eq.13), increses at each step.*/
	uint32_t u32_c0; /* initial c value, from client. 0 means cont. velócity mode.*/
};
EXTERN_GLB volatile struct motor_list_item s_mot_list[MOTOR_COUNT_MAX][MOT_DRIVELIST_SIZE]; /*[motor][buffer ptr]*/


EXTERN_GLB volatile int16_t gi16_MotorDrvBusy; /* =1 if motors are moving*/
/*motor absolute position, unit is 1/4 step (smalles step driver can control.) */
EXTERN_GLB volatile int32_t gai32_MotorAbsPos0; 
EXTERN_GLB volatile int32_t gai32_MotorAbsPos1; 
EXTERN_GLB volatile int32_t gai32_MotorAbsPos2; 
EXTERN_GLB volatile int32_t gai32_MotorAbsPos3; 

EXTERN_GLB volatile int16_t i16_MotDrvEnable;  /* motDrv enable signal, set:motdListRowWritten() reset:motorDriver() start */
EXTERN_GLB volatile int16_t i16_HasSwappedFlags; /* bit0=motor 0. 1=has swapped segment. resetted when all have.
							 for stopped motor flag is always 1. when all flags are 1, drive_list-ptr increases*/
EXTERN_GLB volatile uint16_t u16_timerSegmentSwap;

EXTERN_GLB volatile int32_t g_i32_debugTimer;
EXTERN_GLB volatile uint16_t u16_debug;

EXTERN_GLB volatile int16_t b_MotorDriverStart;
EXTERN_GLB volatile int16_t i16_motdSegmentTime;
/*motor current levels, expressed as t_on for pwm signal, t_cycle=CURRENT_LEVEL_PWM_PERIOD */
EXTERN_GLB volatile int16_t i16_motdCurrentLevelRun;
EXTERN_GLB volatile int16_t i16_motdCurrentLevelHalt;

/* for bitwise addressing */
#define BIT_SET( _var, _bit)  ( (_var) |= 1<< (_bit) )
#define BIT_CLR( _var, _bit)  ( (_var) &= ~( 1<< (_bit)) )
#define IS_B_SET( _var, _bit) ( (_var) &(1<< (_bit)) )
/*example: n=4-> 00001111 */
#define BIT_PATTERN_N_ONES( _n) ( (1<< (_n))-1 )

#define TACC_RADIX 4
#define CI32_RADIX 16
#define LARGEST_TACCURATE  2147418112L


EXTERN_GLB volatile int16_t g_i16_ReceivedCmds; /* serialdrv->command_i | number of avaiable commands in rxbuffer*/
EXTERN_GLB volatile int16_t g_i16_UsartEcho; /* serialdrv->command_i | number of avaiable commands in rxbuffer*/
/*EXTERN_GLB volatile struct motor_item s_motor[MOTOR_COUNT_MAX];*/

/* modes.  stop, const.acc/const speed. fwd and rew.
"continue" means that previous movement continues, mode is not changed in motor-data */
enum e_MotMode{
MODE_FWD_ACC,	/*const.acceleration, direction of acc is same as velocity*/
MODE_FWD_DACC,  /*deacceleration, rotates fwd. direction of acc is opposite to the velocity */
MODE_FWD_S,		/*const.speed*/
MODE_STOP,		/*motor stops */
MODE_REW_S,
MODE_REW_ACC,	/*acceleratees, rotates rewerse*/
MODE_REW_DACC,
MODE_CONTINUE,	/* motor continues previous movement */
MODE_END		/*whole motorDriver stops, no error is generated*/
};
#define TIMER_MAX_INTERVAL 60000L // unit is timer tick. applies in const.acc.mode. if speed goes too slow, causes error.
#define MIN_CI_VALUE 4L<<CI32_RADIX /* unit timer ticks. defines maximum allowed speed at acceleration mode
 if ci would be allowed under this limit, there is danger of underflow.*/

/*motor signal-port lookup. moving fwd or rev in tabel rotates motor.*/
    #define p_bp 1
    #define p_ap 2
    #define p_b1 4
    #define p_a1 8
    #define p_b0 16
    #define p_a0 32
#define MOTOR_PORT_CURRENT_OFF BIT0|BIT1|BIT2|BIT3|BIT4|BIT5	//motor port, signals to switch currents off from both windings

#ifdef __SMOOTHS_MAIN_C
EXTERN_GLB int16_t ia_motdSteps[MOT_CYCLIC_STEPS]=
    {
	p_a0|p_a1|p_ap|p_bp,
	p_a1|p_ap|p_b0|p_bp,
	p_a0|p_ap|p_b1|p_bp,
	p_ap|p_b0|p_b1,
	p_a0|p_ap|p_b1,
	p_a1|p_ap|p_b0,
	p_a0|p_a1,
	p_a1|p_b0,
	p_a0|p_b1,
	p_b0|p_b1|p_bp,
	p_a0|p_b1|p_bp,
	p_a1|p_b0|p_bp
    };
#else
EXTERN_GLB int16_t ia_motdSteps[MOT_CYCLIC_STEPS];
#endif
    #undef p_bp
    #undef p_ap
    #undef p_b1
    #undef p_a1
    #undef p_b0
    #undef p_a0
EXTERN_GLB volatile int16_t motdMotorDrvLst_insert; //points to the write line in motor_list
EXTERN_GLB volatile int16_t motdMotorDrvLst_extract;/*points to the line which is quaranteed to be read and therefore empty
								this is used for normal buffer management (under/overrun checks)
								*/
EXTERN_GLB volatile int16_t motdMotorDrvLst_xtrnext;/*points to line to be read next. can point to a line which is not yet valid (written)
								 this is the actual "read" pointer. But it set fwd in advance, some time before actually reading.
								 */
								 
/*access motor driver scratchpad variables. add this into functions whics use motdUpdate-macro.*/
#define ACCESS_MOTORUPDATE_VARS  int16_t i16_denom;uint16_t u16_temp;int32_t i32_tmp;  /*end of macro*/

#define MOTOR_VARS(m_)\
EXTERN_GLB volatile int16_t m##m_##i16_mode;\
EXTERN_GLB volatile uint16_t m##m_##u16_timer;\
EXTERN_GLB volatile int16_t m##m_##i16_stepPos;\
EXTERN_GLB volatile int16_t m##m_##i16_n;\
EXTERN_GLB volatile int16_t m##m_##i16_accelerate;\
EXTERN_GLB volatile uint32_t m##m_##u32_ci;\
EXTERN_GLB volatile int16_t m##m_##i16_t;\
EXTERN_GLB volatile int16_t m##m_##i16_v0;
MOTOR_VARS(0)
MOTOR_VARS(1)
MOTOR_VARS(2)
MOTOR_VARS(3)
#endif
/*temporary variables for backgroud process .*/
EXTERN_GLB volatile int16_t gi16_tmp_n;
EXTERN_GLB volatile int16_t gi16_tmp_mode;
EXTERN_GLB volatile uint32_t gu32_tmp_ci;
