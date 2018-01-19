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
#include "resistance.h"
#include "general.h"

ResistanceRange resistanceRange = ZERO_TO_ONE_MEG;

extern xQueueHandle screenQueue;

void PrintResistanceReading(ScreenUpdate screen, int32_t reading, Units units, bool newData) {
    screen.type = READING;

    sprintf(screen.message, "%6d.%dOhms", reading, !reading);

    int length = strlen(screen.message);
    for (int i = length; i < 16; i++) {
        screen.message[i] = ' ';
    }
    xQueueSend(screenQueue, &screen, 50);

    if (newData) {
        PrintData("%d,%d,%d\r\n", reading, !reading, units);
    }
}

uint32_t CalculateResistanceIntegrand(uint32_t adcValue) {
    if (adcValue > 2048) {
        adcValue -= 2048;
    } else {
        adcValue = 2048 - adcValue;
    }
    switch (resistanceRange) {
        case ZERO_TO_ONE_K:
            return ((adcValue * 100000 * 10) / 2048) / 10;
        case ZERO_TO_ONE_MEG:
            return ((adcValue * 100000 * 33) / 2048) / 10;
    }
    return 0;
}

uint32_t CalculateResistanceDecimal(uint32_t adcValue) {
    if (adcValue > 2048) {
        adcValue -= 2048;
    } else {
        adcValue = 2048 - adcValue;
    }
    switch (resistanceRange) {
        case ZERO_TO_ONE_K:
            return ((adcValue * 100000 * 10) / 2048) % 10;
        case ZERO_TO_ONE_MEG:
            return ((adcValue * 100000 * 33) / 2048) % 10;
    }
    return 0;
}

void HandleResistanceSwitching(uint32_t integrand, uint32_t decimal) {
    if ((integrand > 11 && decimal > 50000) || integrand > 12) { // Overlimit
        if (resistanceRange != OVERLIMIT_RESISTANCE) {
            resistanceRange = OVERLIMIT_RESISTANCE;
        }
    } else if ((integrand > 4 && decimal > 50000) || integrand > 5) { // Enable 12
        if (resistanceRange != ZERO_TO_ONE_MEG) {
            resistanceRange = ZERO_TO_ONE_MEG;

            GPIOPinWrite(MUX_PORT, MUX_P1, 0);
            GPIOPinWrite(MUX_PORT, MUX_P2, MUX_P2);
            GPIOPinWrite(MUX_PORT, MUX_P3, MUX_P3);
        }
    } else { // Enable 5
        if (resistanceRange != ZERO_TO_ONE_K) {
            resistanceRange = ZERO_TO_ONE_K;

            GPIOPinWrite(MUX_PORT, MUX_P1, 0);
            GPIOPinWrite(MUX_PORT, MUX_P2, 0);
            GPIOPinWrite(MUX_PORT, MUX_P3, MUX_P3);
        }
    }
}

void HandleResistanceReading(ADCReading reading) {
    ScreenUpdate screen;
    PrintResistanceReading(screen, reading.adcValue, UNITS_RES, reading.newData);
}
