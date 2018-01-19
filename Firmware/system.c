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
#include "handlers/general.h"
#include "handlers/voltage.h"
#include "handlers/current.h"
#include "handlers/resistance.h"
#include "handlers/cont.h"
#include "handlers/logic.h"
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
#include "adc.h"
#include "input.h"
#include "relay.h"

extern xQueueHandle buttonQueue;
extern xQueueHandle adcQueue;
extern xQueueHandle screenQueue;

// Task handles
TaskHandle_t inputTask;
TaskHandle_t inputOptTask;
TaskHandle_t buttonTask;
TaskHandle_t adcNotifyTask;
TaskHandle_t adcReadTask;
TaskHandle_t systemTask;
TaskHandle_t blinkTask;
TaskHandle_t screenTask;

// UART
xSemaphoreHandle uartSemaphore;

// SPI
xSemaphoreHandle spiSemaphore;

static char localScreen[33];
SystemState state = STATE_ON;
int subLocation = 0;

// Menu
SystemLocation MENU_LOCS[] = {
    {LOC_MEASURE, GenericLoad, GenericUnload},
    {LOC_SETTINGS, GenericLoad, GenericUnload},
    {LOC_LOGGING, GenericLoad, GenericUnload},
    {LOC_POWER, GenericLoad, GenericUnload},
    {LOC_RELAYS, GenericLoad, GenericUnload}
};
SystemLocation MEASURE_LOCS[] = {
    {LOC_VOLTAGE_DC, VoltageDCLoad, VoltageDCUnload},
    {LOC_VOLTAGE_AC, VoltageACLoad, VoltageACUnload},
    {LOC_CURRENT, CurrentDCLoad, CurrentDCUnload},
    {LOC_RESISTANCE, GenericLoad, GenericUnload},
    {LOC_CONTINUITY, GenericLoad, GenericUnload},
    {LOC_LOGIC, LogicLoad, LogicUnload}
};
SystemLocation SETTING_LOCS[] = {
    {LOC_SAMPLE, GenericLoad, GenericUnload},
    {LOC_BRIGHTNESS, GenericLoad, GenericUnload}
};
SystemLocation LOGGING_LOCS[] = {
    {LOC_MEASUREMENT, GenericLoad, GenericUnload},
    {LOC_LOG_SAMPLE, GenericLoad, GenericUnload},
    {LOC_NUM_SAMPLES, GenericLoad, GenericUnload},
    {LOC_LOG_PERIOD, GenericLoad, GenericUnload}
};
SystemLocation POWER_LOCS[] = {
    {LOC_STANDBY, GenericLoad, GenericUnload}
};
SystemLocation RELAY_LOCS[] = {
    {LOC_RELAYA, GenericLoad, GenericUnload},
    {LOC_RELAYB, GenericLoad, GenericUnload},
    {LOC_RELAYC, GenericLoad, GenericUnload}//,
    //{LOC_RELAYD, GenericLoad, GenericUnload},
    //{LOC_RELAYE, GenericLoad, GenericUnload}
};
SystemLocation LOCATION_SPLASH = {LOC_SPLASH, GenericLoad, GenericUnload};
SystemLocation LOCATION_MENU = {LOC_MENU, GenericLoad, GenericUnload};
SystemLocation LOCATION_MEASURE = {LOC_MEASURE, GenericLoad, GenericUnload};
SystemLocation LOCATION_SETTING = {LOC_SETTINGS, GenericLoad, GenericUnload};
SystemLocation LOCATION_LOGGING = {LOC_LOGGING, GenericLoad, GenericUnload};
SystemLocation LOCATION_POWER = {LOC_POWER, GenericLoad, GenericUnload};
SystemLocation LOCATION_RELAYS = {LOC_RELAYS, GenericLoad, GenericUnload};
SystemLocation LOCATION_NULL = {LOC_NULL, GenericLoad, GenericUnload};

SystemLocation location = {LOC_SPLASH, GenericLoad, GenericUnload};

