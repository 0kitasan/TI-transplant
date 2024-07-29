#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <string.h>

// 在c++中，就别把占位符也重定向了，会一直报错，没这个必要
// 反正能用sprintf将数据放到自定义的string-buffer中

int a = 100;

int main(void) {
  SYSCFG_DL_init();
  uint32_t cnt = 0;
  while (1) {
    printf("hello world 233 %d\r\n", cnt);
    cnt++;
    delay_cycles(16000000);
  }
}

// 重定向fputc函数
int fputc(int ch, FILE *f) {
  DL_UART_transmitDataBlocking(UART_0_INST, ch);
  return (ch);
}
