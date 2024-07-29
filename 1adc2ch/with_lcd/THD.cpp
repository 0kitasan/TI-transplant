#include "CalcLib_FFT.h"
#include "adc_setup.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "cmsis_gcc.h"
#include "lcd_setup.h"
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
 * PB09 - SPI-CLK:
 * PA18 - SPI-MOSI:
 * PA12 - RST:
 * PA31 - DC:  always high
 * PB28 - CS:  always low
 */

char adc0_charbuf[20];
char adc1_charbuf[20];

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
  delay_ms(1);
  
  timer_cnt_reg = TIMA0->COUNTERREGS.LOAD;
  sprintf(ch_buffer, "%d", timer_cnt_reg);
  printf("timer_cnt_reg=%s\r\n", ch_buffer);
  printf("debug start!\r\n");
  __BKPT(0);
  setup();
  __BKPT(0);
  adc_get_sample();
  uint16_t adc0_mv;
  for (int i = 0; i < ADC_SAMPLE_SIZE; i++) {
    adc0_mv = adc_voltage_mv_trans(adc0_result[i]);
    sprintf(adc0_charbuf, "%d", adc0_mv);
    if (i % 2 == 0) {
      sprintf(ch_buffer, "%d", i / 2);
      printf("%s:\t ADC0_CH1_RES=%s,\t", ch_buffer, adc0_charbuf);
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

// // 重定向fputs函数
// int fputs(const char *restrict s, FILE *restrict stream) {
//   uint16_t i, len;
//   len = strlen(s);
//   for (i = 0; i < len; i++) {
//     DL_UART_transmitDataBlocking(UART_0_INST, s[i]);
//   }
//   return len;
// }

// // 重定向puts函数
// int puts(const char *_ptr) {
//   int count = fputs(_ptr, stdout);
//   count += fputs("\n", stdout);
//   return count;
// }