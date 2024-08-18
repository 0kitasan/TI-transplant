#include "CalcLib_FFT.h"
#include "OLED/bmp.h"
#include "OLED/oled.h"
#include "adc_setup.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "cmsis_gcc.h"
#include "queue.h"
#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#pragma clang optimize on
/*
 * Pin Assignments:
 *
 * PA27 - ADC0_CH0:
 * PA25 - ADC0_CH2:
 * PB16 - OLDE-CLK:
 * PB00 - OLED-SDA:
 * PB21 - Button:
 */

#define V_FACTOR 423
#define I_FACTOR 5100.00
// 初始时应当以1MHz采样，TIM的Period应当为1us
uint32_t timer_cnt_reg;
uint32_t timer_div_reg;
int tim_max;

// ADC_SAMPLE_SIZE=1024：实际上是两个通道各采样512
// 可能会减去dc offset，并且计算RMS时使用float
// 因此使用float类型，不过可能导致内存爆炸的问题
float adc0_mv = 0;
float adc0_ch1_mv[ADC_SAMPLE_SIZE / 2];
float adc0_ch2_mv[ADC_SAMPLE_SIZE / 2];
q31_t adc0_ch1_fft_input_q31[FFT_LENGTH];
q31_t adc0_ch2_fft_input_q31[FFT_LENGTH];
q31_t adc0_ch1_fft_output_q31[FFT_HALF_LENGTH];
q31_t adc0_ch2_fft_output_q31[FFT_HALF_LENGTH];
float32_t adc0_ch1_fft_res_f32[FFT_HALF_LENGTH];
float32_t adc0_ch2_fft_res_f32[FFT_HALF_LENGTH];
float v1_eff, v2_eff;
float power_res;
float power_q;
float phase1, phase2;
float phase_diff;
float phase_diff_format;
float power_q_by_phase;
float thd5;
float thd13;
float ch1_offset = 0;
float ch2_offset = 0;
float fitted_current = 0;
float fitted_voltage = 0;
float Harmonic3[5];
volatile int disp_mode = 0;

float calc_average(float *array, int size) {
  float sum = 0.0f;
  for (int i = 0; i < size; i++) {
    sum += array[i];
  }
  return sum / size;
}

float calc_power_fitted(const float *voltage, const float *current, int length,
                        bool input_unit) {
  float power_sum = 0.0;
  // 根据输入单位确定转换因子
  float conversion_factor = input_unit ? 1.0 : 0.001;
  float voltage_in_volts, current_in_amps, power;
  for (int i = 0; i < length; i++) {
    voltage_in_volts = V_FACTOR * voltage[i] * conversion_factor;
    current_in_amps = I_FACTOR * current[i] * conversion_factor / 1000.0;
    power = voltage_in_volts * current_in_amps;
    power_sum += power;
  }
  return power_sum / length;
}

// 移动平均滤波函数
void moving_average_filter(const float *input, float *output, int size,
                           int window_size) {
  float sum = 0.0f;
  for (int i = 0; i < window_size; i++) {
    sum += input[i];
  }
  output[0] = sum / window_size;

  for (int i = window_size; i < size; i++) {
    sum += input[i] - input[i - window_size];
    output[i - window_size + 1] = sum / window_size;
  }
}

void wave_filter_ave(float *wave, int size) {
  float sum = 0.0f;
  for (int i = 1; i < size - 1; i++) {
    bool condition1 = (wave[i - 1] > wave[i]) && (wave[i] < wave[i - 1]);
    bool condition2 = (wave[i - 1] < wave[i]) && (wave[i] > wave[i - 1]);
    if (condition1 || condition2) {
      wave[i] = (wave[i - 1] + wave[i + 1]) / 2;
    }
  }
}

float phase_format(float phase) {
  while (phase >= 180) {
    phase -= 180;
  }
  return phase;
}

/*
fitted_voltage
fitted_current
power_q_by_phase
thd13
*/
Queue Q_THD;
Queue Q_PF;
Queue Q_FittedCurr;

/*
 * OLED_ShowString(x,y,str,size)
 * x: Horizontal coordinate, increasing from left(0) to right
 * y: Vertical coordinate, increasing from top(0) to bottom
 * size: default size = 16
 */

void oled_disp_pow_test(void) {
  OLED_ShowString(0, 0, (uint8_t *)"Veff=19", 16);
  OLED_ShowString(0, 2, (uint8_t *)"Ieff=19", 16);
  OLED_ShowString(0, 4, (uint8_t *)"Power=81", 16);
  OLED_ShowString(0, 6, (uint8_t *)"Q=0.8", 16);
}

