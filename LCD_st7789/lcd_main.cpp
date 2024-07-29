#include "Lcd_Lib/Adafruit_ST7789.h"
#include "lcd_setup.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"

// #include "Fonts/FreeSerif12pt7b.h"
/*
 * Pin Assignments:
 *
 * PB09 - SPI-CLK:
 * PA18 - SPI-MOSI:
 * PA24 - RST:
 * PA17 - DC:  always high
 * PA15 - CS:  always low
 */

using SpiCmd = Adafruit_SPITFT::SpiCmd;

int main(void) {
  SYSCFG_DL_init();

  while (1) {
    setup();
    HAL_Delay(200);
  }
}
