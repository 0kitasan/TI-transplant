// adc_setup.h
#ifndef ADC_SETUP_H
#define ADC_SETUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "ti/driverlib/dl_adc12.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"

#define FFT_LENGTH 1024
#define ADC_SAMPLE_SIZE 1024

extern uint16_t adc0_result[ADC_SAMPLE_SIZE];
extern volatile bool adc0_flag;

void setupDMA(DMA_Regs *dma, uint8_t channelNum, unsigned int srcAddr, unsigned int destAddr, unsigned int transferSize);
void ADC12_0_INST_IRQHandler(void);
void adc_get_sample(void);
uint16_t adc_voltage_mv_trans(uint16_t adc_data);

#ifdef __cplusplus
}
#endif

#endif // ADC_SETUP_H