void oled_disp_thd_test(void) {
  OLED_ShowString(0, 0, (uint8_t *)"THD=3.0%%", 16);
  OLED_ShowString(0, 2, (uint8_t *)"H1=11", 16);
  OLED_ShowString(0, 4, (uint8_t *)"H2=45", 16);
  OLED_ShowString(0, 6, (uint8_t *)"H3=14", 16);
}

void oled_disp_pow(float Veff, float Ieff, float PF) {
  char ch_buf[30];
  // 校准后电压RMS
  sprintf(ch_buf, "Veff=%.3fV", Veff);
  OLED_ShowString(0, 0, (uint8_t *)ch_buf, 16);
  // 校准后电流RMS
  sprintf(ch_buf, "Ieff=%.3fmA", Ieff);
  OLED_ShowString(0, 2, (uint8_t *)ch_buf, 16);
  // 有功功率(=UI平均值=Ueff*Ieff*cos)
  sprintf(ch_buf, "P=%.3fW", Veff * Ieff * PF / 1000.0);
  OLED_ShowString(0, 4, (uint8_t *)ch_buf, 16);
  // 功率因数
  sprintf(ch_buf, "PF=%.3f", PF);
  OLED_ShowString(0, 6, (uint8_t *)ch_buf, 16);
}

void oled_disp_pow_half(float Veff, float Ieff, float PF) {
  char ch_buf[30];
  Ieff /= 2;
  // 校准后电压RMS
  sprintf(ch_buf, "Veff=%.3fV", Veff);
  OLED_ShowString(0, 0, (uint8_t *)ch_buf, 16);
  // 校准后电流RMS
  sprintf(ch_buf, "Ieff=%.3fmA", Ieff);
  OLED_ShowString(0, 2, (uint8_t *)ch_buf, 16);
  // 有功功率(=UI平均值=Ueff*Ieff*cos)
  sprintf(ch_buf, "P=%.3fW  half!", Veff * Ieff * PF / 1000.0);
  OLED_ShowString(0, 4, (uint8_t *)ch_buf, 16);
  // 功率因数
  sprintf(ch_buf, "PF=%.3f  half!", PF);
  OLED_ShowString(0, 6, (uint8_t *)ch_buf, 16);
}

void oled_disp_thd(float THD, float H3[]) {
  char ch_buf[30];
  sprintf(ch_buf, "THD=%.3f", THD);
  OLED_ShowString(0, 0, (uint8_t *)ch_buf, 16);
  sprintf(ch_buf, "H1=%.3f", H3[0]);
  OLED_ShowString(0, 2, (uint8_t *)ch_buf, 16);
  sprintf(ch_buf, "H3=%.3f", H3[2]);
  OLED_ShowString(0, 4, (uint8_t *)ch_buf, 16);
  sprintf(ch_buf, "H5=%.3f", H3[4]);
  OLED_ShowString(0, 6, (uint8_t *)ch_buf, 16);
}

volatile int cnt = 0;

void GROUP1_IRQHandler(void) {
  cnt++;
  disp_mode = cnt % 4;
}

