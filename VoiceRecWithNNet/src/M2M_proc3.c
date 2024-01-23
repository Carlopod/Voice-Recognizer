/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <complex.h>


#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"
#include "app_cfg.h"
#include "at_utils.h"

#include "m2mb_fs_stdio.h"
#include "m2mb_ati.h"
#include "m2m_log.h"
#include "m2m_utilities.h"
#include "m2mb_os_api.h"
#include "types.h"
#include "fft.h"
#include "m2mb_gpio.h"



/*==================================================================================================
                            LOCAL CONSTANT DEFINITION
==================================================================================================*/

#define HEIGHT 98
#define WIDTH 50
#define DEPTH 12
#define FILTER_DIM 3
#define FILTER_CHANNEL 1

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
extern INT32 taskID2;
float *inputAllElements;
float ***input;
M2MB_FILE_T* weights;
M2MB_FILE_T* bias_sfo;
M2MB_FILE_T* tmean_sfo;
M2MB_FILE_T* tvar_sfo;
M2MB_FILE_T* offset_sfo;
M2MB_FILE_T* scale_sfo;
const char weights_file[] = LOCALPATH "/weights.bin";
const char bias_file[] = LOCALPATH "/bias.bin";
const char tmean_file[] = LOCALPATH "/trainedMean.bin";
const char tvar_file[] = LOCALPATH "/trainedVariance.bin";
const char offset_file[] = LOCALPATH "/offset.bin";
const char scale_file[] = LOCALPATH "/scale.bin";
extern char mode[];

float* imageinput;

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
 * IMPORTANT NOTES: This function runs on the user task #3 (on it's main loop).
 *					Running complex code here will block other events coming to this task.
 * ============================================================================================== */

