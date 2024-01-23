/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <complex.h>

#include "m2mb_types.h"
#include "m2mb_os_types.h"
#include "m2mb_os_api.h"
#include "m2mb_os_tmr.h"
#include "m2mb_os.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_gpio.h"


#include <stdio.h>
#include <string.h>

#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"
#include "app_cfg.h"
#include "azx_ati.h"
#include "at_utils.h"

#include "m2m_log.h"
#include "m2m_utilities.h"
#include "types.h"
#include "fft.h"

/*==================================================================================================
                            LOCAL CONSTANT DEFINITION
==================================================================================================*/
#define TRUE 1
#define AZ_PORT 1
#define AT_PARSER 1 //char=8bit campioni su cui lavorare a 16bit
#define DATA_LEN 400
#define DIM 160
#define MID 80
#define VOLUME 0.005

#define GPIO_PIN_1 "6"

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
M2MB_OS_SEM_HANDLE ATLock;
INT16 over_buffer [NOVER];
extern const INT16 null_tail[FIN_FFT-DATA_LEN];
extern INT32 taskID2;
int n = sizeof(INT16); //1=byte=8bit ingresso,
M2MB_OS_TMR_HANDLE err_timer = NULL; //timer WAIT case
task_state_type task1_status=S_START;
M2MB_OS_RESULT_E lockRes;
M2MB_OS_RESULT_E osRes;
const char file_name[] = LOCALPATH "/yes16kHz.wav";
const char mode[] = "r";

M2MB_FILE_T* sfo;
INT16 recv[160];
INT16 firstrecv[48022];
INT16 voiceStartSample;

INT16 occupato;

INT32 ret;
INT32 gpio_fd_1;
M2MB_GPIO_VALUE_E val;


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
 * IMPORTANT NOTES: This process has the highest priority of all 10 user tasks.
 * 					This function runs on the user task #1 (on it's main loop).
 *					Running complex code here will block other events coming to this task.
 * ============================================================================================== */

