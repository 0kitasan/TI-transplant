#include "CalcLib_FFT.h"
#include "adc_setup.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "cmsis_gcc.h"
#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include <cstdint>
#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * Pin Assignments:
 *
 * PA27 - ADC0_CH0:
 * PA25 - ADC0_CH2:
 * PB09 - SPI-CLK:
 * PA18 - SPI-MOSI:
 * PA12 - RST:
 * PA31 - DC:
 * PB28 - CS:
 */

#define FFT_LENGTH ADC_SAMPLE_SIZE
#define FFT_HALF_LENGTH FFT_LENGTH / 2 + 1

// fft_calc1024_magnitude_test();

// 初始时应当以1MHz采样，TIM的Period应当为1us
uint32_t timer_cnt_reg;
uint32_t timer_div_reg;
int tim_max;

// ADC_SAMPLE_SIZE=1024：实际上是两个通道各采样512
// 可能会减去dc offset，并且计算RMS时使用float
// 因此使用float类型，不过可能导致内存爆炸的问题
float adc0_ch1_mv[ADC_SAMPLE_SIZE / 2];
float adc0_ch2_mv[ADC_SAMPLE_SIZE / 2];

float thd;
float thd_v4;

int main(void) {
  /*---------Initialization---------*/
  SYSCFG_DL_init();
  NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
  formatted_print("debug start!\r\n");
  timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
  formatted_print("timer_cnt_reg=%d\r\n", timer_cnt_reg);
  /*---------sampling---------*/
  __BKPT(0);
  adc_get_sample();
  uint16_t adc0_mv;
  for (int i = 0; i < ADC_SAMPLE_SIZE; i++) {
    // change to mV and decouple array
    adc0_mv = adc_voltage_mv_trans(adc0_result[i]);
    if (i % 2 == 0) {
      // 存储到通道 1 的数组
      adc0_ch1_mv[i / 2] = adc0_mv - 1650;
      formatted_print("%d:\t ADC0_CH1_RES=%d,\t", i / 2, adc0_mv);
    } else {
      // 存储到通道 2 的数组
      adc0_ch2_mv[i / 2] = adc0_mv - 1650;
      formatted_print("ADC0_CH2_RES=%d\r\n", adc0_mv);
    }
  }

  /*---------data process---------*/
  // 注意，上面减去了offset
  float v1_eff = calcRMS(adc0_ch1_mv, 500, 0);
  float v2_eff = calcRMS(adc0_ch2_mv, 500, 0);
  float power_res = power_calc_test(adc0_ch1_mv, adc0_ch1_mv, 500, 0);
  formatted_print("V_eff:\t%f\t%f\r\n", v1_eff, v2_eff);
  formatted_print("power_res=%f\r\n", power_res);
  __BKPT(0);
  /*---------@test:calc THD---------*/
  timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
  timer_div_reg = TIMA0->CLKDIV;
  for (int i = 0; i < ADC_SAMPLE_SIZE; i++) {
    adc0_result[i] = 0;
  }
  adc_get_sample();
  for (int i = 0; i < ADC_SAMPLE_SIZE; i++) {
    formatted_print("twice sample value=%d\r\n", adc0_result[i]);
  }
  __BKPT(0);
  while (1) {
  }
}

// 重定向fputc函数
int fputc(int ch, FILE *f) {
  DL_UART_transmitDataBlocking(UART_0_INST, ch);
  return (ch);
}
