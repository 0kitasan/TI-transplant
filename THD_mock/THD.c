#include "CalcLib_FFT.h"
#include "DrivLib_TM1638.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>

/*
 * Pin Assignments:
 *
 * PA15 - DAC12:    12-bit Digital-to-Analog Converter, ref=2.5V.
 * PA22 - DAC8:     8-bit DAC with OPA, ref=3.3V.
 * PA25 - ADC:      Analog-to-Digital Converter, ref=3.3V.
 * PA13 - GPIO:     TempPin: General-Purpose I/O for diode.
 */

#define FFT_LENGTH 1024
#define ADC_SAMPLE_SIZE 1024
#define samples 1024
uint16_t adc_result[ADC_SAMPLE_SIZE];
volatile bool adc_flag = false;
uint32_t adc_sample_rate = 0;

void setupDMA(DMA_Regs *dma, uint8_t channelNum, unsigned int srcAddr,
              unsigned int destAddr, unsigned int transferSize) {
  DL_DMA_setSrcAddr(dma, channelNum, (unsigned int)srcAddr);
  DL_DMA_setDestAddr(dma, channelNum, (unsigned int)destAddr);
  DL_DMA_setTransferSize(dma, channelNum, transferSize);
  DL_DMA_enableChannel(dma, channelNum);
}

void ADC12_0_INST_IRQHandler(void) {
  DL_ADC12_getPendingInterrupt(ADC12_0_INST);
  adc_flag = true;
}

void ADC_get_sample() {
  setupDMA(DMA, DMA_CH0_CHAN_ID, (unsigned int)&(ADC0->ULLMEM.MEMRES[0]),
           (unsigned int)adc_result, ADC_SAMPLE_SIZE);
  DL_ADC12_enableConversions(ADC12_0_INST);
  DL_Timer_startCounter(TIMER_0_INST);
  while (false == adc_flag) {
    __WFE();
  }
  DL_Timer_stopCounter(TIMER_0_INST);
  adc_flag = false;
}

void DAC8_byOPA_output(COMP_Regs *comp, OA_Regs *opa, uint32_t value) {
  static uint16_t COMP_REF_VOLTAGE_mV = 3300;
  uint16_t dacValue = value * 255 / COMP_REF_VOLTAGE_mV;
  DL_COMP_setDACCode0(comp, dacValue);
  DL_COMP_enable(comp);
  DL_OPA_enable(opa);
}

void DAC12_output(DAC12_Regs *dac12, uint32_t value) {
  static uint16_t DAC12_REF_VOLTAGE_mV = 2500;
  uint16_t dacValue = value * 4095 / DAC12_REF_VOLTAGE_mV;
  DL_DAC12_output12(dac12, dacValue);
  DL_DAC12_enable(dac12);
}

void wave_control(int mode, int pos) {
  // Cro,top,bot,nor
  int times2delay = 3200000;
  // control offset
  DAC8_byOPA_output(COMP_0_INST, OPA_0_INST, 1650);
  delay_cycles(times2delay);
  int offset;
  if (mode == 0) {
    DL_GPIO_clearPins(GPIO_OUTPUT_PORT, GPIO_OUTPUT_DIODE_PIN);
    switch (pos) {
    case 0:
      offset = 1650;
      TM1638_disp_str("nor.");
      break;
    case -1:
      offset = 1100;
      TM1638_disp_str("nor.bot");
      break;
    case 1:
      offset = 2200;
      TM1638_disp_str("nor.top");
      break;
    }
  } else {
    TM1638_disp_str("Cro.");
    offset = 1650;
    DL_GPIO_setPins(GPIO_OUTPUT_PORT, GPIO_OUTPUT_DIODE_PIN);
  }

  /*set vpp to proper value*/
  int v = 1100;
  int vl = 200;
  int vr = 2000;
  uint16_t vpp = 0;
  // control gain
  DAC12_output(DAC0, v);
  delay_cycles(times2delay);
  for (int i = 0; i < 2; i++) {
    ADC_get_sample();
  }
  vpp = findMax(adc_result, samples) - findMin(adc_result, samples);
  for (int i = 0; i < 10; i++) {
    if (vpp > 3000 * 4096 / 3300) {
      vl = v;
      v = (v + vr) / 2;
    } else {
      vr = v;
      v = (v + vl) / 2;
    }
    DAC12_output(DAC0, v);
    delay_cycles(times2delay);
    for (int i = 0; i < 2; i++) {
      ADC_get_sample();
    }
    vpp = findMax(adc_result, samples) - findMin(adc_result, samples);
  }
  // set offset to proper value
  DAC8_byOPA_output(COMP_0_INST, OPA_0_INST, offset);
  delay_cycles(times2delay);
}

float thd;
float thd_v4;
int max_index;

// 初始时应当以1MHz采样，TIM的Period应当为1us
uint32_t timer_cnt_reg;
uint32_t timer_div_reg;
int tim_max;

