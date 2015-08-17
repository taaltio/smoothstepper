/*!
****************************************************************
**
*****************************************************************************/

#ifndef __TYPES_H
#define __TYPES_H

#ifdef __TYPES_C
#define EXTERN
#else
#define EXTERN extern
#endif

/*************************************************************************
**  GLOBAL DEFINES                                                      **
*************************************************************************/

#define TRUE   1
#define FALSE  0
#define MAXVAL_I16 32767      
#define MAXVAL_U16 65535     
#define MAXVAL_I32 2147483647L
#define MAXVAL_U32 4294967295L
/*************************************************************************
**  TYPE DEFINITIONS                                                    **
*************************************************************************/
/* PC_EMULATED_ENVIROMENT cant be tested as its defined afterwards.*/
#if 0
/* for ms visual c++*/
typedef unsigned char uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int uint32_t;
typedef float Float32;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef char TdBoolean;
typedef unsigned char Char;
typedef char Boolean;
#else
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef float Float32;
typedef signed char int8_t;
typedef signed int int16_t;
typedef signed long int32_t;
typedef char TdBoolean;
typedef unsigned char Char;
typedef char Boolean;

#endif

#endif
