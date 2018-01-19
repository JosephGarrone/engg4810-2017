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

extern xQueueHandle buttonQueue;
extern xQueueHandle screenQueue;

extern TaskHandle_t inputTask;
extern TaskHandle_t inputOptTask;

void OutputTop() {
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uxArraySize = uxTaskGetNumberOfTasks();
    unsigned long totalRuntime;

    pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

    if( pxTaskStatusArray != NULL )
    {
        uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
                                 uxArraySize,
                                 &totalRuntime );

        totalRuntime /= 100UL;

        for( x = 0; x < uxArraySize; x++ )
        {
            unsigned long timeAsPercent = 0;
            if (totalRuntime > 0) {
                timeAsPercent = pxTaskStatusArray[ x ].ulRunTimeCounter / totalRuntime;
            }
            PrintLog("%s\t\t%d\t\t%d\t\t%d\t\t%u%%\r\n",
                             pxTaskStatusArray[ x ].pcTaskName,
                             pxTaskStatusArray[ x ].eCurrentState,
                             pxTaskStatusArray[ x ].usStackHighWaterMark,
                             pxTaskStatusArray[ x ].ulRunTimeCounter,
                             timeAsPercent);
        }

        /* The array is no longer needed, free the memory it consumes. */
        vPortFree( pxTaskStatusArray );
    }
}

bool AreIdentical(char* src, char* dest) {
    if (strcmp(src, dest) == 0 && strlen(src) == strlen(dest)) {
        return true;
    }

    return false;
}

Command GetCommand(char* str) {
    if (AreIdentical(str, "CMD:b1")) {
        return CMD_B1;
    }
    if (AreIdentical(str, "CMD:b2")) {
        return CMD_B2;
    }
    if (AreIdentical(str, "CMD:b3")) {
        return CMD_B3;
    }
    if (AreIdentical(str, "CMD:b4")) {
        return CMD_B4;
    }
    if (AreIdentical(str, "CMD:b5")) {
        return CMD_B5;
    }
    if (AreIdentical(str, "CMD:b6")) {
        return CMD_B6;
    }
    if (AreIdentical(str, "CMD:b7")) {
        return CMD_B7;
    }
    if (AreIdentical(str, "CMD:b8")) {
        return CMD_B8;
    }
    if (AreIdentical(str, "CMD:b9")) {
        return CMD_B9;
    }
    if (AreIdentical(str, "CMD:b10")) {
        return CMD_B10;
    }
    if (AreIdentical(str, "TOP")) {
        return CMD_B16;
    }
    return CMD_NULL;
}

bool IsCommand(char* input) {
    if (GetCommand(input) != CMD_NULL) {
        return true;
    }
    return false;
}

void HandleInput(char *input, ScreenUpdate screenUpdate, ButtonType button) {
    if (IsCommand(input)) {
        switch (GetCommand(input)) {
            case CMD_B1:
                button = B1;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B2:
                button = B2;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B3:
                button = B3;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B4:
                button = B4;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B5:
                button = B5;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B6:
                button = B6;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B7:
                button = B7;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B8:
                button = B8;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B9:
                button = B9;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B10:
                button = B10;
                xQueueSend(buttonQueue, &button, 50);
                break;
            case CMD_B16:
                OutputTop();
                break;
        }
    } else {
        // Only trim if the length is greater than we can display
        /*int end = strlen(input);
        if (end > 31) {
            end = 31;
        }

        memcpy(screenUpdate.message, &input, end);
        screenUpdate.message[end] = '\0';
        xQueueSend(screenQueue, &screenUpdate, 50);*/
    }
}

void InputTask(void *pvParams) {
    char input[BUFFER_SIZE];
    ScreenUpdate screenUpdate;
    screenUpdate.type = ENTIRE_SCREEN;
    ButtonType button = NO_BUTTON;
    char c;
    int pos = 0;
    bool gotLine = false;

    while (true) {
        if (UARTcharAvail()) {
            c = UARTgetc();

            if (c != -1) {
                if (c != '\r' && c!= '\n' && pos < BUFFER_SIZE - 1) {
                    input[pos++] = c;
                } else if (pos >= BUFFER_SIZE - 1) {
                    input[BUFFER_SIZE - 1] = '\0';
                    pos = 0;
                    gotLine = true;
                } else {
                    input[pos] = '\0';
                    pos = 0;
                    gotLine = true;
                }

                if (gotLine) {
                    gotLine = false;
                    vTaskDelay(10 / portTICK_RATE_MS);
                    HandleInput(input, screenUpdate, button);
                }
            }
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void OpticalInputTask(void *pvParams) {
    char input[BUFFER_SIZE];
    ScreenUpdate screenUpdate;
    screenUpdate.type = ENTIRE_SCREEN;
    ButtonType button = NO_BUTTON;
    char c;
    int pos = 0;
    bool gotLine = false;

    while (true) {
        if (UARTcharAvail2()) {
            c = UARTgetc2();

            if (c != -1) {
                if (c != '\r' && c!= '\n' && pos < BUFFER_SIZE - 1) {
                    input[pos++] = c;
                } else if (pos >= BUFFER_SIZE - 1) {
                    input[BUFFER_SIZE - 1] = '\0';
                    pos = 0;
                    gotLine = true;
                } else {
                    input[pos] = '\0';
                    pos = 0;
                    gotLine = true;
                }

                if (gotLine) {
                    gotLine = false;
                    vTaskDelay(10 / portTICK_RATE_MS);
                    HandleInput(input, screenUpdate, button);
                }
            }
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void InputTaskInit() {
    UARTprintf("LOG:Input...\r\n");

    xTaskCreate(OpticalInputTask, (const portCHAR*) "InputOpt", INPUT_STACK_SIZE, NULL, tskIDLE_PRIORITY + INPUT_PRIORITY, &inputOptTask);
    xTaskCreate(InputTask, (const portCHAR*) "Input", INPUT_STACK_SIZE, NULL, tskIDLE_PRIORITY + INPUT_PRIORITY, &inputTask);

    UARTprintf("LOG:Input Initialised\r\n");
}
