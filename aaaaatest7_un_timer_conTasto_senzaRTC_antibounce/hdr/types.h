/*===============================================================================================*/
/*         >>> Copyright (C) Telit Communications S.p.A. Italy All Rights Reserved. <<<          */
/*!
  @file
    types.h

  @brief
    <brief description>
  @details
    <detailed description>
  @note
    Dependencies:
    m2mb_types.h

  @author
		FabioPi
  @date
    30/11/2021
 */

/*=================================================================
#Telit Extensions
#
#Copyright (C) 2019, Telit Communications S.p.A.
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
#Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
#Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in
#the documentation and/or other materials provided with the distribution.
#
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS``AS IS'' AND ANY EXPRESS OR IMPLIED
#WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
#DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#POSSIBILITY OF SUCH DAMAGE.
#
==============================================================*/


#ifndef HDR_TYPES_H_
#define HDR_TYPES_H_

/* Global declarations ==========================================================================*/
/* Global typedefs ==============================================================================*/
typedef enum
{
  INIT,
  TIMER,
  DATA,
  WAIT,
  OK,
  SEND,
  TASK_END
} TASK_OP_TYPE;

typedef enum
{
	S_IDLE,
	S_BUSY,
	S_START,
	S_ERROR,
	S_END
} task_state_type;

typedef enum
{
  P_INT_16,
  P_DOUBLE,
  P_CPLX,
  P_ERROR
} type_switcher;

/* Global functions =============================================================================*/

#endif /* HDR_TYPES_H_ */
