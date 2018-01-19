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

extern TaskHandle_t screenTask;

xQueueHandle screenQueue;

static uint8_t charPlusMinus[8] = {
    0b00000,
    0b00100,
    0b00100,
    0b11111,
    0b00100,
    0b00100,
    0b0000,
    0b11111
};
static uint8_t brightness1[8] = {
    0b11011,
    0b10001,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b10101,
    0b11011
};
static uint8_t brightness2[8] = {
    0b11011,
    0b10001,
    0b00000,
    0b00000,
    0b00000,
    0b01110,
    0b10101,
    0b11011
};
static uint8_t brightness3[8] = {
    0b11011,
    0b10001,
    0b00000,
    0b00000,
    0b11111,
    0b01110,
    0b10101,
    0b11011
};
static uint8_t brightness4[8] = {
    0b11011,
    0b10001,
    0b00000,
    0b11111,
    0b11111,
    0b01110,
    0b10101,
    0b11011
};
static uint8_t brightness5[8] = {
    0b11011,
    0b10001,
    0b01110,
    0b11111,
    0b11111,
    0b01110,
    0b10101,
    0b11011
};
static uint8_t brightness6[8] = {
    0b11011,
    0b10101,
    0b01110,
    0b11111,
    0b11111,
    0b01110,
    0b10101,
    0b11011
};
uint8_t currentBrightness = BRIGHTNESS_6;
uint32_t period = (CLOCK_FREQ / PWM_DIVIDER) / LED_FREQ;
uint32_t duty = (CLOCK_FREQ / PWM_DIVIDER) / LED_FREQ; // Default to always on
uint32_t dutyIncrement = ((CLOCK_FREQ / PWM_DIVIDER) / LED_FREQ) / BRIGHTNESS_STEPS;
static char currentScreen[33] = "";

int GetBrightness() {
    return (currentBrightness - MIN_BRIGHTNESS);
}

void ScreenTop(char* dest, char* top) {
    strncpy(dest, top, 16);
    if (strlen(top) < 16) {
        for (int i = strlen(top); i < 16; i++) {
            dest[i] = ' ';
        }
    }
}

void ScreenBot(char* dest, char* bottom) {
    strncpy(&(dest[16]), bottom, 16);
    if (strlen(bottom) < 16) {
        for (int i = strlen(bottom); i < 16; i++) {
            dest[16 + i] = ' ';
        }
    }
}

void ScreenOff() {
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 0);
}

void ScreenRestore() {
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, duty - ((currentBrightness - MIN_BRIGHTNESS) * dutyIncrement));
}

void ScreenDecreaseBrightness(ScreenUpdate screenUpdate) {
    screenUpdate.type = BRIGHTNESS;
    if (currentBrightness > MIN_BRIGHTNESS) {
        currentBrightness--;

        PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, duty - ((currentBrightness - MIN_BRIGHTNESS) * dutyIncrement));

        PrintLog("Brightness at level %d\r\n", currentBrightness - MIN_BRIGHTNESS);

        xQueueSend(screenQueue, &screenUpdate, 50);
    }
}

void ScreenIncreaseBrightness(ScreenUpdate screenUpdate) {
    screenUpdate.type = BRIGHTNESS;
    if (currentBrightness < MAX_BRIGHTNESS) {
        currentBrightness++;

        PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, duty - ((currentBrightness - MIN_BRIGHTNESS) * dutyIncrement));

        PrintLog("Brightness at level %d\r\n", currentBrightness - MIN_BRIGHTNESS);

        xQueueSend(screenQueue, &screenUpdate, 50);
    }
}

void ScreenClear(ScreenUpdate screen) {
    screen.type = CLEAR_SCREEN;
    xQueueSend(screenQueue, &screen, 50);
}

