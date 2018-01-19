#ifndef __RELAY_H
#define __RELAY_H

#define RELAY_DELAY 20 //ms
#define NUM_RELAYS 3

#include <stdbool.h>
#include "structs.h"

#define MUX_S0_PORT GPIO_PORTF_BASE
#define MUX_S0_PIN GPIO_PIN_3
#define MUX_S1_PORT GPIO_PORTE_BASE
#define MUX_S1_PIN GPIO_PIN_0

#define MODE_12V 0b00000011
#define MODE_5V  0b00000001
#define MODE_1V  0b00000010
#define MODE_CUR 0b00000000

void ConfigureRelays();
void SetRelay(Relay);
void ResetRelay(Relay);
bool GetRelayState(Relay);
void SetMux(char);

#endif
