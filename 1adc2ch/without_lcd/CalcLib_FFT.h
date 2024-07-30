// CalcLib_FFT.h
#ifndef CALCLIB_FFT_H
#define CALCLIB_FFT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

int findMaxIndexInRange(uint32_t *arr_input, int start, int end);
// int findMaxAbsIndexInRange(int32_t *arr_input, int start, int end);
void formatted_print(const char *format, ...);
void fft_calc1024_magnitude(const float *input_signal, float *output_magnitude);
void fft_calc1024_magnitude_test(void); // 仅用于测试
float calcRMS(const float *voltage, int length, bool input_unit);
float power_calc_test(const float *voltage, const float *current, int length,
                      bool input_unit);
uint16_t findMax(uint16_t arr[], int size);
uint16_t findMin(uint16_t arr[], int size);
float calculateTHD(uint32_t fft_int[]);
float calculateTHD_v4(uint32_t fft_int[]);
uint32_t cal_ADC_SampleRate(uint16_t square_freq, uint16_t *adc_sample_res,
                            uint16_t adc_sample_size);
void floatToString(float num, char *str, int precision);

#ifdef __cplusplus
}
#endif
#endif // CALCLIB_FFT_H