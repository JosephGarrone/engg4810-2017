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
#include "logic.h"
#include "voltage.h"
#include "general.h"
#include "../drivers/ads124x.h"
#include "utils.h"
#include "relay.h"
#include "math.h"

extern xQueueHandle screenQueue;
extern VoltageRange voltageRange;

uint32_t buzzerPeriod = (CLOCK_FREQ / PWM_DIVIDER) / LOW_FREQ;
uint32_t buzzerDuty = (CLOCK_FREQ / PWM_DIVIDER) / LOW_FREQ / 2; // Default to 50% duty
int logicLevel = FLOATING; // Default to invalid state
int lastLogicLevel = INVALID; // Default to invalid state

float CalculateLogicVoltage(int32_t adcValue) {
    float result = 0.0f;
    float percent = 0.0f;

    if (adcValue < 0) {
        percent = adcValue / ADC_MIN;
    } else {
        percent = adcValue / ADC_MAX;
    }

    result = percent * TWELVE_VOLT_MOD;
    result = -3.5581f * result + 2.5427f;

    return result;
}

void PrintLogicReading(ScreenUpdate screen, float voltage, Units units, bool newData) {
    screen.type = READING;
    bool isFloating = false;

    GPIOPinWrite(COMP_PORT, COMP_PIN, COMP_PIN);

    DelayMSec(1);

    float otherVoltage = CalculateLogicVoltage(ADCGetConversion());

    GPIOPinWrite(COMP_PORT, COMP_PIN, LOW);

    if (abs(otherVoltage - voltage) > 1.0f) {
        isFloating = true;
    }

    if (voltage > 0.6f && !isFloating) {
        lastLogicLevel = logicLevel;
        logicLevel = HIGH;
        buzzerPeriod = (CLOCK_FREQ / PWM_DIVIDER) / HIGH_FREQ;
        PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
        sprintf(screen.message, "HIGH");
    } else if (!isFloating) {
        lastLogicLevel = logicLevel;
        logicLevel = LOW;
        buzzerPeriod = (CLOCK_FREQ / PWM_DIVIDER) / LOW_FREQ;
        PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
        sprintf(screen.message, "LOW");
    } else if (isFloating) {
        PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);
        logicLevel = FLOATING;
        sprintf(screen.message, "FLOATING");
    }
    buzzerDuty = buzzerPeriod / 2;

    if (lastLogicLevel != logicLevel && logicLevel != FLOATING) {
        PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);
        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, buzzerPeriod);
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, buzzerDuty);
        PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
    }

    int length = strlen(screen.message);
    for (int i = length; i < 16; i++) {
        screen.message[i] = ' ';
    }
    xQueueSend(screenQueue, &screen, 50);

    if (newData) {
        if (isFloating) {
            PrintData("0,5,%d\r\n", units);
        } else if (voltage > 0.6f) {
            PrintData("1,0,%d\r\n", units);
        } else if (voltage <= 0.6f) {
            PrintData("0,0,%d\r\n", units);
        }
    }
}

void LogicLoad() {
    // Setup the buzzer pwm
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    GPIOPinConfigure(GPIO_PB5_M0PWM3);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_5);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, buzzerPeriod);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, buzzerDuty);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);

    // Setup the relays
    SetRelay(RELAY_A);
    ResetRelay(RELAY_B);
    ResetRelay(RELAY_C);

    SetMux(MODE_12V);
}

void LogicUnload() {
    // Disable the buzzer
    PWMGenDisable(PWM0_BASE, PWM_GEN_1);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);

    // Setupthe relays
    ResetRelay(RELAY_A);
    ResetRelay(RELAY_B);
    ResetRelay(RELAY_C);

    SetMux(MODE_12V);
}

void HandleLogicReading(ADCReading reading) {
    float voltage;
    ScreenUpdate screen;

    voltage = CalculateLogicVoltage(reading.adcValue);
    PrintLogicReading(screen, voltage, UNITS_LOG, reading.newData);
}