INT32 M2M_msgProc1(INT32 type, INT32 param1, INT32 param2){



	static INT16* frame = NULL;


	switch(type)
	{
	case INIT:
	{
		if(task1_status == S_START)//controlla lo stato in cui si trova il task. S_START significa che deve essere inizializzato
		{


			AZX_LOG_CFG_T cfg =
			{
					/*.log_level*/   AZX_LOG_LEVEL_DEBUG,
					/*.log_channel*/ AZX_LOG_TO_MAIN_UART,
					/*.log_colours*/ 0
			};
			azx_log_init(&cfg);

			M2M_LOG_INFO("Initializing memory buffer to 0!\r\n");
			memset(&over_buffer[0], 0, n*NOVER);
			M2M_LOG_INFO("Welcome to SPCM Application!\r\n");
			on_timer_action();
			on_timer2_action();


			//initializing gpio
			gpio_fd_1 = m2mb_gpio_open("/dev/GPIO" GPIO_PIN_1, 0);

			if( gpio_fd_1 == -1 ){
				AZX_LOG_ERROR("Error in opening gpio!\r\n");
				return -1;
			}
			ret = m2mb_gpio_ioctl( gpio_fd_1, M2MB_GPIO_IOCTL_SET_PULL, M2MB_GPIO_PULL_UP );
			if ( ret  ) {
				AZX_LOG_ERROR("setting gpio interrupt failed!\r\n");
				m2mb_os_taskSleep( M2MB_OS_MS2TICKS( 1000 ) );
				return -1;
			}
			ret = m2mb_gpio_ioctl( gpio_fd_1, M2MB_GPIO_IOCTL_SET_DIR, M2MB_GPIO_MODE_INPUT );
			if ( ret  ) {
				AZX_LOG_ERROR("setting gpio interrupt failed!\r\n");
				m2mb_os_taskSleep( M2MB_OS_MS2TICKS( 1000 ) );
				return -1;
			}

			ret =  m2mb_gpio_multi_ioctl( gpio_fd_1,
					CMDS_ARGS( M2MB_GPIO_IOCTL_INIT_INTR, NULL,
							M2MB_GPIO_IOCTL_SET_INTR_TYPE, INTR_CB_SET,
							M2MB_GPIO_IOCTL_SET_INTR_CB, FallingTrigg_cb,
							M2MB_GPIO_IOCTL_SET_INTR_ARG, NULL,
							M2MB_GPIO_IOCTL_SET_INTR_TRIGGER, M2MB_GPIO_INTR_NEGEDGE )
			);


			CHAR rsp[100];
			M2MB_RESULT_E retVal;
			INT16 instanceID = 0; /*AT0, bound to UART by default config*/

			retVal = at_cmd_async_init(instanceID);
			if ( retVal == M2MB_RESULT_SUCCESS )
			{
				AZX_LOG_TRACE( "at_cmd_async_init() returned success value\r\n" );
			}
			else
			{
				AZX_LOG_ERROR( "at_cmd_async_init() returned failure value\r\n" );
				return -1;
			}

			//Sending command AT#DVI=1,2,1
			AZX_LOG_INFO("Sending command AT#DVI=1,2,1 in async mode\r\n");
			retVal = send_async_at_command(instanceID, "AT#DVI=1,2,1\r", rsp, sizeof(rsp));
			if ( retVal != M2MB_RESULT_SUCCESS )
			{
				AZX_LOG_ERROR( "Error sending command AT+CGMR\r\n" );
			}
			else
			{
				AZX_LOG_INFO("Command response: <%s>\r\n\r\n", rsp);
			}
			m2mb_os_taskSleep( M2MB_OS_MS2TICKS(1000) );
			AZX_LOG_INFO("Sending command AT#DVIEXT=1,0,0,0,0 in async mode\r\n");
			retVal = send_async_at_command(instanceID, "AT#DVIEXT=1,0,0,0,0\r", rsp, sizeof(rsp));
			if ( retVal != M2MB_RESULT_SUCCESS )
			{
				AZX_LOG_ERROR( "Error sending command AT#DVIEXT=1,0,0,0,0\r\n" );
			}
			else
			{
				AZX_LOG_INFO("Command response: <%s>\r\n\r\n", rsp);
			}

			// Power saving function mode: 4 disable both TX and RX
			AZX_LOG_INFO("Sending command AT+CFUN=4 in async mode\r\n");
			retVal = send_async_at_command(instanceID, "AT+CFUN=4\r", rsp, sizeof(rsp));
			if ( retVal != M2MB_RESULT_SUCCESS )
			{
				AZX_LOG_ERROR( "Error sending command AT+CFUN=4\r\n" );
			}
			else
			{
				AZX_LOG_INFO("Command response: <%s>\r\n\r\n", rsp);
			}

			retVal = at_cmd_async_deinit(instanceID);
			if ( retVal == M2MB_RESULT_SUCCESS )
			{
				AZX_LOG_TRACE( "at_cmd_async_deinit() returned success value\r\n" );
			}
			else
			{
				AZX_LOG_ERROR( "at_cmd_async_deinit() returned failure value\r\n" );
				return -1;
			}

			//8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888
			//Configuring the Maxim DVI codec and IIC registers
			AZX_LOG_INFO("CALLING MAXIM SETUP\r\n\r\n");
			maxim_setup();
		}

		if(S_START == task1_status)
		{
			task1_status=S_IDLE;
		}
	}
	break;

	case TIMER:
	{
		//m2mb_os_sem_get(ATLock, M2MB_OS_WAIT_FOREVER );
		//Enabling alsa mixer input on default card and send arec 3sec
		startup_interface();
		if(S_IDLE==task1_status)
		{
			sfo = m2mb_fs_fopen(file_name, mode);
			memset(recv,0,sizeof(recv));
			memset(firstrecv,0,sizeof(firstrecv));
			m2mb_fs_fread(firstrecv, sizeof (CHAR), sizeof(firstrecv), sfo);
			m2mb_fs_fclose (sfo);
			voiceStartSample = findVoiceStartSample(firstrecv+22); // parte da 22 perchè salta i primi byte di descrizione file wave
			M2M_LOG_INFO("VoiceStartSample:%d\r\n", voiceStartSample);
			INT16* buffer = m2mb_os_malloc((800 +1)); //161 byte  E se lo facessi direttamente INT16 (evito confusione endian NO!)
			memcpy(buffer, firstrecv+22+voiceStartSample-1600, 800); // -1600 e se parte da zero per valori negativi finisce fuori dai confini?
			azx_tasks_sendMessageToTask(1, DATA, (INT32)buffer, 800);
		}


	}break;

	case DATA:
	{
		/*
		 * riempie array di puntatori alle zone allocate con i campioni tramite setParam
		 */

		if(S_IDLE==task1_status)
		{
			setParam((INT16* ) param1);
			int i;
			for(i=1; i<98; i++){
				INT16* buffer = m2mb_os_malloc((sizeof(recv) +1));
				memcpy(buffer, firstrecv+422+(160*(i-1))+voiceStartSample-1600, sizeof(recv)); //1600 uguale a 10centesimi di secondo
				setParam((INT16* ) buffer);
			}
		}
		else
		{
			AZX_LOG_INFO("dopo par1 free\r\n");
			//m2mb_os_free((INT16* ) param1);
		}
		timer_start();  //start timer 1

	}
	break;

	case SEND:
	{/* riceve il puntatore alla zona allocata durante DATA */
		if(S_IDLE == task1_status)
		{
			/*
			 * Alloco memoria sufficiente per una finestra fft da 512 campioni (160+240+112)
			 * Copio i 240 campioni del precedente segmento (over_buffer)
			 * copio 160 campioni attuali
			 * riempio la coda con 112 campioni nulli
			 * salvo infine 240 campioni da usare come overlap per la prossimo segmento
			 */
			static int counter = 0;
			if(counter==0 ){
				INT16* buffer = (INT16* ) param1;
				frame = m2mb_os_malloc((FIN_FFT*n) + 1);
				memcpy(frame , buffer, param2);
				memcpy(frame + DATA_LEN, null_tail, 112*n);
				osRes = m2mb_os_free(buffer);
				if ( osRes != M2MB_OS_SUCCESS )
				{
					AZX_LOG_INFO("mem free ERROR\r\n");
				}
				memcpy(over_buffer, frame + DIM, NOVER*n);
				azx_tasks_sendMessageToTask(taskID2, DATA, (INT32) frame, FIN_FFT*n);
				frame = NULL;
			}
			else{
				INT16* buffer = (INT16* ) param1;
				frame = m2mb_os_malloc((FIN_FFT*n) + 1);
				memcpy(frame, over_buffer, n*NOVER);
				memcpy(frame + NOVER, buffer, param2);
				memcpy(frame + DATA_LEN, null_tail, 112*n);
				osRes = m2mb_os_free(buffer);
				if ( osRes != M2MB_OS_SUCCESS )
				{
					AZX_LOG_INFO("mem free ERROR\r\n");
				}
				memcpy(over_buffer, frame + DIM, NOVER*n);
				azx_tasks_sendMessageToTask(taskID2, DATA, (INT32) frame, FIN_FFT*n);
				frame = NULL;
			}
			counter++;
			if(counter >=98){
				counter=0;
			}
		}
	}break;

	case WAIT:
	{
		while(S_IDLE==task1_status)
		{
			task1_status = S_BUSY;
		}
	}break;

	case OK:
	{
		/*
		 * resetta il contatore dei segmenti registrati
		 * fa ripartire il task1 tramite ag INIT
		 */

		gpio_fd_1 = m2mb_gpio_open("/dev/GPIO" GPIO_PIN_1, 0);
		if( gpio_fd_1 == -1 ){
			AZX_LOG_ERROR("Error in opening gpio!\r\n");
			return -1;
		}
		ret = m2mb_gpio_ioctl( gpio_fd_1, M2MB_GPIO_IOCTL_SET_PULL, M2MB_GPIO_PULL_UP );
		if ( ret  ) {
			AZX_LOG_ERROR("setting gpio interrupt failed!\r\n");
			m2mb_os_taskSleep( M2MB_OS_MS2TICKS( 1000 ) );
			return -1;
		}
		ret = m2mb_gpio_ioctl( gpio_fd_1, M2MB_GPIO_IOCTL_SET_DIR, M2MB_GPIO_MODE_INPUT );
		if ( ret  ) {
			AZX_LOG_ERROR("setting gpio interrupt failed!\r\n");
			m2mb_os_taskSleep( M2MB_OS_MS2TICKS( 1000 ) );
			return -1;
		}

		ret =  m2mb_gpio_multi_ioctl( gpio_fd_1,
				CMDS_ARGS( M2MB_GPIO_IOCTL_INIT_INTR, NULL,
						M2MB_GPIO_IOCTL_SET_INTR_TYPE, INTR_CB_SET,
						M2MB_GPIO_IOCTL_SET_INTR_CB, FallingTrigg_cb,
						M2MB_GPIO_IOCTL_SET_INTR_ARG, NULL,
						M2MB_GPIO_IOCTL_SET_INTR_TRIGGER, M2MB_GPIO_INTR_NEGEDGE )
		);

		task1_status = S_IDLE;
		occupato = 0;
	}break;


	default:
		break;
	}
	return 0;
}