void calc_ALL(void) {
  adc_get_sample();
  for (int i = 0; i < ADC_SAMPLE_SIZE; i++) {
    // change to mV and decouple array
    adc0_mv = adc_voltage_mv_trans(adc0_result[i]);
    if (i % 2 == 0) {
      // 存储到通道 1 的数组
      adc0_ch1_mv[i / 2] = adc0_mv;
    } else {
      // 存储到通道 2 的数组
      adc0_ch2_mv[i / 2] = adc0_mv;
    }
  }
  //__BKPT(0);
  /*---------Calculate RMS and POWER---------*/
  int cal_num = 512;
  //__BKPT(0);
  ch1_offset = calc_average(adc0_ch1_mv, cal_num);
  ch2_offset = calc_average(adc0_ch2_mv, cal_num);
  for (int i = 0; i < FFT_LENGTH; i++) {
    adc0_ch1_mv[i] -= ch1_offset;
    adc0_ch2_mv[i] -= ch2_offset;
  }
  v1_eff = calcRMS(adc0_ch1_mv, cal_num, 0);
  v2_eff = calcRMS(adc0_ch2_mv, cal_num, 0);
  power_res = calcPOWER(adc0_ch1_mv, adc0_ch2_mv, cal_num, 0);
  power_q = power_res / (v1_eff * v2_eff);
  fitted_current = I_FACTOR * v1_eff;
  fitted_voltage = V_FACTOR * v2_eff;

  /*---------calculate phase for POWER_Q---------*/
  int max_idx1 = findMaxAbsIndexInRange_f32(adc0_ch1_mv, 0, FFT_LENGTH);
  float ch1_scale_factor = 1.0 / adc0_ch1_mv[max_idx1];
  scale_float_array(adc0_ch1_mv, FFT_LENGTH, ch1_scale_factor);
  adc0_ch1_mv[0] = 0;
  arm_float_to_q31(adc0_ch1_mv, adc0_ch1_fft_input_q31, FFT_LENGTH);
  phase1 = fft_get_phase(adc0_ch1_fft_input_q31);
  int max_idx2 = findMaxAbsIndexInRange_f32(adc0_ch2_mv, 0, FFT_LENGTH);
  float ch2_scale_factor = 1.0 / adc0_ch1_mv[max_idx2];
  scale_float_array(adc0_ch2_mv, FFT_LENGTH, ch2_scale_factor);
  adc0_ch2_mv[0] = 0;
  arm_float_to_q31(adc0_ch2_mv, adc0_ch2_fft_input_q31, FFT_LENGTH);
  phase2 = fft_get_phase(adc0_ch2_fft_input_q31);
  phase_diff = phase2 - phase1;
  phase_diff_format = 180 - phase_format(phase_diff + 720) - 1.3;
  power_q_by_phase = cos(phase_diff_format * PI / 180);

  /*---------do FFT to calculate THD---------*/
  formatted_print("---");
  remove_dc_and_apply_hanning_window(adc0_ch1_mv, FFT_LENGTH);
  remove_dc_and_apply_hanning_window(adc0_ch2_mv, FFT_LENGTH);

  fft_calc512_magnitude_q31(adc0_ch1_fft_input_q31, adc0_ch1_fft_output_q31);
  arm_q31_to_float(adc0_ch1_fft_output_q31, adc0_ch1_fft_res_f32,
                   FFT_HALF_LENGTH);
  for (int i = 0; i < 128; i++) {
    formatted_print("%d\tFFT_Q31=%.7f\r\n", i, adc0_ch1_fft_res_f32[i]);
  }
  thd13 = calculateTHD(adc0_ch1_fft_res_f32, 13);
  formatted_print("THD_step13=%.5f\r\n", thd13);
  GetH3(adc0_ch1_fft_res_f32, thd13, Harmonic3, fitted_current);
}