void SystemBlinkTask(void *pvParams) {
    uint32_t lastTime = 0;
    uint32_t currentTime = xTaskGetTickCount();
    bool isHigh = true;

    while (true) {
        currentTime = xTaskGetTickCount();

        switch (state) {
            case STATE_ON:
                GPIOPinWrite(LED2_PORT, LED2_PIN, LED2_PIN);
                lastTime = currentTime;
                isHigh = true;
                break;
            case STATE_ON_LOGGING:
                if ((currentTime - lastTime) / portTICK_RATE_MS >= LED_BLINK_DURATION && isHigh) {
                    GPIOPinWrite(LED2_PORT, LED2_PIN, LOW);
                    isHigh = false;
                }
                if ((currentTime - lastTime) / portTICK_RATE_MS >= LOGGING_BLINK_INTERVAL) {
                    GPIOPinWrite(LED2_PORT, LED2_PIN, LED2_PIN);
                    lastTime = currentTime;
                    isHigh = true;
                }
                break;
            case STATE_STANDBY:
                if ((currentTime - lastTime) / portTICK_RATE_MS >= LED_BLINK_DURATION && isHigh) {
                    GPIOPinWrite(LED2_PORT, LED2_PIN, LOW);
                    isHigh = false;
                }
                if ((currentTime - lastTime) / portTICK_RATE_MS >= STANDBY_BLINK_INTERVAL) {
                    GPIOPinWrite(LED2_PORT, LED2_PIN, LED2_PIN);
                    lastTime = currentTime;
                    isHigh = true;
                }
                break;
        }

        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

bool SystemChangeLocation(SystemLocation loc) {
    // Unload previous location
    location.onUnload();

    // Store new location
    location = loc;

    // Initialise new location
    loc.onLoad();

    // Reset the sub location
    subLocation = 0;

    // Return true (For a reRun)
    return true;
}

bool SystemHandleButtonAction(ButtonType button, SystemLocation *locs, SystemLocation back, int arraySize) {
    int locMin = 0;
    int locMax = arraySize / sizeof(SystemLocation);
    bool reRun = false;

    switch (button) {
        case B1:
            if (back.location != LOC_NULL) {
                reRun = SystemChangeLocation(back);
            }
            break;
        case B2:
            subLocation = (subLocation > 1) ? subLocation - 1 : locMin;
            break;
        case B3:
            subLocation = (subLocation < locMax - 1) ? subLocation + 1 : locMax - 1;
            break;
        case B4:
            reRun = SystemChangeLocation(locs[subLocation]);
            break;
    }

    return reRun;
}

void SystemHandleButton(ButtonType button) {
    ScreenUpdate update;
    ScreenUpdate brightnessUpdate;
    bool reRun = true;
    bool updateScreen = true;

    if (button == NO_BUTTON) {
        return;
    }

    update.type = ENTIRE_SCREEN;

    while (reRun) {
        PrintLog("At location %d\r\n", location.location);
        reRun = false;
        switch (location.location) {
            case LOC_SPLASH: // Handled elsewhere
                break;
            case LOC_MENU:
                reRun = SystemHandleButtonAction(button, MENU_LOCS, LOCATION_NULL, sizeof(MENU_LOCS));

                // Drawing logic
                switch (subLocation) {
                    case 0:
                        ScreenTop(update.message, "  [MEASURE]    >");
                        break;
                    case 1:
                        ScreenTop(update.message, "< [SETTINGS]   >");
                        break;
                    case 2:
                        ScreenTop(update.message, "< [LOGGING]    >");
                        break;
                    case 3:
                        ScreenTop(update.message, "< [POWER]      >");
                        break;
                    case 4:
                        ScreenTop(update.message, "< [RELAYS]      ");
                        break;
                }
                ScreenBot(update.message, "     \x7F    \x7E  SEL");
                break;
            case LOC_MEASURE:
                reRun = SystemHandleButtonAction(button, MEASURE_LOCS, LOCATION_MENU, sizeof(MEASURE_LOCS));

                // Drawing logic
                switch (subLocation) {
                    case 0:
                        ScreenTop(update.message, "  [VOLTAGE DC] >");
                        break;
                    case 1:
                        ScreenTop(update.message, "< [VOLTAGE AC] >");
                        break;
                    case 2:
                        ScreenTop(update.message, "< [CURRENT]    >");
                        break;
                    case 3:
                        ScreenTop(update.message, "< [RESISTANCE] >");
                        break;
                    case 4:
                        ScreenTop(update.message, "< [CONTINUITY] >");
                        break;
                    case 5:
                        ScreenTop(update.message, "< [LOGIC]");
                        break;
                }
                ScreenBot(update.message, "BCK  \x7F    \x7E  SEL");
                break;
            case LOC_VOLTAGE_DC:
            case LOC_VOLTAGE_AC:
            case LOC_CURRENT:
            case LOC_RESISTANCE:
            case LOC_CONTINUITY:
            case LOC_LOGIC:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_MEASURE);
                        break;
                    case B2:
                        ADCDecreaseSampling();
                        break;
                    case B3:
                        ADCIncreaseSampling();
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            case LOC_SETTINGS:
                reRun = SystemHandleButtonAction(button, SETTING_LOCS, LOCATION_MENU, sizeof(SETTING_LOCS));

                // Drawing logic
                switch (subLocation) {
                    case 0:
                        ScreenTop(update.message, "  [SAMPL RATE] >");
                        break;
                    case 1:
                        ScreenTop(update.message, "< [BRIGHTNESS]  ");
                        break;
                }
                ScreenBot(update.message, "BCK  \x7F    \x7E  SEL");
                break;
            case LOC_SAMPLE:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_SETTING);
                        break;
                    case B2:
                        ADCDecreaseSampling();
                        break;
                    case B3:
                        ADCIncreaseSampling();
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            case LOC_BRIGHTNESS:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_SETTING);
                        break;
                    case B2:
                        ScreenDecreaseBrightness(brightnessUpdate);
                        break;
                    case B3:
                        ScreenIncreaseBrightness(brightnessUpdate);
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            case LOC_LOGGING:
                reRun = SystemHandleButtonAction(button, LOGGING_LOCS, LOCATION_MENU, sizeof(LOGGING_LOCS));

                // Drawing logic
                switch (subLocation) {
                    case 0:
                        ScreenTop(update.message, "  [MEASUREMNT] >");
                        break;
                    case 1:
                        ScreenTop(update.message, "< [SAMPL RATE] >");
                        break;
                    case 2:
                        ScreenTop(update.message, "< [# OF SMPLS] >");
                        break;
                    case 3:
                        ScreenTop(update.message, "< [LOG PERIOD]  ");
                        break;
                }
                ScreenBot(update.message, "BCK  \x7F    \x7E  SEL");
                break;
            case LOC_POWER:
                reRun = SystemHandleButtonAction(button, POWER_LOCS, LOCATION_MENU, sizeof(POWER_LOCS));

                // Drawing logic
                switch (subLocation) {
                    case 0:
                        ScreenTop(update.message, "  [ENTER STBY]  ");
                        break;
                }
                ScreenBot(update.message, "BCK          SEL");
                break;
            case LOC_STANDBY:
                state = STATE_STANDBY;
                break;
            case LOC_RELAYS:
                reRun = SystemHandleButtonAction(button, RELAY_LOCS, LOCATION_MENU, sizeof(RELAY_LOCS));

                switch (subLocation) {
                    case 0:
                        ScreenTop(update.message, "  [RELAY A]    >");
                        break;
                    case 1:
                        ScreenTop(update.message, "< [RELAY B]    >");
                        break;
                    case 2:
                        ScreenTop(update.message, "< [RELAY C]     ");
                        break;
                    case 3:
                        ScreenTop(update.message, "< [RELAY D]    >");
                        break;
                    case 4:
                        ScreenTop(update.message, "< [RELAY E]     ");
                        break;
                }

                ScreenBot(update.message, "BCK  \x7F    \x7E  SEL");
                break;
            case LOC_RELAYA:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_RELAYS);
                        break;
                    case B2:
                        SetRelay(RELAY_A);
                        break;
                    case B3:
                        ResetRelay(RELAY_A);
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            case LOC_RELAYB:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_RELAYS);
                        break;
                    case B2:
                        SetRelay(RELAY_B);
                        break;
                    case B3:
                        ResetRelay(RELAY_B);
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            case LOC_RELAYC:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_RELAYS);
                        break;
                    case B2:
                        SetRelay(RELAY_C);
                        break;
                    case B3:
                        ResetRelay(RELAY_C);
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            case LOC_RELAYD:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_RELAYS);
                        break;
                    case B2:
                        SetRelay(RELAY_D);
                        break;
                    case B3:
                        ResetRelay(RELAY_D);
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            case LOC_RELAYE:
                switch (button) {
                    case B1:
                        reRun = SystemChangeLocation(LOCATION_RELAYS);
                        break;
                    case B2:
                        SetRelay(RELAY_E);
                        break;
                    case B3:
                        ResetRelay(RELAY_E);
                        break;
                    case B4:
                        // Do nothing
                        break;
                }
                break;
            default:
                updateScreen = false;
                break;
        }

        // Update the screen
        if (updateScreen) {
            PrintLog("%s\r\n", update.message);
            xQueueSend(screenQueue, &update, 0);
        }

        // Ensure buttons aren't processed more than once
        button = NO_BUTTON;
    }
}

