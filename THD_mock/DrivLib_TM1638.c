// DrivLib_TM1638.c
#include "DrivLib_TM1638.h"
#include "ti_msp_dl_config.h"
#include <stdio.h> // 包含标准输入输出库
#include <string.h>

// 引脚定义
// #define GPIO_SPI_0_CS0_PORT GPIOB #define GPIO_SPI_0_CS0_PIN DL_GPIO_PIN_6
// PB9 SCLK
// PB8 MOSI
// PB7 MISO
// PB6 CS0
// 单独使用1个GPIO控制CS
// PB13 GPIO_CS
// PA12 GPIO_CS

/*
 -- A --
|       |
F       B
|       |
 -- G --
|       |
E       C
|       |
 -- D --  DP

0b-PGFEDCBA
*/

const uint8_t DIGITS[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66,
                            0x6d, 0x7d, 0x07, 0x7f, 0x6f};
const uint8_t charTable[CHAR_TABLE_SIZE] = {
    ['0'] = 0x3f, // 0b00111111
    ['1'] = 0x06, // 0b00000110
    ['2'] = 0x5b, // 0b01011011
    ['3'] = 0x4f, // 0b01001111
    ['4'] = 0x66, // 0b01100110
    ['5'] = 0x6d, // 0b01101101
    ['6'] = 0x7d, // 0b01111101
    ['7'] = 0x07, // 0b00000111
    ['8'] = 0x7f, // 0b01111111
    ['9'] = 0x6f, // 0b01101111
    ['A'] = 0x77, // 0b01110111
    ['b'] = 0x7C, // 0b01111100
    ['C'] = 0x39, // 0b00111001
    ['d'] = 0x5E, // 0b01011110
    ['E'] = 0x79, // 0b01111001
    ['F'] = 0x71, // 0b01110001
    ['H'] = 0x76, // 0b01110110
    ['I'] = 0x30, // 0b00110000
    ['J'] = 0x1E, // 0b00011110
    ['L'] = 0x38, // 0b00111000
    ['o'] = 0x5C, // 0b01011100
    ['P'] = 0x73, // 0b01110011
    ['r'] = 0x50, // 0b01010000
    ['U'] = 0x3E, // 0b00111110
    [' '] = 0x00, // 0b00000000
    ['.'] = 0x80, // 0b10000000
    ['='] = 0x48, // 0b01001000
    ['t'] = 0x78, // 0b01111000
    ['h'] = 0x74, // 0b01110100
    ['n'] = 0x54, // 0b01010100
    ['p'] = 0x73  // 0b01110011
};

SPI_Regs *TM1638_SPI_PTR = NULL;
GPIO_Regs *TM1638_GPIO_CS_PORT = NULL;
uint32_t TM1638_GPIO_CS_PIN = 0;

void TM1638_init(SPI_Regs *USR_TM1638_SPI_PTR,
                 GPIO_Regs *USR_TM1638_GPIO_CS_PORT,
                 uint32_t USR_TM1638_GPIO_CS_PIN) {
  TM1638_SPI_PTR = USR_TM1638_SPI_PTR;
  TM1638_GPIO_CS_PORT = USR_TM1638_GPIO_CS_PORT;
  TM1638_GPIO_CS_PIN = USR_TM1638_GPIO_CS_PIN;
}

// 将单个字符转换为七段显示器编码
uint8_t charToSegment(char c) {
  if (c < CHAR_TABLE_SIZE) {
    return charTable[(uint8_t)c];
  }
  return 0x00; // 对于不在表中的字符，返回空显示编码
}

// 将字符串转换为七段显示器编码数组
void stringToSegments(const char *str, uint8_t *segments, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    segments[i] = charToSegment(str[i]);
  }
}

void spi_transmit_byte(uint8_t data) {
  while (DL_SPI_isBusy(TM1638_SPI_PTR))
    ;
  DL_SPI_transmitData8(TM1638_SPI_PTR, data);
  while (DL_SPI_isBusy(TM1638_SPI_PTR))
    ;
}

// 由于自增模式出现了显示起点偏移的bug，因此只能使用固定地址模式
void spi_send_cmd(uint8_t cmd) {
  DL_GPIO_clearPins(TM1638_GPIO_CS_PORT, TM1638_GPIO_CS_PIN);
  spi_transmit_byte(cmd);
  DL_GPIO_setPins(TM1638_GPIO_CS_PORT, TM1638_GPIO_CS_PIN);
}

const uint8_t TM1638_INIT_ADDRESS = 0xC0;

void TM1638_write_segment(uint8_t pos, uint8_t segment_code) {
  DL_GPIO_clearPins(TM1638_GPIO_CS_PORT, TM1638_GPIO_CS_PIN);
  spi_transmit_byte(TM1638_INIT_ADDRESS + pos);
  spi_transmit_byte(segment_code);
  DL_GPIO_setPins(TM1638_GPIO_CS_PORT, TM1638_GPIO_CS_PIN);
}

void TM1638_reset(void) {
  spi_send_cmd(0x44);
  for (int i = 0; i < 16; i++) {
    TM1638_write_segment(i, 0x00);
  }
  spi_send_cmd(0x8F);
}

void TM1638_disp_int(uint32_t num2disp) {
  uint8_t digits[8] = {0};           // TM1638 支持最多 8 位数字显示
  static uint8_t data_set[16] = {0}; // 16 个显示寄存器
  // 提取每一位数字，从高位到低位存储
  for (int i = 0; i < 8; ++i) {
    digits[7 - i] = num2disp % 10;
    num2disp /= 10;
  }
  // 将数字转换为 TM1638 的显示数据格式
  for (int i = 0; i < 8; ++i) {
    data_set[i * 2] = DIGITS[digits[i]];
  }
  spi_send_cmd(0x44);
  for (int i = 0; i < 16; i++) {
    TM1638_write_segment(i, data_set[i]);
  }
  spi_send_cmd(0x8F); // 打开显示并设置亮度
}

void TM1638_disp_str(const char *str2disp) {
  uint8_t length = strlen(str2disp);
  uint8_t text_on_segments[length];
  uint8_t segments[16] = {0}; // 初始化为0

  // 将字符串转换为七段显示器编码
  stringToSegments(str2disp, text_on_segments, length);
  // 填充 segments 数组
  int seg_idx = 0;
  for (int txt_idx = 0; txt_idx < length && seg_idx < 8; txt_idx++) {
    if (str2disp[txt_idx] == '.') {
      if (seg_idx > 0) {
        segments[(seg_idx - 1) * 2] |= 0x80; // 将前一个字符段码与小数点组合
      }
    } else {
      segments[seg_idx * 2] = text_on_segments[txt_idx];
      seg_idx++;
    }
  }
  spi_send_cmd(0x44);
  for (int i = 0; i < 16; i++) {
    TM1638_write_segment(i, segments[i]);
  }
  spi_send_cmd(0x8F);
}

// int main(void) {
//   SYSCFG_DL_init();

//   /* Set LED to indicate start of transfer */
//   DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);

//   DL_GPIO_setPins(GPIO_CS_PORT, GPIO_CS_PIN_PIN);
//   TM1638_reset();
//   delay_cycles(16000000);
//   TM1638_disp_str("thd=11.45");

//   __BKPT();
//   /* After all data is transmitted, toggle LED */
//   while (1) {
//     DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
//     delay_cycles(16000000);
//   }
// }
