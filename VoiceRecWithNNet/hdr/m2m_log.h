/*
 * m2m_log.h
 *
 *  Created on: 13 gen 2017
 *      Author: FabioPi
 */

#ifndef HDR_M2M_LOG_H_
#define HDR_M2M_LOG_H_


#include "m2mb_types.h"
#include "azx_log.h"

/* This file provides logging utilities definitions. */


#define M2M_LOG_MASK_LEN 2 //2 bytes

/* Log level enumeration */
typedef enum
{
	_LOG_NONE_BIT 		= 0x0000,   /* Do not print anything */
	_LOG_ERROR_BIT 		= 0x0001,  	/* like VERB, but adds an "ERROR" at the beginning of the message */
	_LOG_WARN_BIT		= 0x0002,   /* like VERB, but adds a "WARNING" at the beginning of the message. */
	_LOG_INFO_BIT		= 0x0004, 	/* print the message only, without any additional info */
	_LOG_VERB_BIT		= 0x0008,	/* print source-file name, line of the instruction and task ID*/
	_LOG_DEBUG_BIT		= 0x0010,	/* Prints most of the messages, adds a "DEBUG" at the beginning of the message*/
	_LOG_DEBUG_MORE_BIT = 0x0020, 	/* Prints every message, adds a "DEBUG" at the beginning of the message*/

	_LOG_LWIP_BIT		= 0x1000,	/*LWIP prints*/
	_LOG_MAX_BIT   		= 0xFFFF

} M2M_LOG_LEVEL_E;


#define LOG_NONE_MASK 			_LOG_NONE_BIT
#define LOG_ERROR_MASK 			_LOG_ERROR_BIT
#define LOG_WARNING_MASK 		_LOG_ERROR_BIT 		| _LOG_WARN_BIT
#define LOG_INFO_MASK 			LOG_WARNING_MASK 	| _LOG_INFO_BIT  //Common log level
#define LOG_VERB_MASK 			LOG_INFO_MASK 		| _LOG_VERB_BIT  //verbose log level
#define LOG_DEBUG_MASK 			LOG_VERB_MASK 		| _LOG_DEBUG_BIT  //debug log level
#define LOG_DEBUG_MORE_MASK 	LOG_DEBUG_MASK 		| _LOG_DEBUG_MORE_BIT  //trace log level




/*Log errors*/
typedef enum
{
	BUF_ALLOC_ERROR = -100,
	NOT_INIT_ERROR,
	USB_CABLE_NOT_PLUG,
	NO_MORE_USB_INSTANCE,
	CANNOT_OPEN_USB_CH,

	MAX_ERROR_LEN
} _T_M2M_LOG_ERRORS;


typedef INT32 (*_m2m_log_function)(const char *, void*);


//typedef INT32 (*_m2m_usb_log_function)(M2M_USB_CH, unsigned char *);


typedef enum
{
	_LOG_UART_MAIN,
	_LOG_UART_AUX,
	_LOG_USB0,
	_LOG_USB1,
	_LOG_USB2,
	_LOG_USB3,
	_LOG_USB4,
	_LOG_USB5,
	_LOG_USB_AUTO,
	_LOG_USB_DEFAULT,

	_LOG_MAX
} M2M_LOG_HANDLE_T;


typedef struct
{
	UINT32 				logMask;
	M2M_LOG_HANDLE_T 	channel;
}M2M_LOG_CFG;


typedef struct
{
	UINT32 				gLog_Mask;
	M2M_LOG_HANDLE_T 	gLog_Channel;
	INT8				isInit;
} _M2M_LOG_STRUCT;


/* PUBLIC functions*/






/* MACROS */


#define M2M_LOG_ERROR AZX_LOG_ERROR
#define M2M_LOG_WARN AZX_LOG_WARN
#define M2M_LOG_INFO AZX_LOG_INFO
#define M2M_LOG_VERB AZX_LOG_DEBUG
#define M2M_LOG_DEBUG AZX_LOG_TRACE
#define M2M_LOG_DEBUG_MORE AZX_LOG_TRACE



#define M2M_SET_LOG_LEVEL_MASK(m, L) 	(m |= (1 << L))
#define M2M_UNSET_LOG_LEVEL_MASK(m, L) 	(m &= (~(1<< L)))
#define M2M_IS_LOG_LEVEL_SET(m, L) 		(m & L) ==  L

#endif /* HDR_M2M_LOG_H_ */
