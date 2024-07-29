// DrivLib_TM1638.h
#ifndef DRIVLIB_TM1638_H
#define DRIVLIB_TM1638_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

extern const uint8_t DIGITS[10];
#define CHAR_TABLE_SIZE 128
extern const uint8_t charTable[CHAR_TABLE_SIZE];

extern SPI_Regs *TM1638_SPI_PTR;
extern GPIO_Regs *TM1638_GPIO_CS_PORT;
extern uint32_t TM1638_GPIO_CS_PIN;
extern const uint8_t TM1638_INIT_ADDRESS;

void TM1638_init(SPI_Regs *USR_TM1638_SPI_PTR,
                 GPIO_Regs *USR_TM1638_GPIO_CS_PORT,
                 uint32_t USR_TM1638_GPIO_CS_PIN);
uint8_t charToSegment(char c);
void stringToSegments(const char *str, uint8_t *segments, uint8_t length);
void spi_transmit_byte(uint8_t data);
void spi_send_cmd(uint8_t cmd);
void TM1638_write_segment(uint8_t pos, uint8_t segment_code);
void TM1638_reset(void);
void TM1638_disp_int(uint32_t num2disp);
void TM1638_disp_str(const char *str2disp);

#endif // DRIVLIB_TM1638_H