int main(void) {
  /*---------Initialization---------*/
  SYSCFG_DL_init();
  NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
  NVIC_EnableIRQ(GPIO_IN_INT_IRQN);
  OLED_Init();
  formatted_print("debug start!\r\n");
  timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
  timer_div_reg = TIMA0->CLKDIV;
  formatted_print("timer_cnt_reg=%d\r\n", timer_cnt_reg);
  formatted_print("timer_div_reg=%d\r\n", timer_div_reg);
  initQueue(&Q_THD);
  initQueue(&Q_PF);
  initQueue(&Q_FittedCurr);
  /*---------set sample rate and get sample---------*/
  scale_tim_freq_byarr(TIMA0, 0.005000 * 2 * 0.512);
  timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
  formatted_print("timer_cnt_reg=%d\r\n", timer_cnt_reg);

  //   adc_get_sample();
  //   for (int i = 0; i < ADC_SAMPLE_SIZE; i++) {
  //     // change to mV and decouple array
  //     adc0_mv = adc_voltage_mv_trans(adc0_result[i]);
  //     if (i % 2 == 0) {
  //       // 存储到通道 1 的数组
  //       adc0_ch1_mv[i / 2] = adc0_mv;
  //     } else {
  //       // 存储到通道 2 的数组
  //       adc0_ch2_mv[i / 2] = adc0_mv;
  //     }
  //   }
  //   //__BKPT(0);
  //   /*---------Calculate RMS and POWER---------*/
  //   int cal_num = 512;
  //   //__BKPT(0);
  //   ch1_offset = calc_average(adc0_ch1_mv, cal_num);
  //   ch2_offset = calc_average(adc0_ch2_mv, cal_num);
  //   for (int i = 0; i < FFT_LENGTH; i++) {
  //     adc0_ch1_mv[i] -= ch1_offset;
  //     adc0_ch2_mv[i] -= ch2_offset;
  //   }
  //   v1_eff = calcRMS(adc0_ch1_mv, cal_num, 0);
  //   v2_eff = calcRMS(adc0_ch2_mv, cal_num, 0);
  //   power_res = calcPOWER(adc0_ch1_mv, adc0_ch2_mv, cal_num, 0);
  //   power_q = power_res / (v1_eff * v2_eff);
  //   fitted_current = I_FACTOR * v1_eff;
  //   fitted_voltage = V_FACTOR * v2_eff;

  //   /*---------calculate phase for POWER_Q---------*/
  //   int max_idx1 = findMaxAbsIndexInRange_f32(adc0_ch1_mv, 0, FFT_LENGTH);
  //   float ch1_scale_factor = 1.0 / adc0_ch1_mv[max_idx1];
  //   scale_float_array(adc0_ch1_mv, FFT_LENGTH, ch1_scale_factor);
  //   adc0_ch1_mv[0] = 0;
  //   arm_float_to_q31(adc0_ch1_mv, adc0_ch1_fft_input_q31, FFT_LENGTH);
  //   phase1 = fft_get_phase(adc0_ch1_fft_input_q31);
  //   int max_idx2 = findMaxAbsIndexInRange_f32(adc0_ch2_mv, 0, FFT_LENGTH);
  //   float ch2_scale_factor = 1.0 / adc0_ch1_mv[max_idx2];
  //   scale_float_array(adc0_ch2_mv, FFT_LENGTH, ch2_scale_factor);
  //   adc0_ch2_mv[0] = 0;
  //   arm_float_to_q31(adc0_ch2_mv, adc0_ch2_fft_input_q31, FFT_LENGTH);
  //   phase2 = fft_get_phase(adc0_ch2_fft_input_q31);
  //   phase_diff = phase2 - phase1;
  //   phase_diff_format = 180 - phase_format(phase_diff + 720) - 1.3;
  //   power_q_by_phase = cos(phase_diff_format * PI / 180);
  //   formatted_print("Power Q_by_phi:\t%.7f\r\n", power_q_by_phase);
  //   formatted_print("UI_ave_Q_by_phi:\t%.7f\r\n",
  //                   power_q_by_phase * fitted_voltage * fitted_current /
  //                   1000);

  //   //__BKPT(0);

  //   /*---------do FFT to calculate THD---------*/
  //   formatted_print("---");
  //   remove_dc_and_apply_hanning_window(adc0_ch1_mv, FFT_LENGTH);
  //   remove_dc_and_apply_hanning_window(adc0_ch2_mv, FFT_LENGTH);

  //   fft_calc512_magnitude_q31(adc0_ch1_fft_input_q31,
  //   adc0_ch1_fft_output_q31); arm_q31_to_float(adc0_ch1_fft_output_q31,
  //   adc0_ch1_fft_res_f32,
  //                    FFT_HALF_LENGTH);
  //   for (int i = 0; i < 128; i++) {
  //     formatted_print("%d\tFFT_Q31=%.7f\r\n", i, adc0_ch1_fft_res_f32[i]);
  //   }
  //   thd13 = calculateTHD(adc0_ch1_fft_res_f32, 13);
  //   formatted_print("THD_step13=%.5f\r\n", thd13);
  //   GetH3(adc0_ch1_fft_res_f32, thd13, Harmonic3, fitted_current);
  while (1) {
    calc_ALL();
    enqueue(&Q_THD, thd13);
    enqueue(&Q_PF, power_q_by_phase);
    enqueue(&Q_FittedCurr, fitted_current);
    switch (disp_mode) {
    case 0:
      OLED_Clear();
      oled_disp_pow(fitted_voltage, getAverage(&Q_FittedCurr),
                    getAverage(&Q_PF));
      break;
    case 1:
      OLED_Clear();
      oled_disp_thd(getAverage(&Q_THD), Harmonic3);
      break;
    case 2:
      OLED_Clear();
      oled_disp_pow_half(fitted_voltage, getAverage(&Q_FittedCurr),
                         getAverage(&Q_PF));
      break;
    case 3:
      OLED_Clear();
      break;
    }
    delay_ms(1000);
  }
}

// 重定向fputc函数
int fputc(int ch, FILE *f) {
  DL_UART_transmitDataBlocking(UART_0_INST, ch);
  return (ch);
}
#pragma clang optimize off