// CalcLib_FFT.c
#include "CalcLib_FFT.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "cmsis_gcc.h"
#include <math.h>
#include <stdarg.h>
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

// int findMaxAbsIndexInRange(int32_t *arr_input, int start, int end) {
//   int maxIndex = start;
//   int32_t maxAbsValue = abs(arr_input[start]);

//   for (int i = start + 1; i <= end; i++) {
//     int32_t currentAbsValue = abs(arr_input[i]);
//     if (currentAbsValue > maxAbsValue) {
//       maxAbsValue = currentAbsValue;
//       maxIndex = i;
//     }
//   }
//   return maxIndex;
// }

// 通用格式化打印函数
void formatted_print(const char *format, ...) {
  char buffer[256]; // 假设最大格式化字符串长度为 256
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  printf("%s", buffer);
}

void fft_calc1024_magnitude(const float *input_signal,
                            float *output_magnitude) {
  // 我们只关心频率的幅度
  static const int FFT_LENGTH = 1024; // 定义FFT的长度为静态变量
  // 有效频率分量数量，第0个值是直流分量，所以要+1
  static const int HALF_LENGTH = FFT_LENGTH / 2 + 1;
  float fft_inputbuf[FFT_LENGTH * 2]; // FFT输入数组，包含实部和虚部
  // 生成FFT输入数组
  for (int i = 0; i < FFT_LENGTH; i++) {
    fft_inputbuf[2 * i] = input_signal[i]; // 实部
    fft_inputbuf[2 * i + 1] = 0;           // 虚部
  }
  // 执行FFT
  arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_inputbuf, 0, 1);
  // 计算幅度，前 HALF_LENGTH 个是有效的
  arm_cmplx_mag_f32(fft_inputbuf, output_magnitude, HALF_LENGTH);
}

void fft_calc1024_magnitude_test(void) {
  static const int FFT_LENGTH = 1024;
  static const int FFT_HALF_LENGTH = FFT_LENGTH / 2 + 1;
  static char ch_buffer[50];
  float fft_inputbuf_real[FFT_LENGTH];       // FFT 输入数组
  float fft_outputbuf_real[FFT_HALF_LENGTH]; // FFT 输出数组（频域）

  // 生成信号序列
  for (int i = 0; i < FFT_LENGTH; i++) {
    fft_inputbuf_real[i] = 100 + 10 * arm_sin_f32(2 * PI * i * 1 / FFT_LENGTH) +
                           20 * arm_sin_f32(2 * PI * i * 50 / FFT_LENGTH) +
                           30 * arm_cos_f32(2 * PI * i * 300 / FFT_LENGTH);
  }
  fft_calc1024_magnitude(fft_inputbuf_real, fft_outputbuf_real);
  for (int i = 0; i < FFT_HALF_LENGTH; i++) {
    sprintf(ch_buffer, "cfft_out %d=%f\r\n", i, fft_outputbuf_real[i]);
    printf("%s", ch_buffer);
  }
}

float power_calc_test(const float *voltage, const float *current, int length,
                      bool input_unit) {
  float power_sum = 0.0;
  // 根据输入单位确定转换因子
  float conversion_factor = input_unit ? 1.0 : 0.001;
  for (int i = 0; i < length; i++) {
    float voltage_in_volts = voltage[i] * conversion_factor;
    float current_in_amps = current[i] * conversion_factor;
    float power = voltage_in_volts * current_in_amps;
    power_sum += power;
  }
  return power_sum / length;
}

float calcRMS(const float *voltage, int length, bool input_unit) {
  float sum_of_squares = 0.0;
  // 根据输入单位确定转换因子
  float conversion_factor = input_unit ? 1.0 : 0.001;
  // char test_buf[20];
  for (int i = 0; i < length; i++) {
    float voltage_in_volts = voltage[i] * conversion_factor;
    // sprintf(test_buf, "%f", voltage[i]);
    // printf("vol=%s\r\n", test_buf);
    sum_of_squares += voltage_in_volts * voltage_in_volts;
    // sprintf(test_buf, "%f", sum_of_squares);
    // printf("sum=%s\r\n", test_buf);
  }
  return sqrt(sum_of_squares / length);
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