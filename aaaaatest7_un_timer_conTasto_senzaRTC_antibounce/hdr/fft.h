/*
 * fft.h
 *
 *  Created on: 18 mar 2022
 *      Author: Kevin
 */

#ifndef HDR_FFT_H_
#define HDR_FFT_H_

typedef float complex cplx;

#define FIN_POS 257
#define FIN_FFT 512
#define NOVER 240

#define BIN_LEN 98
#define DATA_LEN 400

#define BARK_BANDS 50
#define NHOP 160


#include "m2mb_types.h"
#include "m2mb_rtc.h"



//dichiarazione delle funzioni del file fft.c

void int2cplx(INT16* param1,cplx* fft, int dim);
void _fft_p(cplx* buf, cplx* out, int n, int step);
void fft_p(cplx* buf, int n);
void hann_window_p(cplx* xin);
void bid_spec_print (float ***input, float *f, int width, float iminput);
void on_timer_action(void);
void on_timer2_action(void);
void general_printer(const INT32 arr_address, int n, type_switcher print_type);
void timer_stop(void);
void timer2_stop(void);
void timer_start(void);
void timer2_start(void);
void restart_w_new_param(void);
INT8 info_atPort_result(INT32 res);
float* bark_spec_array(cplx *fArr);
void matrix_print ( float ***input);
void filter_print ( float ****input);
void convolution(float ***input, float ***output, float ****filter, int height, int width, int depth, int FILTER_CHANNEL, float* bias, int indicek);
void setParam(INT16* p_address);
void batchAndRelu(float ***array3d, int x, int y, int z, float* mean, float* var, float* offset, float* scale, int indicek);
void maxPooling(float ***input, int height, int width, int depth);
void maxPooling2(float ***input, int height, int width, int depth);
void maxPooling3(float ***input, int height, int width, int depth);
void fclayer(float ***input, float *filter, int width, int depth, float* bias, int indicek);
void softmax(float ***input);
INT16* getParam(void);
void FallingTrigg_cb(UINT32 fd,  void *userdata );
void configRegister(INT32 fd, UINT8 regAddr, const char* regName, UINT8 *bytesToWrite, UINT32 byteslen, char* message);
int maxim_setup(void);
int send_process(const char* cmd, const char* cmd_str);
int startup_interface(void);
INT16 findVoiceStartSample( INT16 *audioData);

#endif /* HDR_FFT_H_ */
