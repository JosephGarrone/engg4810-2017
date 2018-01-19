#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "drivers/rgb.h"
#include "drivers/buttons.h"
#include "drivers/ads124x.h"
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
#include "general.h"

extern xQueueHandle screenQueue;

void BaseHandlerInit() {
    GPIOPinTypeGPIOOutput(MUX_PORT, MUX_P1 | MUX_P2 | MUX_P3);
    GPIOPinWrite(MUX_PORT, MUX_P1, 0);
    GPIOPinWrite(MUX_PORT, MUX_P2, 0);
    GPIOPinWrite(MUX_PORT, MUX_P3, MUX_P3);
}

void SetADCGain(float gain) {

}

void GenericLoad() {

}

void GenericUnload() {

}
