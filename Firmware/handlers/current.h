#ifndef __CURRENT_H
#define __CURRENT_H

#include "structs.h"

#define CURRENT_OVER 195.0f
#define CURRENT_UP_200 10.0f
#define CURRENT_DOWN_10 9.0f

void HandleCurrentReading(ADCReading);
void CurrentDCLoad();
void CurrentDCUnload();
void CurrentACLoad();
void CurrentACUnload();

#endif
