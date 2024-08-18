// CalcLib_FFT.c
#include "CalcLib_FFT.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "cmsis_gcc.h"
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

int findMaxIndexInRange_uint32(uint32_t *arr_input, int start, int end) {
  int maxIndex = start;
  for (int i = start + 1; i <= end; i++) {
    if (arr_input[i] > arr_input[maxIndex]) {
      maxIndex = i;
    }
  }
  return maxIndex;
}

int findMaxIndexInRange_f32(float *arr_input, int start, int end) {
  int maxIndex = start;
  for (int i = start + 1; i <= end; i++) {
    if (arr_input[i] > arr_input[maxIndex]) {
      maxIndex = i;
    }
  }
  return maxIndex;
}

int findMaxAbsIndexInRange_f32(float *arr_input, int start, int end) {
  int maxAbsIndex = start;
  for (int i = start + 1; i <= end; i++) {
    if (fabs(arr_input[i]) > fabs(arr_input[maxAbsIndex])) {
      maxAbsIndex = i;
    }
  }
  return maxAbsIndex;
}

void scale_float_array(float *array, int size, float scale_factor) {
  for (int i = 0; i < size; i++) {
    array[i] *= scale_factor;
  }
}

void remove_dc_and_apply_hanning_window(float adc_buffer[], int num_samples) {
  // 计算直流分量
  float dc_component = 0.0;
  for (int i = 0; i < num_samples; i++) {
    dc_component += adc_buffer[i];
  }
  dc_component /= num_samples;
  // 从 adc_buffer 中减去直流分量
  for (int i = 0; i < num_samples; i++) {
    adc_buffer[i] -= dc_component;
  }
  // 应用汉宁窗函数
  for (int i = 0; i < num_samples; i++) {
    adc_buffer[i] *= 0.5 * (1 - cos(2 * PI * i / (num_samples - 1)));
  }
}

void fft_calc512_magnitude_q31(const q31_t *input_signal,
                               q31_t *output_magnitude) {
  q31_t fft_inputbuf[FFT_LENGTH * 2]; // FFT输入数组，包含实部和虚部
  // 生成FFT输入数组
  for (int i = 0; i < FFT_LENGTH; i++) {
    fft_inputbuf[2 * i] = input_signal[i]; // 实部
    fft_inputbuf[2 * i + 1] = 0;           // 虚部
  }
  // 执行FFT
  arm_cfft_q31(&arm_cfft_sR_q31_len512, fft_inputbuf, 0, 1);
  // 计算幅度，前 HALF_LENGTH 个是有效的
  arm_cmplx_mag_q31(fft_inputbuf, output_magnitude, FFT_HALF_LENGTH);
}

float fft_get_phase(const q31_t *input_signal) {
  q31_t fft_inputbuf[FFT_LENGTH * 2]; // FFT输入数组，包含实部和虚部
  // 生成FFT输入数组
  for (int i = 0; i < FFT_LENGTH; i++) {
    fft_inputbuf[2 * i] = input_signal[i]; // 实部
    fft_inputbuf[2 * i + 1] = 0;           // 虚部
  }
  // 执行FFT
  arm_cfft_q31(&arm_cfft_sR_q31_len512, fft_inputbuf, 0, 1);
  // 假设基波在output_magnitude中是第10个
  int fundamental_index = 10;
  q31_t real_part = fft_inputbuf[2 * fundamental_index];
  q31_t imag_part = fft_inputbuf[2 * fundamental_index + 1];
  // 将定点数转换为浮点数并缩放
  float real_part_f = real_part / 2147483648.0f;
  float imag_part_f = imag_part / 2147483648.0f;
  // 计算基波相位
  float phase = atan2f(imag_part_f, real_part_f);
  // 将相位转换为角度（度数）
  phase = phase * (180.0f / PI);
  return phase;
}

