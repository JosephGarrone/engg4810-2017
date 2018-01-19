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

extern xSemaphoreHandle uartSemaphore;
char buffer[160];

void PrintSystem(const char *pcString, ...) {
    va_list vaArgP;
    va_start(vaArgP, pcString);

    if (uartSemaphore == NULL) {
        return;
    }

    if (xSemaphoreTake(uartSemaphore, 50) == pdTRUE) {
        UARTprintf("%s", TAG_SYSTEM);
        vsprintf(buffer, pcString, vaArgP);
        UARTprintf("%s", buffer);

        xSemaphoreGive(uartSemaphore);
    }

    va_end(vaArgP);
}

void PrintLog(const char *pcString, ...) {
    va_list vaArgP;
    va_start(vaArgP, pcString);

    if (uartSemaphore == NULL) {
        return;
    }

    if (xSemaphoreTake(uartSemaphore, 50) == pdTRUE) {
        UARTprintf("%s", TAG_LOG);
        vsprintf(buffer, pcString, vaArgP);
        UARTprintf("%s", buffer);

        xSemaphoreGive(uartSemaphore);
    }

    va_end(vaArgP);
}

void PrintData(const char *pcString, ...) {
    va_list vaArgP;
    va_start(vaArgP, pcString);

    if (uartSemaphore == NULL) {
        return;
    }

    if (xSemaphoreTake(uartSemaphore, 50) == pdTRUE) {
        UARTprintf("%s", TAG_DATA);
        vsprintf(buffer, pcString, vaArgP);
        UARTprintf("%s", buffer);

        xSemaphoreGive(uartSemaphore);
    }

    va_end(vaArgP);
}