void PrintSettingScreen() {
    ScreenUpdate screen;
    screen.type = TOP;
    uint32_t delay = GetADCDelay();
    char buffer[10];
    int brightness = GetBrightness();
    int i = 0;

    switch (location.location) {
        case LOC_SAMPLE:
            if (delay >= 600000) {
                sprintf(screen.message, "Sample:   %3dm  ", delay / 60000);
            } else if (delay >= 60000) {
                sprintf(screen.message, "Sample:   %1d.%1dm  ", delay / 60000, (delay % 60000) / 6000);
            } else if (delay >= 10000) {
                sprintf(screen.message, "Sample:   %3ds  ", delay / 1000);
            } else {
                sprintf(screen.message, "Sample:   %1d.%1ds  ", delay / 1000, (delay % 1000) / 100);
            }
            break;
        case LOC_BRIGHTNESS:
            for (i = 0; i < brightness + 1; i++) {
                buffer[i] = 0b11111111;
            }
            for (; i < 6; i++) {
                buffer[i] = ' ';
            }
            buffer[i] = '\0';

            sprintf(screen.message, "Brght: [%s]", buffer);
            break;
    }
    xQueueSend(screenQueue, &screen, 50);
}

void PrintSettingContext() {
    ScreenUpdate screen;
    screen.type = CONTEXT;

    switch (location.location) {
        case LOC_SAMPLE:
            sprintf(screen.message, "BCK  S-  S+     ");
            break;
        case LOC_BRIGHTNESS:
            sprintf(screen.message, "BCK  B-  B+     ");
            break;
    }
    xQueueSend(screenQueue, &screen, 50);
}

