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
#include "cont.h"

extern xQueueHandle screenQueue;

void PrintContReading(ScreenUpdate screen, int32_t reading, Units units, bool newData) {
    screen.type = READING;

    if (reading % 2 == 0) {
        sprintf(screen.message, "CONTINUOUS");
    } else {
        sprintf(screen.message, "DISCONT.");
    }

    int length = strlen(screen.message);
    for (int i = length; i < 16; i++) {
        screen.message[i] = ' ';
    }
    xQueueSend(screenQueue, &screen, 50);

    if (newData) {
        PrintData("%d,%d,%d\r\n", reading % 2, 0, units);
    }
}

void HandleContReading(ADCReading reading) {
    ScreenUpdate screen;
    PrintContReading(screen, reading.adcValue, UNITS_CONT, reading.newData);
}
