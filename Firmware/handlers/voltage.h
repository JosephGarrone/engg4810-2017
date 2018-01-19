#ifndef __VOLTAGE_H
#define __VOLTAGE_H

#include "structs.h"

#define VOLT_OVER 11.5f
#define VOLT_UP_12_TRANS 5.0f
#define VOLT_DOWN_5_TRANS 4.75f
#define VOLT_UP_5_TRANS 1.00f
#define VOLT_DOWN_1_TRANS 0.90f

void HandleACReading(ADCReading);
void HandleDCReading(ADCReading);
void VoltageDCLoad();
void VoltageDCUnload();
void VoltageACLoad();
void VoltageACUnload();

#endif
