#include "lcd_setup.h"
#include "Lcd_Lib/Adafruit_ST7789.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"

using SpiCmd = Adafruit_SPITFT::SpiCmd;

void delay_ms(int ms) {
  // converter
  static int cycles_for_1ms = CPU_CLK / 1000;
  delay_cycles(ms * cycles_for_1ms);
}

st7789_lcd::st7789_lcd(SPI_Regs *spi_ptr, GPIO_Regs *dc_port, uint32_t dc_pin,
                       GPIO_Regs *rst_port, uint32_t rst_pin)
    : LCD_SPI_PTR(spi_ptr), LCD_GPIO_DC_PORT(dc_port), LCD_GPIO_DC_PIN(dc_pin),
      LCD_GPIO_RST_PORT(rst_port), LCD_GPIO_RST_PIN(rst_pin) {}

void st7789_lcd::reset(void) {
  DL_GPIO_setPins(LCD_GPIO_RST_PORT, LCD_GPIO_RST_PIN);
  delay_ms(10);
  DL_GPIO_clearPins(LCD_GPIO_RST_PORT, LCD_GPIO_RST_PIN);
  delay_ms(5);
  DL_GPIO_setPins(LCD_GPIO_RST_PORT, LCD_GPIO_RST_PIN);
  delay_ms(10);
}

// 实例化
st7789_lcd my_st7789(SPI_0_INST, GPIO_OUT_PORT, GPIO_OUT_DC_PIN, GPIO_OUT_PORT,
                     GPIO_OUT_RST_PIN);

void spi_transmit_byte(uint8_t data) {
  while (DL_SPI_isBusy(my_st7789.LCD_SPI_PTR))
    ;
  DL_SPI_transmitData8(my_st7789.LCD_SPI_PTR, data);
  while (DL_SPI_isBusy(my_st7789.LCD_SPI_PTR))
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
    DL_GPIO_clearPins(my_st7789.LCD_GPIO_DC_PORT, my_st7789.LCD_GPIO_DC_PIN);
    break;

  case SpiCmd::dc_high:
    DL_GPIO_setPins(my_st7789.LCD_GPIO_DC_PORT, my_st7789.LCD_GPIO_DC_PIN);
    break;

  case SpiCmd::transmit:
    spi_transmit_data(pdata, size);
    break;
  case SpiCmd::delay:
    delay_ms(size);
    break;
  }
}

Adafruit_ST7789 lcd(240, 320, lcd_callback);

void setup() {
  // should reset
  my_st7789.reset();
  lcd.init(240, 320);
  lcd.setRotation(2);
  // lcd.fillScreen(ST77XX_BLACK);
  lcd.fillScreen(ST77XX_CYAN);
  lcd.setCursor(0, 0);
  lcd.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  lcd.printf("Hello World!\n");
  lcd.printf("ST7789 IPS LCD, TI transplant\n");
  lcd.printf("Jerry's embedded\n");
  for (int i = 0; i != 120; ++i) {
    lcd.drawFastHLine(i, 80 + i, 240 - 2 * i, ST77XX_RED);
    lcd.drawFastHLine(i, 320 - 1 - i, 240 - 2 * i, ST77XX_YELLOW);
    lcd.drawFastVLine(i, 80 + i, 240 - 2 * i, ST77XX_GREEN);
    lcd.drawFastVLine(240 - 1 - i, 80 + i, 240 - 2 * i, ST77XX_BLUE);
    delay_ms(10);
  }
}
