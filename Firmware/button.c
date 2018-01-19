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
#include "button.h"

extern TaskHandle_t buttonTask;

uint32_t buttonStates = 0x00000000;
xQueueHandle buttonQueue;

uint32_t ButtonsRead() {
    uint32_t delta = 0;
    uint32_t data = 0;
    static uint32_t clockA = 0;
    static uint32_t clockB = 0;

    data = (uint8_t)((GPIOPinRead(B1_PORT, B1_PIN) >> B1_SHIFT)) << B1_POS |
           (uint8_t)((GPIOPinRead(B2_PORT, B2_PIN) >> B2_SHIFT)) << B2_POS |
           (uint8_t)((GPIOPinRead(B3_PORT, B3_PIN) >> B3_SHIFT)) << B3_POS |
           (uint8_t)((GPIOPinRead(B4_PORT, B4_PIN) >> B4_SHIFT)) << B4_POS;

    delta = data ^ buttonStates;

    clockA ^= clockB;
    clockB = ~clockB;

    clockA &= delta;
    clockB &= delta;

    buttonStates &= clockA | clockB;
    buttonStates |= (~(clockA | clockB)) & data;

    delta ^= (clockA | clockB);

    return (~buttonStates);
}

void ButtonTask(void *pvParams) {
    uint32_t curButtonState;
    uint32_t prevButtonState;
    ButtonType button;

    while (true) {
        curButtonState = ButtonsRead();

        if (curButtonState != prevButtonState) {
            prevButtonState = curButtonState;

            for (int i = 0; i < 4; i++) {
                if ((curButtonState & (1 << i)) == (1 << i)) {
                    int internal = i;
                    if (i == 3) {
                        internal = 0;
                    }
                    if (i == 2) {
                        internal = 1;
                    }
                    if (i == 1) {
                        internal = 2;
                    }
                    if (i == 0) {
                        internal = 3;
                    }
                    button = (ButtonType)internal;
                    xQueueSend(buttonQueue, &button, 0);
                    break;
                }
            }
        }

        vTaskDelay(25 / portTICK_RATE_MS);
    }
}

void ButtonTaskInit() {
    UARTprintf("LOG:Buttons...\r\n");

    ROM_GPIODirModeSet(B1_PORT, B1_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIODirModeSet(B2_PORT, B2_PIN | B3_PIN, GPIO_DIR_MODE_IN);
    ROM_GPIODirModeSet(B4_PORT, B4_PIN, GPIO_DIR_MODE_IN);

    MAP_GPIOPadConfigSet(B1_PORT, B1_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    MAP_GPIOPadConfigSet(B2_PORT, B2_PIN | B3_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    MAP_GPIOPadConfigSet(B4_PORT, B4_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Initialise the buttons
    ButtonsInit();

    buttonStates = (uint8_t)((GPIOPinRead(B1_PORT, B1_PIN) >> B1_SHIFT)) << B1_POS |
            (uint8_t)((GPIOPinRead(B2_PORT, B2_PIN) >> B2_SHIFT)) << B2_POS |
            (uint8_t)((GPIOPinRead(B3_PORT, B3_PIN) >> B3_SHIFT)) << B3_POS |
            (uint8_t)((GPIOPinRead(B4_PORT, B4_PIN) >> B4_SHIFT)) << B4_POS;
    xTaskCreate(ButtonTask, (const portCHAR*) "Button", BUTTON_STACK_SIZE, NULL, tskIDLE_PRIORITY + BUTTON_PRIORITY, &buttonTask);

    UARTprintf("LOG:Buttons Initialised\r\n");
}
