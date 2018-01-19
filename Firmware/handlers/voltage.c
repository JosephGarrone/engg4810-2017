#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

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
#include "voltage.h"
#include "general.h"
#include "relay.h"

VoltageRange voltageRange = TWELVE_TO_TWELVE;

extern xQueueHandle screenQueue;

uint32_t lastVReading = 0;
uint32_t currentVReading = 0;

void PrintVoltageReading(ScreenUpdate screen, float voltage, Units units, bool newData) {
    char floatBuffer[20];
    screen.type = READING;

    if (voltage >= VOLT_OVER || voltage <= -VOLT_OVER) {
        sprintf(screen.message, "OVER LIMIT");
    } else {
        if (units == UNITS_AC) {
            sprintf(screen.message, "%  9.5fV", voltage);
        } else {
            sprintf(screen.message, "%  9.5fV", voltage);
        }
    }

    int length = strlen(screen.message);
    for (int i = length; i < 16; i++) {
        screen.message[i] = ' ';
    }

    switch (voltageRange) {
        case OVERLIMIT_VOLTAGE:
        case TWELVE_TO_TWELVE:
            memcpy(&(screen.message[11]), "\x01""12V", 4);
            break;
        case FIVE_TO_FIVE:
            memcpy(&(screen.message[12]), "\x01""5V", 3);
            break;
        case ONE_TO_ONE:
            memcpy(&(screen.message[12]), "\x01""1V", 3);
            break;
    }

    xQueueSend(screenQueue, &screen, 50);

    if (newData) {
        int splitLocation = 0;
        sprintf(floatBuffer, "%f", voltage);
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

float CalculateVoltage(int32_t adcValue) {
    float result = 0.0f;
    float percent = 0.0f;

    if (adcValue < 0) {
        percent = adcValue / ADC_MIN;
    } else {
        percent = adcValue / ADC_MAX;
    }

    if (voltageRange == OVERLIMIT_VOLTAGE || voltageRange == TWELVE_TO_TWELVE) {
        result = percent * TWELVE_VOLT_MOD;
        result = -3.5581f * result + 2.5427f;
    } else if (voltageRange == FIVE_TO_FIVE) {
        result = percent * FIVE_VOLT_MOD;
        result = -3.5950f * result + 1.0210f;
    } else {
        result = percent * ONE_VOLT_MOD;
        result = -3.5637f * result + 0.1808f;
    }

    return result;
}

void Enter12VRange() {
    ResetRelay(RELAY_A);
    ResetRelay(RELAY_B);
    ResetRelay(RELAY_C);

    SetMux(MODE_12V);
}

void Enter5VRange() {
    ResetRelay(RELAY_A);
    ResetRelay(RELAY_B);
    ResetRelay(RELAY_C);

    SetMux(MODE_5V);
}

void Enter1VRange() {
    ResetRelay(RELAY_A);
    ResetRelay(RELAY_B);
    ResetRelay(RELAY_C);

    SetMux(MODE_1V);
}

void HandleVoltageSwitching(float voltage) {
    if (voltage >= VOLT_OVER || voltage <= -VOLT_OVER) { // Overlimit
        if (voltageRange != OVERLIMIT_VOLTAGE) {
            voltageRange = OVERLIMIT_VOLTAGE;

            Enter12VRange();

            lastVReading = currentVReading;
        }
    } else if (voltage >= VOLT_UP_12_TRANS || voltage <= -VOLT_UP_12_TRANS) { // Enable 12
        if (voltageRange != TWELVE_TO_TWELVE) {
            voltageRange = TWELVE_TO_TWELVE;

            Enter12VRange();

            lastVReading = currentVReading;
        }
    } else if ((voltage >= VOLT_UP_5_TRANS && voltage <= VOLT_DOWN_5_TRANS) ||
            (voltage <= -VOLT_UP_5_TRANS && voltage >= -VOLT_DOWN_5_TRANS)) { // Enable 5
        if (voltageRange != FIVE_TO_FIVE) {
            voltageRange = FIVE_TO_FIVE;

            Enter5VRange();

            lastVReading = currentVReading;
        }
    } else if ((voltage <= VOLT_DOWN_1_TRANS || voltage >= -VOLT_DOWN_1_TRANS)) { // Enable 1
        if (voltageRange != ONE_TO_ONE) {
            voltageRange = ONE_TO_ONE;

            Enter1VRange();

            lastVReading = currentVReading;
        }
    }
}

void VoltageDCLoad() {
    voltageRange = TWELVE_TO_TWELVE;

    // Setup relays
    Enter12VRange();
}

void VoltageDCUnload() {
    // Setup relays
    Enter12VRange();
}

void VoltageACLoad() {
    voltageRange = TWELVE_TO_TWELVE;

    // Setup relays
    Enter12VRange();
}

void VoltageACUnload() {
    // Setup relays
    Enter12VRange();
}

void HandleDCReading(ADCReading reading) {
    float voltage;
    ScreenUpdate screen;

    currentVReading = xTaskGetTickCount();
    if (currentVReading - lastVReading < 1000) {
        return;
    }

    voltage = CalculateVoltage(reading.adcValue);
    PrintVoltageReading(screen, voltage, UNITS_DC, reading.newData);
    if (reading.newSample) {
        HandleVoltageSwitching(voltage);
    }
}
void HandleACReading(ADCReading reading) {
    float voltage;
    ScreenUpdate screen;

    currentVReading = xTaskGetTickCount();
    if (currentVReading - lastVReading < 1000) {
        return;
    }

    voltage = CalculateVoltage(reading.adcValue);
    PrintVoltageReading(screen, voltage, UNITS_AC, reading.newData);
    if (reading.newSample) {
        HandleVoltageSwitching(voltage);
    }
}
