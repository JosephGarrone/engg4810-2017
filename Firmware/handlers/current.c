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
#include "current.h"
#include "general.h"
#include "utils.h"
#include "relay.h"

CurrentRange currentRange = TWO_HUNNIT_TO_TWO_HUNNIT;

extern xQueueHandle screenQueue;

uint32_t lastAReading = 0;
uint32_t currentAReading = 0;

void PrintCurrentReading(ScreenUpdate screen, float current, Units units, bool newData) {
    char floatBuffer[20];
    screen.type = READING;

    if (current >= CURRENT_OVER || current <= -CURRENT_OVER) {
        sprintf(screen.message, "OVER LIMIT");
    } else {
        if (units == UNITS_AC) {
            sprintf(screen.message, "%  5.1f""mA", current);
        } else {
            sprintf(screen.message, "%  5.1f""mA", current);
        }
    }

    int length = strlen(screen.message);
    for (int i = length; i < 16; i++) {
        screen.message[i] = ' ';
    }

    switch (currentRange) {
        case OVERLIMIT_CURRENT:
        case TWO_HUNNIT_TO_TWO_HUNNIT:
            memcpy(&(screen.message[10]), "\x01""0.2A", 4);
            break;
        case TEN_TO_TEN:
            memcpy(&(screen.message[10]), "\x01""10mA", 3);
            break;
    }

    xQueueSend(screenQueue, &screen, 50);

    if (newData) {
        int splitLocation = 0;
        sprintf(floatBuffer, "%f", current);
        int strLen = strlen(floatBuffer);

        for (int i = 0; i < strLen; i++) {
            if (floatBuffer[i] == '.') {
                splitLocation = i + 1;
                floatBuffer[i] = '\0';
                break;
            }
        }
        PrintData("%s,%s,%d\r\n", floatBuffer, &(floatBuffer[splitLocation]), units);
    }
}

float CalculateCurrent(int32_t adcValue) {
    float result = 0.0f;
    float percent = 0.0f;

    if (adcValue < 0) {
        percent = adcValue / ADC_MIN;
    } else {
        percent = adcValue / ADC_MAX;
    }

    if (currentRange == OVERLIMIT_CURRENT || currentRange == TWO_HUNNIT_TO_TWO_HUNNIT) {
        result = percent * 1000;
        result = -0.9801f * result + 33.733f;
        result = 1.0255f * result + 2.5148f;
    } else {
        result = percent * 1000;
        result = 1 * result + 0;
    }

    return result;
}

void Enter10CurrentRange() {
    ResetRelay(RELAY_A);
    ResetRelay(RELAY_B);
    SetRelay(RELAY_C);

    SetMux(MODE_CUR);
}

void Enter200CurrentRange() {
    ResetRelay(RELAY_A);
    ResetRelay(RELAY_B);
    ResetRelay(RELAY_C);

    SetMux(MODE_CUR);
}

void HandleCurrentSwitching(float current) {
    if (current >= CURRENT_OVER || current <= -CURRENT_OVER) {
        if (currentRange != OVERLIMIT_CURRENT) {
            currentRange = OVERLIMIT_CURRENT;

            Enter200CurrentRange();

            lastAReading = currentAReading;
        }
    } else if (current >= CURRENT_UP_200 || current <= -CURRENT_UP_200) {
        if (currentRange != TWO_HUNNIT_TO_TWO_HUNNIT) {
            currentRange = TWO_HUNNIT_TO_TWO_HUNNIT;

            Enter10CurrentRange();

            lastAReading = currentAReading;
        }
    } else if (current <= CURRENT_DOWN_10 || current >= -CURRENT_DOWN_10) {
        if (currentRange != TEN_TO_TEN) {
            currentRange = TEN_TO_TEN;

            Enter10CurrentRange();

            lastAReading = currentAReading;
        }
    }
}

void CurrentDCLoad() {
    currentRange = TWO_HUNNIT_TO_TWO_HUNNIT;

    Enter200CurrentRange();
}

void CurrentDCUnload() {
    Enter200CurrentRange();
}

void CurrentACLoad() {

}

void CurrentACUnload() {

}

void HandleCurrentReading(ADCReading reading) {
    float current;
    ScreenUpdate screen;

    currentAReading = xTaskGetTickCount();
    if (currentAReading - lastAReading < 1000) {
        return;
    }

    current = CalculateCurrent(reading.adcValue);
    PrintCurrentReading(screen, current, UNITS_CURRENT, reading.newData);
    if (reading.newSample) {
        HandleCurrentSwitching(current);
    }
}
