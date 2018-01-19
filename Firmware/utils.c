#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "drivers/rgb.h"
#include "drivers/buttons.h"
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
#include "utils.h"

#define HIGH 1
#define LOW 1

void DelayMSec(uint32_t delay)
{
    ROM_SysCtlDelay((ROM_SysCtlClockGet() / (3 * 1000)) * delay);
}

void DelayUSec(uint32_t delay) {
    ROM_SysCtlDelay((ROM_SysCtlClockGet() / (3 * 1000000)) * delay);
}

void SelectADC() {
    DeselectSD();

    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, LOW);
}

void DeselectADC() {
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
}

void SelectSD() {
    DeselectADC();

    //GPIOPinWrite(CS_PORT, CS, LOW);
}

void DeselectSD() {
    //GPIOPinWrite(CS_PORT, CS, HIGH);
}
