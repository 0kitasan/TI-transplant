// CalcLib_FFT.h
#ifndef CALCLIB_FFT_H
#define CALCLIB_FFT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_const_structs.h"
#include "arm_math.h"
#include <stdbool.h>
#include <stdint.h>

#define FFT_LENGTH 512
#define FFT_HALF_LENGTH FFT_LENGTH / 2 + 1

int findMaxIndexInRange_uint32(uint32_t *arr_input, int start, int end);
int findMaxIndexInRange_f32(float *arr_input, int start, int end);
int findMaxAbsIndexInRange_f32(float *arr_input, int start, int end);
void scale_float_array(float *array, int size, float scale_factor);
void remove_dc_and_apply_hanning_window(float adc_buffer[], int num_samples);
// 通用格式化打印函数
void formatted_print(const char *format, ...);
void fft_calc512_magnitude_q31(const q31_t *input_signal,
                               q31_t *output_magnitude);
float fft_get_phase(const q31_t *input_signal);
float calcRMS(const float *voltage, int length, bool input_unit);
float calcPOWER(const float *voltage, const float *current, int length,
                bool input_unit);
uint16_t findMax(uint16_t arr[], int size);
uint16_t findMin(uint16_t arr[], int size);
float calculateTHD(float fft_int[], int step);
void GetH3(float fft_int[], float thd_res, float H_out3[], float V_rms);
#ifdef __cplusplus
}
#endif
#endif // CALCLIB_FFT_H