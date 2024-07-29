#include "CalcLib_FFT.h"
#include "DrivLib_TM1638.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "cmsis_gcc.h"
#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <string.h>
/*
 * Pin Assignments:
 *
 * PA27 - ADC0_CH0:  
 * PA25 - ADC0_CH2:
 */

#define FFT_LENGTH 1024
#define ADC_SAMPLE_SIZE 1024
#define samples 1024
uint16_t adc0_result[ADC_SAMPLE_SIZE];
uint16_t adc1_result[ADC_SAMPLE_SIZE];
char adc0_charbuf[20];
char adc1_charbuf[20];
volatile bool adc0_flag = false;
/*
750-1250
250-1750
*/
uint32_t adc_sample_rate = 0;

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

void adc_get_sample() {
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

float thd;
float thd_v4;
int max_index;

// 初始时应当以1MHz采样，TIM的Period应当为1us
uint32_t timer_cnt_reg;
uint32_t timer_div_reg;
int tim_max;
char ch_buffer[30];

int main(void) {
  /*---------Initialization---------*/
  SYSCFG_DL_init();
  NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
  printf("debug start!\r\n");
  timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
  sprintf(ch_buffer, "%d", timer_cnt_reg);
  printf("timer_cnt_reg=%s\r\n", ch_buffer);
  __BKPT(0);
  adc_get_sample();
  uint16_t adc0_mv;
  for (int i = 0; i < ADC_SAMPLE_SIZE; i++) {
    adc0_mv = adc_voltage_mv_trans(adc0_result[i]);
    sprintf(adc0_charbuf, "%d", adc0_mv);
    if (i % 2 == 0) {
      printf("%d:\t ADC0_CH1_RES=%s,\t", i / 2, adc0_charbuf);
    } else {
      printf("ADC0_CH2_RES=%s\r\n", adc0_charbuf);
    }
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

// 重定向fputs函数
int fputs(const char *restrict s, FILE *restrict stream) {
  uint16_t i, len;
  len = strlen(s);
  for (i = 0; i < len; i++) {
    DL_UART_transmitDataBlocking(UART_0_INST, s[i]);
  }
  return len;
}

// 重定向puts函数
int puts(const char *_ptr) {
  int count = fputs(_ptr, stdout);
  count += fputs("\n", stdout);
  return count;
}