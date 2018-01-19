#ifndef __LOGIC_H
#define __LOGIC_H

#include "structs.h"

#define LOW_FREQ 500
#define HIGH_FREQ 2000

#define FLOATING -2
#define INVALID -3

#define COMP_PORT GPIO_PORTC_BASE
#define COMP_PIN GPIO_PIN_6

void HandleLogicReading(ADCReading);
void LogicLoad();
void LogicUnload();

#endif