void PrintMeasureContext() {
    ScreenUpdate screen;
    screen.type = CONTEXT;
    uint32_t delay = GetADCDelay();

    if (delay >= 600000) {
        sprintf(screen.message, "BCK  S-  S+ %3dm", delay / 60000);
    } else if (delay >= 60000) {
        sprintf(screen.message, "BCK  S-  S+ %1d.%1dm", delay / 60000, (delay % 60000) / 6000);
    } else if (delay >= 10000) {
        sprintf(screen.message, "BCK  S-  S+ %3ds", delay / 1000);
    } else {
        sprintf(screen.message, "BCK  S-  S+ %1d.%1ds", delay / 1000, (delay % 1000) / 100);
    }
    xQueueSend(screenQueue, &screen, 50);
}

void PrintRelayReading(Relay relay) {
    ScreenUpdate screen;
    screen.type = TOP;

    bool state = GetRelayState(relay);

    switch (relay) {
        case RELAY_A:
            if (state) {
                sprintf(screen.message, "RELAY A: RESET  ");
            } else {
                sprintf(screen.message, "RELAY A: SET    ");
            }
            break;
        case RELAY_B:
            if (state) {
                sprintf(screen.message, "RELAY B: RESET  ");
            } else {
                sprintf(screen.message, "RELAY B: SET    ");
            }
            break;
        case RELAY_C:
            if (state) {
                sprintf(screen.message, "RELAY C: RESET  ");
            } else {
                sprintf(screen.message, "RELAY C: SET    ");
            }
            break;
        case RELAY_D:
            if (state) {
                sprintf(screen.message, "RELAY D: RESET  ");
            } else {
                sprintf(screen.message, "RELAY D: SET    ");
            }
            break;
        case RELAY_E:
            if (state) {
                sprintf(screen.message, "RELAY E: RESET  ");
            } else {
                sprintf(screen.message, "RELAY E: SET    ");
            }
            break;
    }

    xQueueSend(screenQueue, &screen, 50);
}

