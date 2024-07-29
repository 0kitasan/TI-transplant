#include "Lcd_Lib/Adafruit_ST7789.h"
#include "lcd_setup.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"

/*
 * Pin Assignments:
 *
 * PB16 - SPI-CLK:
 * PB08 - SPI-MOSI:
 * PA01 - RST:
 * PB13 - DC:
 * PB20 - CS:
 */
int main(void) {
  SYSCFG_DL_init();
  st7789_lcd_setup();
  while (1) {
    delay_ms(200);
  }
}