INT32 M2M_msgProc3(INT32 type, INT32 param1, INT32 param2)
{
	/*
	 * Converte l'array di float in ingresso (param1) in array di char e lo invia ad un altro task per la stampa
	 * array_spec : � un casting dell'array il cui indirizzo � passato come param1;
	 * 	ho passato un INT32 e voglio che venga interpretato come un puntatore a float;
	 * 	in pratica qui sto dicendo che array_spec punta ad una zona di memoria in cui � salvato un float (param1);
	 * 	da notare che qui non sto copiando un array, sto assegnando ad una variabile l'indirizzo di un'altra (entrambe punteranno alla stessa zona di memoria).
	 *
	 */

	float * array_spec= (float *) param1;

	switch(type){
	case INIT:{
		int z = 1, x = HEIGHT, y = WIDTH ;
		//  Array Iterators
		int i, k;
		tmean_sfo = m2mb_fs_fopen(tmean_file, mode);
		imageinput = m2mb_os_malloc(1 * sizeof(float));
		m2mb_fs_fread(imageinput, sizeof (float), 1, tmean_sfo);
		//imageinput = layer1;
		//  Allocate 3D Array

		inputAllElements = m2mb_os_malloc(x * y * z * sizeof(float));
		input = m2mb_os_malloc(z * sizeof(float **));

		for(k = 0; k < z; k++)
		{
			input[k] = m2mb_os_malloc(x * sizeof(float *));
			for(i = 0; i < x; i++)
			{
				input[k][i] = inputAllElements + (k * x * y) + (i * y);
			}
		}

		bid_spec_print(  input, array_spec, param2/sizeof(float), *imageinput);
	}break;

	case DATA:{
		bid_spec_print( input, array_spec, param2/sizeof(float), *imageinput);
	}break;

	case OK:{
		//avvisa il task2 che a sua volta avvisa task1

		bid_spec_print( input, array_spec, param2/sizeof(float), *imageinput);

		//=================================================================
		int z = DEPTH, x = HEIGHT, y = WIDTH ;
		int i, j, k, l;

		float *outputAllElements = m2mb_os_malloc(x * y * z * sizeof(float));
		float ***output = m2mb_os_malloc(z * sizeof(float **));

		for(k = 0; k < z; k++)
		{
			output[k] = m2mb_os_malloc(x * sizeof(float *));

			for(i = 0; i < x; i++)
			{
				output[k][i] = outputAllElements + (k * x * y) + (i * y);
			}
		}

		for(k = 0; k < z; k++)
		{
			for(i = 0; i < x; i++)
			{
				for(j = 0; j < y; j++)
				{
					output[k][i][j] = 0;
				}
			}
		}


		//=================================================================
		int w = 12; // w = numero blocchi 3D
		x=3;
		y=3;
		z=1;
		//  Allocate 4D Array
		float *filterAllElements = m2mb_os_malloc(x * y * z * w * sizeof(float));
		float ****filter = m2mb_os_malloc(w * sizeof(float ***));

		for(l = 0; l < w; l++)
		{
			filter[l] = m2mb_os_malloc(z * sizeof(float **));

			for(k = 0; k < z; k++)
			{
				filter[l][k] = m2mb_os_malloc(x * sizeof(float *));

				for(i = 0; i < x; i++)
				{
					filter[l][k][i] = filterAllElements + (l * z * x * y) + (k * x * y) + (i * y);
				}
			}
		}
		weights = m2mb_fs_fopen(weights_file, mode);
		m2mb_fs_fread(filterAllElements, sizeof (CHAR), 432, weights);



		//=================================================================
		float *bias = m2mb_os_malloc(192 * sizeof(float));
		bias_sfo = m2mb_fs_fopen(bias_file, mode);
		m2mb_fs_fread(bias, sizeof (CHAR), 768, bias_sfo);
		m2mb_fs_fclose (bias_sfo);

		float *tmean = m2mb_os_malloc(180 * sizeof(float));

		m2mb_fs_fread(tmean, sizeof (CHAR), 720, tmean_sfo);
		m2mb_fs_fclose (tmean_sfo);


		float *tvar = m2mb_os_malloc(180 * sizeof(float));
		tvar_sfo = m2mb_fs_fopen(tvar_file, mode);
		m2mb_fs_fread(tvar, sizeof (CHAR), 720, tvar_sfo);
		m2mb_fs_fclose (tvar_sfo);


		float *offset = m2mb_os_malloc(180 * sizeof(float));
		offset_sfo = m2mb_fs_fopen(offset_file, mode);
		m2mb_fs_fread(offset, sizeof (CHAR), 720, offset_sfo);
		m2mb_fs_fclose (offset_sfo);


		float *scale = m2mb_os_malloc(180 * sizeof(float));
		scale_sfo = m2mb_fs_fopen(scale_file, mode);
		m2mb_fs_fread(scale, sizeof (CHAR), 720, scale_sfo);
		m2mb_fs_fclose (scale_sfo);
		// 504 msec ------403------------------------------------------------------------




		int channel = 1;
		convolution(input, output, filter, HEIGHT, WIDTH, 12, channel, bias, 0);
		//572 msec ----------------------------------------------------------------------------------------------
		// libero memoria
		m2mb_os_free(inputAllElements);
		for(k = 0; k < 1; k++)
		{
			m2mb_os_free(input[k]);
		}
		m2mb_os_free (input);

		w=12;
		z=1;
		//  Deallocate 3D array

		m2mb_os_free(filterAllElements);

		for(l = 0; l < w; l++)
		{
			for(k = 0; k < z; k++)
			{
				m2mb_os_free(filter[l][k]);
			}
			m2mb_os_free(filter[l]);
		}
		m2mb_os_free (filter);
		// --------------447msec----------------------------------------------------------------


		batchAndRelu(output,98,50,12,tmean,tvar,offset,scale, 0);


		//---------------474msec ------------455msec opt relu
		maxPooling(output,98,50,12);


		//605 msec--------484msec-----------467optrelu-----------------------------------------------------------------


		//-.-.-.-..-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.--.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.


		x=49;
		y=25;
		z=24;
		float *output2AllElements = m2mb_os_malloc(x * y * z * sizeof(float));
		float ***output2 = m2mb_os_malloc(z * sizeof(float **));

		for(k = 0; k < z; k++)
		{
			output2[k] = m2mb_os_malloc(x * sizeof(float *));

			for(i = 0; i < x; i++)
			{
				output2[k][i] = output2AllElements + (k * x * y) + (i * y);
			}
		}

		for(k = 0; k < z; k++)
		{
			for(i = 0; i < x; i++)
			{
				for(j = 0; j < y; j++)
				{
					output2[k][i][j] = 0;
				}
			}
		}

		w = 24; // w = numero blocchi 3D
		x=3;
		y=3;
		z=12;
		//  Allocate  4D Array
		float *filter2AllElements = m2mb_os_malloc(x * y * z * w * sizeof(float));
		float ****filter2 = m2mb_os_malloc(w * sizeof(float ***));
		for(l = 0; l < w; l++)
		{
			filter2[l] = m2mb_os_malloc(z * sizeof(float **));

			for(k = 0; k < z; k++)
			{
				filter2[l][k] = m2mb_os_malloc(x * sizeof(float *));

				for(i = 0; i < x; i++)
				{
					filter2[l][k][i] = filter2AllElements + (l * z * x * y) + (k * x * y) + (i * y);
				}
			}
		}
		//605msec -------------------------------------------------------------------------------------------
		m2mb_fs_fread(filter2AllElements, sizeof (CHAR), 10368, weights);
		//607 ------------------------------------------------------------------------------------------

		channel = 12;
		convolution(output, output2, filter2, 49, 25, 24, channel, bias, 12);


		// 900msec ---------------644-----------------------------------------------------------------------------------

		//  Deallocate 3D array
		m2mb_os_free(outputAllElements);
		for(k = 0; k < 12; k++)
		{
			m2mb_os_free(output[k]);
		}
		m2mb_os_free (output);

		w=24;
		z=12;
		//  Deallocate 3D array

		m2mb_os_free(filter2AllElements);

		for(l = 0; l < w; l++)
		{
			for(k = 0; k < z; k++)
			{
				m2mb_os_free(filter2[l][k]);
			}
			m2mb_os_free(filter2[l]);
		}
		m2mb_os_free (filter2);

		batchAndRelu(output2,49,25,24,tmean,tvar,offset,scale, 12);
		maxPooling2(output2,49,25,24);



		//915 -----------------654msec--------------------------------------------------------------------------
		//-.-.-.-..-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.--.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

#if 1
		x=25;
		y=13;
		z=48;
		float *output3AllElements = m2mb_os_malloc(x * y * z * sizeof(float));
		float ***output3 = m2mb_os_malloc(z * sizeof(float **));

		for(k = 0; k < z; k++)
		{
			output3[k] = m2mb_os_malloc(x * sizeof(float *));

			for(i = 0; i < x; i++)
			{
				output3[k][i] = output3AllElements + (k * x * y) + (i * y);
			}
		}

		for(k = 0; k < z; k++)
		{
			for(i = 0; i < x; i++)
			{
				for(j = 0; j < y; j++)
				{
					output3[k][i][j] = 0;
				}
			}
		}

		w = 48; // w = numero blocchi 3D
		x=3;
		y=3;
		z=24;
		//  Allocate  4D Array
		float *filter3AllElements = m2mb_os_malloc(x * y * z * w * sizeof(float));
		float ****filter3 = m2mb_os_malloc(w * sizeof(float ***));
		for(l = 0; l < w; l++)
		{
			filter3[l] = m2mb_os_malloc(z * sizeof(float **));

			for(k = 0; k < z; k++)
			{
				filter3[l][k] = m2mb_os_malloc(x * sizeof(float *));

				for(i = 0; i < x; i++)
				{
					filter3[l][k][i] = filter3AllElements + (l * z * x * y) + (k * x * y) + (i * y);
				}
			}
		}
		m2mb_fs_fread(filter3AllElements, sizeof (CHAR), 41472, weights);
		//916msec-------------------------------------------------------------------------------------------
		channel = 24;
		convolution(output2, output3, filter3, 25, 13, 48, channel, bias, 36);


		//1220msec ---------------832msec-----------------------------------------------------------------------------
		//  Deallocate 3D array
		m2mb_os_free(output2AllElements);
		for(k = 0; k < z; k++) //z=24
		{
			m2mb_os_free(output2[k]);
		}
		m2mb_os_free (output2);

		w=48;
		z=24;
		//  Deallocate 3D array

		m2mb_os_free(filter3AllElements);

		for(l = 0; l < w; l++)
		{
			for(k = 0; k < z; k++)
			{
				m2mb_os_free(filter3[l][k]);
			}
			m2mb_os_free(filter3[l]);
		}
		m2mb_os_free (filter3);

		batchAndRelu(output3,25,13,48,tmean,tvar,offset,scale, 36);
		maxPooling2(output3,25,13,48);
#endif
		//-.-.-.-..-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.--.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

#if 1
		x=13;
		y=7;
		z=48;
		float *output4AllElements = m2mb_os_malloc(x * y * z * sizeof(float));
		float ***output4 = m2mb_os_malloc(z * sizeof(float **));

		for(k = 0; k < z; k++)
		{
			output4[k] = m2mb_os_malloc(x * sizeof(float *));

			for(i = 0; i < x; i++)
			{
				output4[k][i] = output4AllElements + (k * x * y) + (i * y);
			}
		}

		for(k = 0; k < z; k++)
		{
			for(i = 0; i < x; i++)
			{
				for(j = 0; j < y; j++)
				{
					output4[k][i][j] = 0;
				}
			}
		}

		w = 48; // w = numero blocchi 3D
		x=3;
		y=3;
		z=48;
		//  Allocate  4D Array
		float *filter4AllElements = m2mb_os_malloc(x * y * z * w * sizeof(float));
		float ****filter4 = m2mb_os_malloc(w * sizeof(float ***));
		for(l = 0; l < w; l++)
		{
			filter4[l] = m2mb_os_malloc(z * sizeof(float **));

			for(k = 0; k < z; k++)
			{
				filter4[l][k] = m2mb_os_malloc(x * sizeof(float *));

				for(i = 0; i < x; i++)
				{
					filter4[l][k][i] = filter4AllElements + (l * z * x * y) + (k * x * y) + (i * y);
				}
			}
		}
		m2mb_fs_fread(filter4AllElements, sizeof (CHAR), 82944, weights);
		//1232 msec ----------------839msec-----------------------------------------------------------------------------



		channel = 48;
		convolution(output3, output4, filter4, 13, 7, 48, channel, bias, 84);



		//1394 msec ----------------------930msec-------------------------------------------------------------------
		//  Deallocate 3D array
		m2mb_os_free(output3AllElements);
		for(k = 0; k < z; k++)
		{
			m2mb_os_free(output3[k]);
		}
		m2mb_os_free (output3);

		w=48;
		z=48;
		//  Deallocate 3D array

		m2mb_os_free(filter4AllElements);

		for(l = 0; l < w; l++)
		{
			for(k = 0; k < z; k++)
			{
				m2mb_os_free(filter4[l][k]);
			}
			m2mb_os_free(filter4[l]);
		}
		m2mb_os_free (filter4);

		batchAndRelu(output4,13,7,48,tmean,tvar,offset,scale, 84);
		//maxPooling2(output4,13,7,84);
#endif
		//-.-.-.-..-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.--.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

#if 1
		x=13;
		y=7;
		z=48;
		float *output5AllElements = m2mb_os_malloc(x * y * z * sizeof(float));
		float ***output5 = m2mb_os_malloc(z * sizeof(float **));

		for(k = 0; k < z; k++)
		{
			output5[k] = m2mb_os_malloc(x * sizeof(float *));

			for(i = 0; i < x; i++)
			{
				output5[k][i] = output5AllElements + (k * x * y) + (i * y);
			}
		}

		for(k = 0; k < z; k++)
		{
			for(i = 0; i < x; i++)
			{
				for(j = 0; j < y; j++)
				{
					output5[k][i][j] = 0;
				}
			}
		}
		//M2M_LOG_INFO("out5:\r\n");
		w = 48; // w = numero blocchi 3D
		x=3;
		y=3;
		z=48;
		//  Allocate  4D Array
		float *filter5AllElements = m2mb_os_malloc(x * y * z * w * sizeof(float));
		float ****filter5 = m2mb_os_malloc(w * sizeof(float ***));
		for(l = 0; l < w; l++)
		{
			filter5[l] = m2mb_os_malloc(z * sizeof(float **));

			for(k = 0; k < z; k++)
			{
				filter5[l][k] = m2mb_os_malloc(x * sizeof(float *));

				for(i = 0; i < x; i++)
				{
					filter5[l][k][i] = filter5AllElements + (l * z * x * y) + (k * x * y) + (i * y);
				}
			}
		}
		m2mb_fs_fread(filter5AllElements, sizeof (CHAR), 82944, weights);



		// 1396 --------------------933msec----------------------------------------------------------
		channel = 48;
		convolution(output4, output5, filter5, 13, 7, 48, channel, bias, 132);
		//1557 msec --------------1150>>1025msec--------------------------------------------------------------
		w=48;
		z=48;

		//  Deallocate 3D array
		m2mb_os_free(output4AllElements);
		for(k = 0; k < z; k++)
		{
			m2mb_os_free(output4[k]);
		}
		m2mb_os_free (output4);


		//  Deallocate 3D array

		m2mb_os_free(filter5AllElements);

		for(l = 0; l < w; l++)
		{
			for(k = 0; k < z; k++)
			{
				m2mb_os_free(filter5[l][k]);
			}
			m2mb_os_free(filter5[l]);
		}
		m2mb_os_free (filter5);

		batchAndRelu(output5,13,7,48,tmean,tvar,offset,scale, 132);
		maxPooling3(output5,13,7,48);

		float *filter6 = m2mb_os_malloc(12 * 336 * sizeof(float));
		m2mb_fs_fread(filter6, sizeof (CHAR), 16128, weights); //12*336*8
		m2mb_fs_fclose	(weights);
		fclayer(output5, filter6, 7, 48, bias, 180);
		softmax(output5);

#endif
		//-.-.-.-..-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.--.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.


		matrix_print ( output5);

		//  Deallocate 3D array
		m2mb_os_free(output5AllElements);
		for(k = 0; k < z; k++)
		{
			m2mb_os_free(output5[k]);
		}
		m2mb_os_free (output5);
		m2mb_os_free(filter6);
		m2mb_os_free(bias);
		m2mb_os_free(tmean);
		m2mb_os_free(tvar);
		m2mb_os_free(offset);
		m2mb_os_free(scale);
		//=================================================================

		m2mb_os_free(imageinput);

		azx_tasks_sendMessageToTask(taskID2, OK, 0, 0);
		M2M_LOG_INFO("Arrived at the END\r\n\r\n");


	}break;

	default :
		break;
	}
	/*
	 * libera la memoria occupata dalla variabile salvata all'indirizzo di memoria puntato da "array_spec"
	 */
	m2mb_os_free(array_spec);



	return 0;
}

