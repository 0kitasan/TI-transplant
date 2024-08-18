// adc_setup.c
#include "adc_setup.h"
#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
uint16_t adc0_result[ADC_SAMPLE_SIZE];
volatile bool adc0_flag = false;

void setupDMA(DMA_Regs *dma, uint8_t channelNum, unsigned int srcAddr,
              unsigned int destAddr, unsigned int transferSize) {
  DL_DMA_setSrcAddr(dma, channelNum, (uint32_t)srcAddr);
  DL_DMA_setDestAddr(dma, channelNum, (uint32_t)destAddr);
  // 如果在syscfg配置过，就无需写这一句话
  // 如果配置的和函数内部写的不同的话，会被函数覆盖
  // DL_DMA_setTransferSize(dma, channelNum, transferSize);
  DL_DMA_enableChannel(dma, channelNum);
}

void ADC12_0_INST_IRQHandler(void) {
  DL_ADC12_getPendingInterrupt(ADC12_0_INST);
  // 好像不写下面这句也没事
  DL_ADC12_disableConversions(ADC12_0_INST);
  adc0_flag = true;
  printf("adc0 over\r\n");
}

void adc_get_sample(void) {
  setupDMA(DMA, DMA_CH0_CHAN_ID, DL_ADC12_getFIFOAddress(ADC12_0_INST),
           (unsigned int)adc0_result, ADC_SAMPLE_SIZE);
  DL_Timer_startCounter(TIMER_0_INST);
  DL_ADC12_enableConversions(ADC12_0_INST);
  while (adc0_flag == false) {
    __WFE();
  }
  DL_Timer_stopCounter(TIMER_0_INST);
  adc0_flag = false;
}

float adc_voltage_mv_trans(uint16_t adc_data) {
  static float v_ref = 3300.0;
  static float resolution = 4095.0;
  return (adc_data * v_ref) / resolution;
}

void scale_tim_freq_byarr(GPTIMER_Regs *timer, float scale_factor) {
  if (timer == NULL || scale_factor <= 0) {
    return; // 检查无效参数
  }
  // 获取当前 LOAD 值
  uint32_t current_load = timer->COUNTERREGS.LOAD + 1;
  // 计算新的 LOAD 值
  uint32_t new_load = (uint32_t)(current_load / scale_factor) - 1;
  // 设置新的 LOAD 值
  timer->COUNTERREGS.LOAD = new_load;
}

uint32_t cal_ADC_SampleRate(uint16_t square_freq, uint16_t *adc_sample_res,
                            uint16_t adc_sample_size) {
  // 该函数用于测试，输入一个固定频率的方波以计算ADC采样率
  // 单位与输入的square_freq保持一致
  uint16_t start_index = 10; // 从第10个样本开始以保证稳定
  uint16_t prev_sample = adc_sample_res[start_index];
  uint16_t first_fall_index = 0;
  uint16_t second_fall_index = 0;
  uint16_t threshold = 1650;
  bool first_fall_found = false;
  // 寻找第一个下降沿
  for (uint16_t i = start_index + 1; i < adc_sample_size; i++) {
    if (prev_sample >= threshold && adc_sample_res[i] < threshold) {
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