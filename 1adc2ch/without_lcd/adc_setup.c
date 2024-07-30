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

uint16_t adc_voltage_mv_trans(uint16_t adc_data) {
  static uint16_t v_ref = 3300;
  static uint16_t resolution = 4096;
  int tmp = adc_data * v_ref;
  return tmp / resolution;
}