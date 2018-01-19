#ifndef __GENERAL_H
#define __GENERAL_H

#include "structs.h"
#include "adc.h"

#define MUX_PORT GPIO_PORTC_BASE
#define MUX_P1 GPIO_PIN_4
#define MUX_P2 GPIO_PIN_5
#define MUX_P3 GPIO_PIN_6

#define ADC_MAX          8388607.0f
#define ADC_MIN          8388608.0f

#define TWELVE_VOLT_GAIN 1.0f
#define FIVE_VOLT_GAIN   2.0f
#define ONE_VOLT_GAIN    8.0f

#define TWELVE_VOLT_MOD  (12.0f * 2.5f)
#define FIVE_VOLT_MOD    (5.0f * 2.5f)
#define ONE_VOLT_MOD     (1.0f * 2.5f)

void PrintReadingContext(ScreenUpdate);
void BaseHandlerInit();
void GenericLoad();
void GenericUnload();
void SetADCGain(float);

#endif