int main(void) {
  /*---------Initialization---------*/
  SYSCFG_DL_init();
  NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
  TM1638_init(SPI_0_INST, GPIO_OUTPUT_PORT, GPIO_OUTPUT_CS_PIN);
  TM1638_disp_str("InIt.");
  //   TM1638_disp_str("nor.");
  //   __BKPT(0);
  //   TM1638_disp_str("nor.top");
  //   __BKPT(0);
  //   TM1638_disp_str("nor.bot");
  //   __BKPT(0);
  //   TM1638_disp_str("Cro.");
  //   __BKPT(0);
  __BKPT(0);

  /*---------first FFT to change sample rate---------*/
  TIMA0->COUNTERREGS.LOAD = 63; // 0.5MHz
  ADC_get_sample();
  float fft_inputbuf[FFT_LENGTH * 2]; // FFT输入数组
  float fft_outputbuf[FFT_LENGTH];    // FFT输出数组
  uint32_t fft_int[FFT_LENGTH / 2];
  for (int i = 0; i < FFT_LENGTH; i++) // 生成信号序列
  {
    fft_inputbuf[2 * i] = adc_result[i];
    // 信号实部，直流分量100,1HZ信号幅值为10，50HZ信号幅值为20，300HZ信号幅值为30。
    fft_inputbuf[2 * i + 1] = 0; // 信号虚部，全部为0
  }
  arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_inputbuf, 0, 1);
  arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, FFT_LENGTH);
  for (int i = 0; i < FFT_LENGTH / 2; i++) // 生成信号序列
  {
    fft_int[i] = (uint32_t)fft_outputbuf[i] / 16;
  }
  fft_int[0] = 0;

  max_index = findMaxIndexInRange(fft_int, 0, 511);
  if (findMaxIndexInRange(fft_int, 0, 511) < 3) {
    TIMA0->CLKDIV = 7;
    TIMA0->COUNTERREGS.LOAD = 199;
    timer_div_reg = TIMA0->CLKDIV;
    timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
    for (int i = 0; i < 3; i++) {
      ADC_get_sample();
    }
    for (int i = 0; i < FFT_LENGTH; i++) // 生成信号序列
    {
      fft_inputbuf[2 * i] = adc_result[i];
      fft_inputbuf[2 * i + 1] = 0; // 信号虚部，全部为0
    }
    arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_inputbuf, 0, 1);
    arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, FFT_LENGTH);
    for (int i = 0; i < FFT_LENGTH / 2; i++) // 生成信号序列
    {
      fft_int[i] = (uint32_t)fft_outputbuf[i] / 16;
    }
    fft_int[0] = 0;
  }
  tim_max =
      (TIMA0->COUNTERREGS.LOAD + 1) * 93 / findMaxIndexInRange(fft_int, 0, 511);
  TIMA0->COUNTERREGS.LOAD = tim_max - 1;
  /*----------set proper gain and offset----------*/
  ADC_get_sample();
  wave_control(0, 0);
  __BKPT(0);

  /*----------measure THD by FFT----------*/
  TIMA0->COUNTERREGS.LOAD = 63; // 0.5MHz
  timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
  ADC_get_sample();
  for (int i = 0; i < FFT_LENGTH; i++) // 生成信号序列
  {
    fft_inputbuf[2 * i] = adc_result[i];
    // 信号实部，直流分量100,1HZ信号幅值为10，50HZ信号幅值为20，300HZ信号幅值为30。
    fft_inputbuf[2 * i + 1] = 0; // 信号虚部，全部为0
  }
  arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_inputbuf, 0, 1);
  arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, FFT_LENGTH);
  for (int i = 0; i < FFT_LENGTH / 2; i++) // 生成信号序列
  {
    fft_int[i] = (uint32_t)fft_outputbuf[i] / 16;
  }
  fft_int[0] = 0;
  thd = calculateTHD(fft_int);
  thd_v4 = calculateTHD_v4(fft_int);
  __BKPT(0);

  /*----------LOW FREQ:measure THD by FFT----------*/
  max_index = findMaxIndexInRange(fft_int, 0, 511);
  if (findMaxIndexInRange(fft_int, 0, 511) < 3) {
    TIMA0->CLKDIV = 7;
    TIMA0->COUNTERREGS.LOAD = 199;
    timer_div_reg = TIMA0->CLKDIV;
    timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
    for (int i = 0; i < 3; i++) {
      ADC_get_sample();
    }
    for (int i = 0; i < FFT_LENGTH; i++) // 生成信号序列
    {
      fft_inputbuf[2 * i] = adc_result[i];
      fft_inputbuf[2 * i + 1] = 0; // 信号虚部，全部为0
    }
    arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_inputbuf, 0, 1);
    arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, FFT_LENGTH);
    for (int i = 0; i < FFT_LENGTH / 2; i++) // 生成信号序列
    {
      fft_int[i] = (uint32_t)fft_outputbuf[i] / 16;
    }
    fft_int[0] = 0;
  }
  tim_max =
      (TIMA0->COUNTERREGS.LOAD + 1) * 93 / findMaxIndexInRange(fft_int, 0, 511);
  TIMA0->COUNTERREGS.LOAD = tim_max - 1;
  __BKPT(0);

  for (int i = 0; i < 3; i++) {
    ADC_get_sample();
  }
  for (int i = 0; i < FFT_LENGTH; i++) // 生成信号序列
  {
    fft_inputbuf[2 * i] = adc_result[i];
    fft_inputbuf[2 * i + 1] = 0; // 信号虚部，全部为0
  }
  arm_cfft_f32(&arm_cfft_sR_f32_len1024, fft_inputbuf, 0, 1);
  arm_cmplx_mag_f32(fft_inputbuf, fft_outputbuf, FFT_LENGTH);
  for (int i = 0; i < FFT_LENGTH / 2; i++) // 生成信号序列
  {
    fft_int[i] = (uint32_t)fft_outputbuf[i] / 128;
  }
  fft_int[0] = 0;
  thd = calculateTHD(fft_int);
  thd_v4 = calculateTHD_v4(fft_int);

  char str1[10] = "thd=";
  char str2[5];
  floatToString(thd_v4, str2, 3);
  strcat(str1, str2);
  TM1638_disp_str(str1);
  __BKPT(0);
  while (1) {
  }
}
