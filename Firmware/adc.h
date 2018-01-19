#ifndef __ADC_H
#define __ADC_H

#include "structs.h"

#define LED1_PORT GPIO_PORTE_BASE
#define LED1_PIN GPIO_PIN_1

#define ADC_NUM_RATES 9
#define ADC_ITEM_SIZE sizeof(ADCRawReading)
#define ADC_QUEUE_SIZE 20

void ADCTaskInit();
void ADCIncreaseSampling();
void ADCDecreaseSampling();
uint32_t GetADCDelay();

#endif
