/*Copyright (C) 2020 Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

/**
  @file
    M2MB_main.c

  @brief
    The file contains the main user entry point of Appzone

  @details
  
  @description
    Sample application showcasing multi tasking functionalities with M2MB API. Debug prints on MAIN UART
  @version 
    1.0.2
  @note
    Start of Appzone: Entry point
    User code entry is in function M2MB_main()

  @author


  @date
    12/03/2019
*/
/* Include files ================================================================================*/
#include <stdio.h>
#include <string.h>

#include "m2mb_types.h"
#include "m2mb_os_types.h"
#include "m2mb_os_api.h"
#include "m2mb_os.h"
#include "m2mb_os_tmr.h"
#include "m2mb_ati.h"
#include "m2mb_gpio.h"


#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"

#include "app_cfg.h"

/* Local defines ================================================================================*/


/* Local typedefs ===============================================================================*/

/* Local statics ================================================================================*/
M2MB_OS_SEM_HANDLE ATLock = NULL;

INT32 taskID1, taskID2, taskID3;

/* Local function prototypes ====================================================================*/
extern INT32 M2M_msgProc1(INT32 type, INT32 param1, INT32 param2);
extern INT32 M2M_msgProc2(INT32 type, INT32 param1, INT32 param2);
extern INT32 M2M_msgProc3(INT32 type, INT32 param1, INT32 param2);



/* Static functions =============================================================================*/
/* Global functions =============================================================================*/
/*-----------------------------------------------------------------------------------------------*/

/***************************************************************************************************
   \User Entry Point of Appzone

   \param [in] Module Id

   \details Main of the appzone user
**************************************************************************************************/
void M2MB_main( int argc, char **argv )
{
	(void)argc;
	(void)argv;

	M2MB_OS_SEM_ATTR_HANDLE semAttrHandle;

	azx_tasks_init();
	azx_sleep_ms(1000);

#if 0
	/*SET output channel */
	AZX_LOG_INIT();
	AZX_LOG_INFO("Starting MultiTask demo app. This is v%s built on %s %s.\r\n",
			VERSION, __DATE__, __TIME__);

#endif
	/*Creating an InterProcess Communication (IPC) semaphore*/
	if (ATLock == NULL)
	{
		m2mb_os_sem_setAttrItem( &semAttrHandle, CMDS_ARGS(
				M2MB_OS_SEM_SEL_CMD_CREATE_ATTR,  NULL,
				M2MB_OS_SEM_SEL_CMD_COUNT, 0 /*IPC*/,
				M2MB_OS_SEM_SEL_CMD_TYPE, M2MB_OS_SEM_GEN,M2MB_OS_SEM_SEL_CMD_NAME, "taskSem"));
		m2mb_os_sem_init( &ATLock, &semAttrHandle );
	}


	taskID1 = azx_tasks_createTask((char*) "myTask1", AZX_TASKS_STACK_L, 1, AZX_TASKS_MBOX_M, M2M_msgProc1);
	taskID2 = azx_tasks_createTask((char*) "myTask2", AZX_TASKS_STACK_M, 5, AZX_TASKS_MBOX_M, M2M_msgProc2);
	taskID3 = azx_tasks_createTask((char*) "myTask3", AZX_TASKS_STACK_M, 4, AZX_TASKS_MBOX_M, M2M_msgProc3);

	azx_tasks_sendMessageToTask(taskID1, 0,0,0);
}