float calcPOWER(const float *voltage, const float *current, int length,
                bool input_unit) {
  float power_sum = 0.0;
  // 根据输入单位确定转换因子
  float conversion_factor = input_unit ? 1.0 : 0.001;
  float voltage_in_volts, current_in_amps, power;
  for (int i = 0; i < length; i++) {
    voltage_in_volts = voltage[i] * conversion_factor;
    current_in_amps = current[i] * conversion_factor;
    power = voltage_in_volts * current_in_amps;
    power_sum += power;
  }
  return power_sum / length;
}

float calcRMS(const float *voltage, int length, bool input_unit) {
  float sum_of_squares = 0.0;
  // 根据输入单位确定转换因子
  float conversion_factor = input_unit ? 1.0 : 0.001;
  for (int i = 0; i < length; i++) {
    float voltage_in_volts = voltage[i] * conversion_factor;
    sum_of_squares += voltage_in_volts * voltage_in_volts;
  }
  return sqrt(sum_of_squares / length);
}

float calculateTHD(float fft_int[], int step) {
  int max = findMaxIndexInRange_f32(fft_int, 0, 255);
  float fundamental_amplitude =
      sqrtf(fft_int[max] * fft_int[max] + fft_int[max + 1] * fft_int[max + 1] +
            fft_int[max - 1] * fft_int[max - 1]);
  float harmonics_amplitude_sum_squared = 0;
  for (int i = 19; i <= step * 10 + 1; i += 10) {
    int index = findMaxIndexInRange_f32(fft_int, i, i + 2);
    harmonics_amplitude_sum_squared += fft_int[index - 1] * fft_int[index - 1] +
                                       fft_int[index] * fft_int[index] +
                                       fft_int[index + 1] * fft_int[index + 1];
  }
  harmonics_amplitude_sum_squared = sqrtf(harmonics_amplitude_sum_squared);
  float thd = 100 * (harmonics_amplitude_sum_squared) / (fundamental_amplitude);
  return thd;
}

void GetH3(float fft_int[], float thd_res, float H_out3[], float I_rms) {
  int max = findMaxIndexInRange_f32(fft_int, 0, 255);
  float fundamental_amplitude =
      sqrtf(fft_int[max] * fft_int[max] + fft_int[max + 1] * fft_int[max + 1] +
            fft_int[max - 1] * fft_int[max - 1]);
  int step = 5;
  float Harmonic_res[step];
  Harmonic_res[0] = fundamental_amplitude;
  for (int i = 19; i <= step * 10 + 1; i += 10) {
    int index = findMaxIndexInRange_f32(fft_int, i, i + 2);
    Harmonic_res[i / 10] = sqrtf(fft_int[index - 1] * fft_int[index - 1] +
                                 fft_int[index] * fft_int[index] +
                                 fft_int[index + 1] * fft_int[index + 1]);
  }
  thd_res /= 100;
  float factor = 1.0 / sqrt(1 + thd_res * thd_res);
  for (int i = 0; i < step; i++) {
    H_out3[i] = factor * Harmonic_res[i] / Harmonic_res[0];
    H_out3[i] *= I_rms;
  }
}

uint16_t findMax(uint16_t arr[], int size) {
  uint16_t max = 0;
  for (int i = 0; i < size; i++) {
    if (arr[i] > max) {
      max = arr[i];
    }
  }
  return max;
}

uint16_t findMin(uint16_t arr[], int size) {
  uint16_t min = UINT16_MAX;
  for (int i = 0; i < size; i++) {
    if (arr[i] < min) {
      min = arr[i];
    }
  }
  return min;
}

void formatted_print(const char *format, ...) {
  // 假设最大格式化字符串长度为 256
  char buffer[256];
  // 检查是否是分割线格式
  if (strcmp(format, "---") == 0) {
    int length = 100; // 分割线长度
    for (int i = 0; i < length; i++) {
      buffer[i] = '-';
    }
    buffer[length] = '\0';
    printf("%s\r\n", buffer);
    return;
  }
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  printf("%s", buffer);
}