void PrintRelayContext() {
    ScreenUpdate screen;
    screen.type = CONTEXT;

    sprintf(screen.message, "BCK  SET  RST   ");

    xQueueSend(screenQueue, &screen, 50);
}

void SystemTask(void *pvParams) {
    ButtonType button = NO_BUTTON;
    ScreenUpdate screen;
    ADCReading reading;
    ADCRawReading raw;
    uint32_t lastTime = 0;
    uint32_t currentTime = xTaskGetTickCount();
    SystemState prevState = STATE_ON;

    // Create a mutex to guard the UART.
    uartSemaphore = xSemaphoreCreateMutex();

    // Create a mutix to guard the SPI
    spiSemaphore = xSemaphoreCreateMutex();

    // Print intro
    PrintLog("ENGG4810 - Team Project II\r\n");
    PrintLog("By Harry Brown, Patricia Chen, Joseph Garrone, Anant Tuli\r\n");

    while (true) {
        // Get the pressed button if any
        if (xQueueReceive(buttonQueue, &button, 0) != pdPASS) {
            button = NO_BUTTON;
        } else {
            PrintLog("Got button %d\r\n", button + 1);
        }

        if (state == STATE_STANDBY) {
            if (prevState != STATE_STANDBY) {
                // Suspend everything
                vTaskSuspend(inputTask);
                vTaskSuspend(inputOptTask);
                //vTaskSuspend(adcNotifyTask);
                //vTaskSuspend(adcReadTask);
                vTaskSuspend(screenTask);

                ScreenOff();
            }

            prevState = state;

            if (button == B1) {
                state = STATE_ON;
            }

            vTaskDelay(10 / portTICK_RATE_MS);
            continue;
        } else if ((state == STATE_ON || state == STATE_ON_LOGGING) && prevState == STATE_STANDBY) {
            // Resume everything
            vTaskResume(inputTask);
            vTaskResume(inputOptTask);
            //vTaskResume(adcNotifyTask);
            //vTaskResume(adcReadTask);
            vTaskResume(screenTask);

            ScreenRestore();

            SystemChangeLocation(LOCATION_POWER);
            button = NO_BUTTON;
        }

        prevState = state;

        // Handle buttons
        SystemHandleButton(button);

        // Attempt to receive an ADC reading
        if (xQueueReceive(adcQueue, &raw, 0) == pdPASS) {
            reading.adcValue = raw.adcValue;
            reading.newData = raw.isSample;
            reading.newSample = true;
        }

        // Handle current screen
        switch (location.location) {
            case LOC_SPLASH:
                vTaskDelay(1000 / portTICK_RATE_MS); // 1 second splash
                location = LOCATION_MENU;
                ScreenClear(screen);
                SystemHandleButton(B1);
                break;
            case LOC_VOLTAGE_DC:
                HandleDCReading(reading);
                PrintMeasureContext();
                break;
            case LOC_VOLTAGE_AC:
                HandleACReading(reading);
                PrintMeasureContext();
                break;
            case LOC_CURRENT:
                HandleCurrentReading(reading);
                PrintMeasureContext();
                break;
            case LOC_RESISTANCE:
                HandleResistanceReading(reading);
                PrintMeasureContext();
                break;
            case LOC_CONTINUITY:
                HandleContReading(reading);
                PrintMeasureContext();
                break;
            case LOC_LOGIC:
                HandleLogicReading(reading);
                PrintMeasureContext();
                break;
            case LOC_SAMPLE:
                PrintSettingScreen();
                PrintSettingContext();
                break;
            case LOC_BRIGHTNESS:
                PrintSettingScreen();
                PrintSettingContext();
                break;
            case LOC_MEASUREMENT:
                //HandleLogicReading(reading);
                break;
            case LOC_LOG_SAMPLE:
                //HandleLogicReading(reading);
                break;
            case LOC_NUM_SAMPLES:
                //HandleLogicReading(reading);
                break;
            case LOC_LOG_PERIOD:
                //HandleLogicReading(reading);
                break;
            case LOC_STANDBY:
                //HandleLogicReading(reading);
                break;
            case LOC_RELAYA:
                PrintRelayReading(RELAY_A);
                PrintRelayContext();
                break;
            case LOC_RELAYB:
                PrintRelayReading(RELAY_B);
                PrintRelayContext();
                break;
            case LOC_RELAYC:
                PrintRelayReading(RELAY_C);
                PrintRelayContext();
                break;
            case LOC_RELAYD:
                PrintRelayReading(RELAY_D);
                PrintRelayContext();
                break;
            case LOC_RELAYE:
                PrintRelayReading(RELAY_E);
                PrintRelayContext();
                break;
        }

        uint32_t currentTime = xTaskGetTickCount();
        if (currentTime - lastTime > 500 || reading.newData) {
            GetScreen(localScreen);
            if (strlen(localScreen) >= 32) {
                PrintSystem("%s\r\n", localScreen);
            }
            lastTime = currentTime;
        }

        reading.newData = false;

        vTaskDelay(10 / portTICK_RATE_MS); // Delay 33ms, don't need to refresh too often
    }
}

