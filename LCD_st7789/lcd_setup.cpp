/*
 * main+.cpp
 *
 *  Created on: Apr 11, 2024
 *      Author: JerryFu
 */

#include "lcd_setup.h"
#include "Lcd_Lib/Adafruit_ST7789.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
// #include "Fonts/FreeSerif12pt7b.h"

using SpiCmd = Adafruit_SPITFT::SpiCmd;

void HAL_Delay(int ms) {
  // converter
  static int cycles_for_1ms = CPU_CLK / 1000;
  delay_cycles(ms * cycles_for_1ms);
}

SPI_Regs *LED_SPI_PTR = SPI_0_INST;
// DC用于区分lcd是接受命令还是数据
GPIO_Regs *LCD_GPIO_DC_PORT = GPIO_OUT_PORT;
uint32_t LCD_GPIO_DC_PIN = GPIO_OUT_DC_PIN;

void spi_transmit_byte(uint8_t data) {
  while (DL_SPI_isBusy(LED_SPI_PTR))
    ;
  DL_SPI_transmitData8(LED_SPI_PTR, data);
  while (DL_SPI_isBusy(LED_SPI_PTR))
    ;
}

void spi_transmit_data(uint8_t *pdata, size_t size) {
  for (int i = 0; i < size; i++) {
    spi_transmit_byte(pdata[i]);
  }
}

void lcd_callback(SpiCmd cmd, uint8_t *pdata, size_t size) {
  //   auto spi = hspi1.Instance;
  switch (cmd) {
  case SpiCmd::init:
    break;

  case SpiCmd::reset:
    break;

  case SpiCmd::cs_low:
    break;

  case SpiCmd::cs_high:
    break;

  case SpiCmd::dc_low:
    DL_GPIO_clearPins(LCD_GPIO_DC_PORT, LCD_GPIO_DC_PIN);
    break;

  case SpiCmd::dc_high:
    DL_GPIO_setPins(LCD_GPIO_DC_PORT, LCD_GPIO_DC_PIN);
    break;

  case SpiCmd::transmit:
    spi_transmit_data(pdata, size);
    break;
  case SpiCmd::delay:
    HAL_Delay(size);
    break;
  }
}

Adafruit_ST7789 lcd(240, 320, lcd_callback);

void setup() {
  HAL_Delay(500);
  lcd.init(240, 320);
  lcd.setRotation(2);
  lcd.fillScreen(ST77XX_BLACK);
  lcd.setCursor(0, 0);
  lcd.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  lcd.printf("Hello World!\n");
  lcd.printf("ST7789 IPS LCD\n");
  lcd.printf("Jerry's embedded\n");
  for (int i = 0; i != 120; ++i) {
    lcd.drawFastHLine(i, 80 + i, 240 - 2 * i, ST77XX_RED);
    lcd.drawFastHLine(i, 320 - 1 - i, 240 - 2 * i, ST77XX_YELLOW);
    lcd.drawFastVLine(i, 80 + i, 240 - 2 * i, ST77XX_GREEN);
    lcd.drawFastVLine(240 - 1 - i, 80 + i, 240 - 2 * i, ST77XX_BLUE);
    HAL_Delay(100);
  }
}
