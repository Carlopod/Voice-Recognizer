/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <complex.h>

#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"
#include "app_cfg.h"

#include "m2m_log.h"
#include "m2m_utilities.h"
#include "types.h"
#include "fft.h"
#include "m2mb_os.h"


/*==================================================================================================
                            LOCAL CONSTANT DEFINITION
==================================================================================================*/
#define TRUE 1
#define AZ_PORT 1
#define AT_PARSER 1
#define BIN_LEN 98
#define FIN_FFT 512

/*==================================================================================================
                            LOCAL TYPES DEFINITION
==================================================================================================*/

/*==================================================================================================
                            LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                            GLOBAL FUNCTIONS PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                            LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                            LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                            GLOBAL VARIABLES
==================================================================================================*/
cplx fft[FIN_FFT] = {0};
extern INT32 taskID1;
extern INT32 taskID3;
/*==================================================================================================
                            LOCAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/*==================================================================================================
                            GLOBAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/* =================================================================================================
 *
 * DESCRIPTION:     Handles events sent to process 1
 *
 * PARAMETERS:      type:	event id
 *                  param1: addition info
 *                  param2: addition info
 *
 * RETURNS:         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 * IMPORTANT NOTES: This function runs on the user task #2 (on it's main loop).
 *					Running complex code here will block other events coming to this task.
 * ============================================================================================== */

INT32 M2M_msgProc2(INT32 type, INT32 param1, INT32 param2)
{
	static task_state_type task2_status = S_IDLE;
	INT16* fft_win = (INT16* ) param1;
	static int n_bin = 0;

	switch(type)
	{
	case DATA:
	{
		if(S_IDLE==task2_status)
		{
			int2cplx(fft_win, fft, param2/sizeof(INT16));
			hann_window_p(fft);
			m2mb_os_free(fft_win);
			fft_p(fft, FIN_FFT);

			if(0 == n_bin){
				n_bin++;
				azx_tasks_sendMessageToTask(taskID3, INIT, (INT32)bark_spec_array(fft), BARK_BANDS*sizeof(float));
			}
			else if(n_bin >= 97){
				task2_status = S_BUSY;
				azx_tasks_sendMessageToTask(taskID1, WAIT, 0, 0);
				timer_stop();
				n_bin = 0;
				azx_tasks_sendMessageToTask(taskID3, OK, (INT32)bark_spec_array(fft), BARK_BANDS*sizeof(float));
			}
			else{
				n_bin++;
				azx_tasks_sendMessageToTask(taskID3, DATA, (INT32)bark_spec_array(fft), BARK_BANDS*sizeof(float));
			}
		}
	}
	break;



	case OK:{
		/*
		 * torna in stato S_IDLE e avvisa task1 che può fare lo stesso */
		task2_status = S_IDLE;
		azx_tasks_sendMessageToTask(taskID1, OK, 0, 0);
	}break;

	default:
		M2M_LOG_INFO("Qualcuno chiama task2 senza DATA o OK!\r\n");
		break;
	}
	return 0;
}
