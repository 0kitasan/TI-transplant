// CalcLib_FFT.h
#ifndef CALCLIB_FFT_H
#define CALCLIB_FFT_H

#include <stdint.h>

int findMaxIndexInRange(uint32_t *arr_input, int start, int end);
uint16_t findMax(uint16_t arr[], int size);
uint16_t findMin(uint16_t arr[], int size);
float calculateTHD(uint32_t fft_int[]);
float calculateTHD_v4(uint32_t fft_int[]);
uint32_t cal_ADC_SampleRate(uint16_t square_freq, uint16_t *adc_sample_res,
                            uint16_t adc_sample_size);
void floatToString(float num, char *str, int precision);
#endif // CALCLIB_FFT_H