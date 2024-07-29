// CalcLib_FFT.c
#include "CalcLib_FFT.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

int findMaxIndexInRange(uint32_t *arr_input, int start, int end) {
  int maxIndex = start;
  for (int i = start + 1; i <= end; i++) {
    if (arr_input[i] > arr_input[maxIndex]) {
      maxIndex = i;
    }
  }
  printf("test\r\n");
  return maxIndex;
}

float calculateTHD(uint32_t fft_int[]) {
  int max = findMaxIndexInRange(fft_int, 0, 511);
  float fundamental_amplitude = fft_int[max];
  // float harmonics_amplitude_sum_squared = 0.0f;
  float harmonics_amplitude_sum_squared;
  int second = findMaxIndexInRange(fft_int, 150, 250);
  int third = findMaxIndexInRange(fft_int, 250, 350);
  int fourth = findMaxIndexInRange(fft_int, 350, 450);
  int fifth = findMaxIndexInRange(fft_int, 450, 511);
  harmonics_amplitude_sum_squared =
      fft_int[second] * fft_int[second] + fft_int[third] * fft_int[third] +
      fft_int[fourth] * fft_int[fourth] + fft_int[fifth] * fft_int[fifth];
  harmonics_amplitude_sum_squared = sqrtf(harmonics_amplitude_sum_squared);
  float thd_res = harmonics_amplitude_sum_squared / fundamental_amplitude;
  float thd_percent = 100 * thd_res;
  return thd_percent;
}

float calculateTHD_v4(uint32_t fft_int[]) {
  // 找到基频
  int max = findMaxIndexInRange(fft_int, 0, 511);
  int fft_maxf_Am_resize;
  int fft_maxf_left_Am_resize;
  int fft_maxf_right_Am_resize;
  float fundamental_amplitude;
  // 计算基频的幅度
  if (fft_maxf_Am_resize > 10000 || fft_maxf_left_Am_resize > 10000 ||
      fft_maxf_right_Am_resize > 10000) {
    fft_maxf_Am_resize = fft_int[max] / 10000;
    fft_maxf_left_Am_resize = fft_int[max - 1] / 10000;
    fft_maxf_right_Am_resize = fft_int[max + 1] / 10000;
    fundamental_amplitude =
        10000 * sqrtf(fft_maxf_Am_resize * fft_maxf_Am_resize +
                      fft_maxf_left_Am_resize * fft_maxf_left_Am_resize +
                      fft_maxf_right_Am_resize * fft_maxf_right_Am_resize);
  } else {
    fft_maxf_Am_resize = fft_int[max];
    fft_maxf_left_Am_resize = fft_int[max - 1];
    fft_maxf_right_Am_resize = fft_int[max + 1];
    fundamental_amplitude =
        sqrtf(fft_maxf_Am_resize * fft_maxf_Am_resize +
              fft_maxf_left_Am_resize * fft_maxf_left_Am_resize +
              fft_maxf_right_Am_resize * fft_maxf_right_Am_resize);
  }
  float harmonics_amplitude_sum_squared = 0.0f;
  // 定义谐波的范围
  int harmonic_ranges[4][2] = {{150, 250}, {250, 350}, {350, 450}, {450, 511}};
  // 计算各次谐波的幅度
  for (int i = 0; i < 4; ++i) {
    int harmonic = findMaxIndexInRange(fft_int, harmonic_ranges[i][0],
                                       harmonic_ranges[i][1]);
    harmonics_amplitude_sum_squared +=
        fft_int[harmonic - 1] * fft_int[harmonic - 1] +
        fft_int[harmonic] * fft_int[harmonic] +
        fft_int[harmonic + 1] * fft_int[harmonic + 1];
  }
  harmonics_amplitude_sum_squared = sqrtf(harmonics_amplitude_sum_squared);
  float thd_res = harmonics_amplitude_sum_squared / fundamental_amplitude;
  float thd_percent = 100 * thd_res;
  return thd_percent;
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

uint32_t cal_ADC_SampleRate(uint16_t square_freq, uint16_t *adc_sample_res,
                            uint16_t adc_sample_size) {
  // 该函数用于测试，输入一个固定频率的方波以计算ADC采样率
  // 单位与输入的square_freq保持一致
  uint16_t start_index = 10; // 从第10个样本开始以保证稳定
  uint16_t prev_sample = adc_sample_res[start_index];
  uint16_t first_fall_index = 0;
  uint16_t second_fall_index = 0;
  bool first_fall_found = false;
  // 寻找第一个下降沿
  for (uint16_t i = start_index + 1; i < adc_sample_size; i++) {
    if (prev_sample >= 2048 && adc_sample_res[i] < 2048) {
      if (!first_fall_found) {
        first_fall_index = i;
        first_fall_found = true;
      } else {
        second_fall_index = i;
        break;
      }
    }
    prev_sample = adc_sample_res[i];
  }
  // 计算从第一个下降沿到第二个下降沿的样本数
  uint16_t period_samples = second_fall_index - first_fall_index;
  // 计算采样率：采样率 = 方波频率 * 周期样本数
  uint32_t sample_rate = period_samples * square_freq;

  return sample_rate;
}

void floatToString(float num, char *str, int precision) {
  // 确定符号
  int isNegative = 0;
  if (num < 0) {
    isNegative = 1;
    num = -num;
  }

  // 提取整数部分
  int integerPart = (int)num;
  num -= integerPart;

  // 提取小数部分
  int factor = 1;
  for (int i = 0; i < precision; ++i) {
    num *= 10;
    factor *= 10;
  }
  int decimalPart = (int)(num + 0.5); // 四舍五入

  // 将整数部分转换为字符串
  char buffer[20];
  int index = 0;
  do {
    buffer[index++] = (integerPart % 10) + '0';
    integerPart /= 10;
  } while (integerPart > 0);

  if (isNegative) {
    buffer[index++] = '-';
  }
  buffer[index] = '\0';

  // 反转整数部分字符串
  for (int i = 0; i < index / 2; ++i) {
    char temp = buffer[i];
    buffer[i] = buffer[index - i - 1];
    buffer[index - i - 1] = temp;
  }

  // 将整数部分复制到结果字符串
  int strIndex = 0;
  for (int i = 0; buffer[i] != '\0'; ++i) {
    str[strIndex++] = buffer[i];
  }

  // 添加小数点
  str[strIndex++] = '.';

  // 将小数部分转换为字符串
  index = 0;
  do {
    buffer[index++] = (decimalPart % 10) + '0';
    decimalPart /= 10;
  } while (decimalPart > 0);

  // 补齐小数部分不足的位数
  while (index < precision) {
    buffer[index++] = '0';
  }
  buffer[index] = '\0';

  // 反转小数部分字符串
  for (int i = 0; i < index / 2; ++i) {
    char temp = buffer[i];
    buffer[i] = buffer[index - i - 1];
    buffer[index - i - 1] = temp;
  }

  // 将小数部分复制到结果字符串
  for (int i = 0; buffer[i] != '\0'; ++i) {
    str[strIndex++] = buffer[i];
  }
  str[strIndex] = '\0';
}