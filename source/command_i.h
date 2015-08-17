/******************************************************************************
 * file: command_i.h
 * project: SmoothStepper 
 * Tuomas Aaltio 2005
 * serialdrv.h
 *
 * 
 *****************************************************************************/
#ifndef __COMMAND_I_H
#define __COMMAND_I_H

#ifdef __COMMAND_I_C
#define EXTERN_COMMAND_I
#else
#define EXTERN_COMMAND_I extern
#endif

EXTERN_COMMAND_I void vTaskCommandI(void );
#endif
