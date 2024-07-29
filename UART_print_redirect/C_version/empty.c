#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <string.h>

int a = 100;

int main(void) {
  SYSCFG_DL_init();
  uint32_t cnt = 0;
  while (1) {
    printf("hello world %d\r\n", cnt);
    cnt++;
    delay_cycles(16000000);
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