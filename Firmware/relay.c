#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "FreeRTOS.h"
#include "third_party/TivaLCD/display.h"
#include "driverlib/rom.h"
#include "driverlib/pwm.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "customstdio.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "system.h"
#include "defines.h"
#include "structs.h"
#include "screen.h"
#include "print.h"
#include "relay.h"
#include "utils.h"
#include "structs.h"

static bool relay_states[NUM_RELAYS];
static int relay_pins[NUM_RELAYS][4] = {
    //            RESET                        SET
    {GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PORTB_BASE, GPIO_PIN_3},
    {GPIO_PORTC_BASE, GPIO_PIN_5, GPIO_PORTC_BASE, GPIO_PIN_4},
    {GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PORTC_BASE, GPIO_PIN_6},
};

void ConfigureMux() {
    GPIOPinTypeGPIOOutput(MUX_S1_PORT, MUX_S1_PIN);
    GPIOPinTypeGPIOOutput(MUX_S0_PORT, MUX_S0_PIN);
}

void ConfigureRelays() {
    for (int i = 0; i < NUM_RELAYS; i++) {
        for (int j = 0; j < 4; j += 2) {
            GPIOPinTypeGPIOOutput(relay_pins[i][j], relay_pins[i][j+1]);
            GPIOPinWrite(relay_pins[i][j], relay_pins[i][j+1], LOW);
        }

        relay_states[i] = true;
    }

    ConfigureMux();
}

void SetMux(char mode) {
    char s1 = mode & (1 << 0);
    char s0 = mode & (1 << 1);

    if (s1) {
        GPIOPinWrite(MUX_S1_PORT, MUX_S1_PIN, MUX_S1_PIN);
    } else {
        GPIOPinWrite(MUX_S1_PORT, MUX_S1_PIN, LOW);
    }

    if (s0) {
        GPIOPinWrite(MUX_S0_PORT, MUX_S0_PIN, MUX_S0_PIN);
    } else {
        GPIOPinWrite(MUX_S0_PORT, MUX_S0_PIN, LOW);
    }
}

void SetRelay(Relay relay) {
    GPIOPinWrite(relay_pins[(int)relay][0], relay_pins[(int)relay][1], relay_pins[(int)relay][1]);
    GPIOPinWrite(relay_pins[(int)relay][2], relay_pins[(int)relay][3], LOW);

    DelayMSec(RELAY_DELAY);

    GPIOPinWrite(relay_pins[(int)relay][0], relay_pins[(int)relay][1], LOW);
    GPIOPinWrite(relay_pins[(int)relay][2], relay_pins[(int)relay][3], LOW);

    relay_states[(int)relay] = false;
}

void ResetRelay(Relay relay) {
    GPIOPinWrite(relay_pins[(int)relay][0], relay_pins[(int)relay][1], LOW);
    GPIOPinWrite(relay_pins[(int)relay][2], relay_pins[(int)relay][3], relay_pins[(int)relay][3]);

    DelayMSec(RELAY_DELAY);

    GPIOPinWrite(relay_pins[(int)relay][0], relay_pins[(int)relay][1], LOW);
    GPIOPinWrite(relay_pins[(int)relay][2], relay_pins[(int)relay][3], LOW);

    relay_states[(int)relay] = true;
}

bool GetRelayState(Relay relay) {
    return relay_states[(int)relay];
}