void ScreenTask(void *pvParams) {
    ScreenUpdate update;
    char temp[17];
    char top[17];
    char bottom[17];

    update.type = ENTIRE_SCREEN;
    strcpy(update.message, "    ENGG4810    H.B,P.C,J.G,A.T");
    xQueueSend(screenQueue, &update, 50);

    while (true) {
        if (xQueueReceive(screenQueue, &update, 0) == pdPASS) {
            switch (update.type) {
                case ENTIRE_SCREEN: {
                    int length = strlen(update.message);
                    int topEnd = length;
                    if (topEnd > 16) {
                        topEnd = 16;
                    }
                    int bottomEnd = length - 16;
                    if (bottomEnd > 16) {
                        bottomEnd = 16;
                    } else if (bottomEnd < 0) {
                        bottomEnd = 0;
                    }

                    memcpy(top, &(update.message), 16);
                    for (int i = topEnd; i < 16; i++) {
                        top[i] = ' ';
                    }
                    top[16] = '\0';

                    memcpy(bottom, &(update.message[16]), 16);
                    for (int i = bottomEnd; i < 16; i++) {
                        bottom[i] = ' ';
                    }
                    bottom[16] = '\0';

                    taskENTER_CRITICAL();
                    setCursorPositionLCD(0, 0);
                    printLCD(top);
                    setCursorPositionLCD(1, 0);
                    printLCD(bottom);
                    taskEXIT_CRITICAL();
                    break;
                }
                case READING: {
                    int length = strlen(update.message);
                    int topEnd = length;
                    if (topEnd > 15) {
                        topEnd = 15;
                    }

                    memcpy(top, &(update.message), 15);
                    for (int i = topEnd; i < 15; i++) {
                        top[i] = ' ';
                    }
                    top[15] = currentBrightness;
                    top[16] = '\0';

                    taskENTER_CRITICAL();
                    setCursorPositionLCD(0, 0);
                    printLCD(top);
                    taskEXIT_CRITICAL();
                    break;
                }
                case TOP: {
                    int length = strlen(update.message);
                    int topEnd = length;
                    if (topEnd > 16) {
                        topEnd = 16;
                    }

                    memcpy(top, &(update.message), 16);
                    for (int i = topEnd; i < 16; i++) {
                        top[i] = ' ';
                    }
                    top[16] = '\0';

                    taskENTER_CRITICAL();
                    setCursorPositionLCD(0, 0);
                    printLCD(top);
                    taskEXIT_CRITICAL();
                    break;
                }
                case CONTEXT: {
                    int length = strlen(update.message);
                    int bottomEnd = length;
                    if (bottomEnd > 16) {
                        bottomEnd = 16;
                    }

                    memcpy(bottom, &(update.message), 16);
                    for (int i = bottomEnd; i < 16; i++) {
                        bottom[i] = ' ';
                    }
                    bottom[16] = '\0';

                    taskENTER_CRITICAL();
                    setCursorPositionLCD(1, 0);
                    printLCD(bottom);
                    taskEXIT_CRITICAL();
                    break;
                }
                case BRIGHTNESS:
                    setCursorPositionLCD(0, POS_BRIGHTNESS);
                    temp[0] = currentBrightness;
                    temp[1] = '\0';
                    taskENTER_CRITICAL();
                    printLCD(temp);
                    taskEXIT_CRITICAL();
                    break;
                case CLEAR_SCREEN:
                    sprintf(top, "                ");
                    sprintf(bottom, "                ");
                    taskENTER_CRITICAL();
                    setCursorPositionLCD(0, 0);
                    printLCD(top);
                    setCursorPositionLCD(1, 0);
                    printLCD(bottom);
                    taskEXIT_CRITICAL();
                    break;
            }
            memcpy(currentScreen, &top, 16);
            memcpy(&(currentScreen[16]), &bottom, 16);
            currentScreen[32] = '\0';
        }

        vTaskDelay(15 / portTICK_RATE_MS);
    }
}

void GetScreen(char* dest) {
    strcpy(dest, currentScreen);
}

void ScreenTaskInit() {
    UARTprintf("LOG:Screen...\r\n");
    screenQueue = xQueueCreate(SCREEN_QUEUE_SIZE, SCREEN_ITEM_SIZE);

    // Intialise the LCD
    initLCD();

    // Create all the custom characters
    createCharLCD(PLUS_MINUS, charPlusMinus);
    createCharLCD(BRIGHTNESS_1, brightness1);
    createCharLCD(BRIGHTNESS_2, brightness2);
    createCharLCD(BRIGHTNESS_3, brightness3);
    createCharLCD(BRIGHTNESS_4, brightness4);
    createCharLCD(BRIGHTNESS_5, brightness5);
    createCharLCD(BRIGHTNESS_6, brightness6);

    // Clear the screen
    clearLCD();

    // Configure an analog output pin
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    GPIOPinConfigure(GPIO_PF1_M1PWM5);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, period);
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, duty - ((currentBrightness - MIN_BRIGHTNESS) * dutyIncrement));
    PWMGenEnable(PWM1_BASE, PWM_GEN_2);
    PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);

    xTaskCreate(ScreenTask, (const portCHAR*) "Screen", SCREEN_STACK_SIZE, NULL, tskIDLE_PRIORITY + SCREEN_PRIORITY, &screenTask);
    UARTprintf("LOG:Screen Initialised\r\n");
}
