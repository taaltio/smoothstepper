/******************************************************************************
 * file: command_i.c
 * Tuomas Aaltio 2005
 * uses FreeRtos. 
 *
 *command interpreter. writen to mtor_list according to UART string received
*/
#define __COMMAND_I_C

#include "types.h"
#include "smooth_globals.h"
	#if !PC_EMULATED_ENVIROMENT
//#include <io.h>
#include <msp430x14x.h>
//#include <signal.h>
#include <limits.h>
//#include <sys/inttypes.h>
   #else
#include "pc_emulation.h"
/*here*/
	#endif

#include "command_i.h"
#include "serialdrv.h"
#include "motor_drv.h"
#include "sys_clock.h"
#include "hw_config.h"
/* 
a background task 
Defines serial command syntax
executes when there is a complete command string available from SerialDrv
Then processes the command and passes it to motorDrv etc.
syntax: 
Motor moves are first put to "on hold".  the las command causes moves to be applied.
This allows moving all motors simultaneously, althought serial commands are sent at different time

Commands
---------
general: all numbers are hex-integers 2nd compl.signed, with leading zeroes, fixed with, without "0x",
		no commas between, no "-" character before  negative numbers.
example: "v00001a02ffe0", set motor 0 to speed0=1a02, speed1=ffe0, where speed1 is negative.
"i"
	version information
""
	motor position status.
	response : [motor0 pos i32][motor1 pos i32][motor2 pos i32] etc..

"v"[1/speed0 i16]
	motor move to buffer: move put to list, but not applied yet
"a" 
	const-acceleration move
"c"
	continues previous move.
";"
	flush buffer. this move is put to list. this and all previous moves are applied.
"g" 
	start moving
"e" 
	end moving
	
*/
void vTaskCommandI()
{
    unsigned char c_cmd;
    int16_t i16_id,i16_i,i16_dT,i16_c0,i16_n,i16_mode;
    int16_t rx_reset_request;   
    ACCESS_MOTORUPDATE_VARS /*can be omitted, if debug mode is not used.*/
	
	/*in normal system, loop for ever. in pc-emulation, runs once per call*/
	#if PC_EMULATED_ENVIROMENT
    while(1){
	#else
	#endif
    if( g_i16_UsartEcho )
    {
        if( (i16_i=usart0Getch()) > 0 )
            {
            usart0Putch( i16_i ); /*echo*/
            if ( i16_i == '*' )
                {
                g_i16_UsartEcho = 0; /*to normal mode.*/
                serResetRxBuffers();
                }
            }
    }
    else
    {
        if( g_i16_ReceivedCmds > 0 )
		{
            serCmdGetChar( & c_cmd );
            rx_reset_request = 0;
            switch( c_cmd )             //characters like '*' do not work properly in switch-case ( bit changes?)
            {
            case 'x':
				rx_reset_request = 1; /*in case of under/overrun this has to be done.*/
                motdInitialize();      
                serSendStr("ready;");
                break;
            case 'i':
                serSendStr("SmoothStepper v1.6;");
				break;
			case 'a':
				serCmdGetI4( & i16_id );
                /*serCmdGetI16( & i16_i );*/
				serCmdGetI16( & i16_c0 );
				serCmdGetI16( & i16_n );
				serCmdGetI4( & i16_mode );
				motdMoveToListCAcc( i16_id, i16_c0, i16_n, i16_mode);
				break;
            case 'v':
                serCmdGetI4( & i16_id );
              /*  serCmdGetI16( & i16_i );*/
				serCmdGetI16( & i16_dT );
				motdMoveToListCSpe( i16_id, i16_dT );
				
				break;
			case 'c':
                serCmdGetI4( & i16_id );
				motdMoveToListCont( i16_id );
				break;
			case 's':
                serCmdGetI4( & i16_id );
				motdMoveToListStop( i16_id );
				break;
			case ';':
				motdListRowWritten();
				break;
			case 'g':
				motdStart();
				break;
			case 'e':
				motdEnd();
				break;
			case 'r':
				motdInitialize();
				u16_debug=0;
				break;
			case 'p':
				serCmdGetI16( & i16_i );
				motdSettings( i16_i );/* segment time */
				break;
			case 'q':
				serCmdGetI4( & i16_id );
				if( i16_id > MOTOR_COUNT_MAX ) i16_id = MOTOR_COUNT_MAX;
				switch(i16_id){
					case 0:usart0WriteI32( gai32_MotorAbsPos0) ; break;	
					case 1:usart0WriteI32( gai32_MotorAbsPos1) ; break;	
					case 2:usart0WriteI32( gai32_MotorAbsPos2) ; break;	
					case 3:usart0WriteI32( gai32_MotorAbsPos3) ; break;	
				}
			   serSendStr(";");
				break;
			case 'm':
				serCmdGetI16( &i16_i );
				if( i16_i > MAX_CURRENT_LEVEL_TON_RUN ) i16_i = MAX_CURRENT_LEVEL_TON_RUN;
				if( i16_i < 0) i16_i = 0;
				i16_motdCurrentLevelRun=  i16_i;
				break;
			case 'n':
				serCmdGetI16( &i16_i );
				if( i16_i > MAX_CURRENT_LEVEL_TON_HALT ) i16_i = MAX_CURRENT_LEVEL_TON_HALT;
				if( i16_i < 0) i16_i = 0;
				i16_motdCurrentLevelHalt=  i16_i;
				break;
			case 'j': /*debug-mode, init for new segment */
				/*movement command must be already send to motor 0*/
				motdMotorDrvLst_xtrnext=0; /*normal movement commands used to get move from client*/
				motdMotorDrvLst_insert=0;
				i16_MotDrvEnable=1;
				g_i32_debugTimer=0; /*used in MotorUpdate to measure time to see end of segment (normally by real timer, in debug mode not.*/
				I_UpdateMotToNextSeg(0); /*command from move list to m0... variables.*/
				break;
			case 'k': /*debug-mode, calculates one step times. If the step was the last of segment, sends EndOfSegment='q' character. blocks */
				if( !i16_MotDrvEnable ){
				   serSendStr("q");	/*end of segment.*/
				}
				else{ /*calculate one step, send results to client. move buffer [0][0] should hold movement command. (received by commands 'v' etc.)*/					
						i16_i=TBR;
					motdMotorUpdate(0,1);/*motor id, debug mode. if segment ends, resets MotDrvEnable.*/
//					**********************************
/*					{
 if( !( m0i16_mode == MODE_FWD_S || m0i16_mode == MODE_REW_S )) {
 switch ( m0i16_mode ) {
 case MODE_FWD_ACC: case MODE_REW_ACC: m0i16_n ++;
 break;
 case MODE_FWD_DACC: m0i16_n ++;
 if( m0i16_n == 0 ) {
 m0i16_n = 1;
 m0i16_mode=MODE_REW_ACC;
 } break;
 case MODE_REW_DACC: m0i16_n ++;
 if( m0i16_n == 0 ) {
 m0i16_n = 1;
 m0i16_mode=MODE_FWD_ACC;
 } break;
 } i16_denom = ( 4 * m0i16_n )+1;
 i32_tmp =(m0u32_ci<<1) ;
 i32_tmp = i32_tmp/ i16_denom;
 m0u32_ci= m0u32_ci - i32_tmp;
 if( m0u32_ci < (4L<<16 )) {
 m0u32_ci = (4L<<16);
 } } if ( m0i16_mode ==MODE_FWD_ACC|| m0i16_mode ==MODE_FWD_DACC || m0i16_mode ==MODE_FWD_S ) {
 gai32_MotorAbsPos0++;
 m0i16_stepPos++;
 if( m0i16_stepPos >= 12 ) m0i16_stepPos = 0;
 } else{
 gai32_MotorAbsPos0--;
 m0i16_stepPos--;
 if( m0i16_stepPos < 0 ) m0i16_stepPos = 12-1;
 } u16_temp = (m0u32_ci >> 16 );
 if( u16_temp <10){
 u16_temp=100;
 {
 usart0Putch('e');
 usart0Putch('0' +4);
 usart0Putch(';');
};
} 
 {
 g_i32_debugTimer+=u16_temp;
 usart0WriteI16(u16_temp);
 } if( 0==0) P1OUT=ia_motdSteps[ m0i16_stepPos ];
{
 if( g_i32_debugTimer > i16_motdSegmentTime){
 i16_MotDrvEnable = 0;

 } } };

 
*/
//					***********************************
					i16_i=TBR-i16_i;
					serSendStr(",");
					usart0WriteI16( i16_i); /*time used to MotorUpdate call. unit clock tick. 8,94us*/
					serSendStr(";")	;									
					motdMotorDrvLst_insert=0; /*needed if command ";" was sent (listRowWritten). */
				} 
				break;
			default:
                serSendStr("?");
                serResetRxBuffers();
                break;
                
            }/*eo switch-case*/
            g_i16_ReceivedCmds--;
            /* frame check :if message was read correctly we shoud have reached the enf of message.
			commands can misalign, if in pc-emulation queue overflows.			*/			
            serCmdGetChar( & c_cmd );
            if( c_cmd != '#' ){
                serSendStr("?;");
                serResetRxBuffers();
            }
            if( rx_reset_request ){ /*only safe after frame check! */
				serResetRxBuffers();
            }
        } /* end of if-new cmd*/ 
    } /*end of normal cmd processing if{}*/
    #if PC_EMULATED_ENVIROMENT
	}	/* end of while(1)*/
	#else
	#endif
    
}/*eofunc*/
    
