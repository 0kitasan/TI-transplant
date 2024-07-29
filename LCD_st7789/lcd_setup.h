/*
 * main+.cpp
 *
 *  Created on: Apr 11, 2024
 *      Author: JerryFu
 */

#ifndef LCD_SETUP_H
#define LCD_SETUP_H
#include "Lcd_Lib/Adafruit_ST7789.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"

using SpiCmd = Adafruit_SPITFT::SpiCmd;

#define CPU_CLK 32000000

void HAL_Delay(int ms);
extern SPI_Regs *LED_SPI_PTR;
// DC用于区分lcd是接受命令还是数据
extern GPIO_Regs *LCD_GPIO_DC_PORT;
extern uint32_t LCD_GPIO_DC_PIN;
void spi_transmit_byte(uint8_t data);
void spi_transmit_data(uint8_t *pdata, size_t size);
void lcd_callback(SpiCmd cmd, uint8_t *pdata, size_t size);
extern Adafruit_ST7789 lcd;
// 声明 setup 函数
void setup();

#endif // LCD_SETUP_H