void SystemTaskInit() {
    UARTprintf("LOG:SYSTEM...\r\n");

    buttonQueue = xQueueCreate(BUTTON_QUEUE_SIZE, BUTTON_ITEM_SIZE);

    // Set up system state LED
    GPIOPinTypeGPIOOutput(LED2_PORT, LED2_PIN);
    GPIOPadConfigSet(B1_PORT, B1_PIN, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(LED2_PORT, LED2_PIN, LED2_PIN);

    // Setup logic comparator pin
    GPIOPinTypeGPIOOutput(COMP_PORT, COMP_PIN);
    GPIOPinWrite(COMP_PORT, COMP_PIN, LOW);

    xTaskCreate(SystemTask, (const portCHAR*) "System", SYSTEM_STACK_SIZE, NULL, tskIDLE_PRIORITY + SYSTEM_PRIORITY, &systemTask);
    xTaskCreate(SystemBlinkTask, (const portCHAR*) "SysBlink", SYS_BLINK_STACK_SIZE, NULL, tskIDLE_PRIORITY + SYS_BLINK_PRIORITY, &blinkTask);

    ButtonTaskInit();
    BaseHandlerInit();
    InputTaskInit();
    ScreenTaskInit();
    ADCTaskInit();

    UARTprintf("LOG:SYSTEM Initialised...\r\n");
}
