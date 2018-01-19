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
#include "adc.h"
#include "handlers/general.h"

extern TaskHandle_t adcNotifyTask;
extern TaskHandle_t adcReadTask;

int sampleIndex = 2;
xSemaphoreHandle adcNotificationSemaphore = NULL;
xQueueHandle adcQueue;

void ADCDecreaseSampling() {
    if (sampleIndex - 1 >= 0) {
        sampleIndex--;
    }
}

void ADCIncreaseSampling() {
    if (sampleIndex + 1 < ADC_NUM_RATES) {
        sampleIndex++;
    }
}

uint32_t GetADCDelay() {
    switch (sampleIndex) {
        case 0:
            return (uint32_t) 500;
        case 1:
            return (uint32_t) 1000;
        case 2:
            return (uint32_t) 2000;
        case 3:
            return (uint32_t) 5000;
        case 4:
            return (uint32_t) 10000;
        case 5:
            return (uint32_t) 60000;
        case 6:
            return (uint32_t) 120000;
        case 7:
            return (uint32_t) 300000;
        case 8:
            return (uint32_t) 600000;
    }
    return 1000; // Shouldn't ever hit this
}

void ADCNotificationTask(void *pvParams) {
    uint32_t lastTime = 0;
    uint32_t currentTime = xTaskGetTickCount();
    bool isHigh = false;

    while (true) {
        currentTime = xTaskGetTickCount();
        if (currentTime - lastTime > LED_BLINK_DURATION && isHigh) {
            GPIOPinWrite(LED1_PORT, LED1_PIN, LOW);
            isHigh = false;
        }
        if (xSemaphoreTake(adcNotificationSemaphore, 0) == pdTRUE) {
            GPIOPinWrite(LED1_PORT, LED1_PIN, LED1_PIN);
            lastTime = currentTime;
            isHigh = true;
        }
        vTaskDelay(25 / portTICK_RATE_MS);
    }
}

void ADCTask(void *pvParams) {
    int32_t adcValue;
    uint32_t lastTime = 0;
    uint32_t lastNonSampledTime = 0;
    uint32_t currentTime = xTaskGetTickCount();
    ADCRawReading raw;

    //ADCProcessorTrigger(ADC0_BASE, 3);

    while (true) {
        //if(ADCIntStatus(ADC0_BASE, 3, false) != false)
        //{
            //ADCIntClear(ADC0_BASE, 3);
            //ADCSequenceDataGet(ADC0_BASE, 3, adcValue);
        adcValue = ADCGetConversion();
        currentTime = xTaskGetTickCount();
        if ((currentTime - lastTime) / portTICK_RATE_MS > GetADCDelay()) {
            raw.adcValue = adcValue;
            raw.isSample = true;
            xSemaphoreGive(adcNotificationSemaphore);
            xQueueSend(adcQueue, &raw, 50);
            PrintLog("Got ADC value: %4d\r\n", adcValue);
            lastTime = currentTime;
            lastNonSampledTime = currentTime;
        } else if ((currentTime - lastNonSampledTime) / portTICK_RATE_MS > 500) {
            raw.adcValue = adcValue;
            raw.isSample = false;
            xQueueSend(adcQueue, &raw, 50);
            lastNonSampledTime = currentTime;
        }
            //ADCProcessorTrigger(ADC0_BASE, 3);
        //}
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void ADCTaskInit() {
    UARTprintf("LOG:ADC...\r\n");

    /*SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2);
    ADCHardwareOversampleConfigure(ADC0_BASE, 64); // 64x hardware oversampling
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE |
                                 ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);*/
    ConfigureADC();

    // Set up sampling LED
    GPIOPinTypeGPIOOutput(LED1_PORT, LED1_PIN);
    GPIOPinWrite(LED1_PORT, LED1_PIN, LED1_PIN);

    UARTprintf("LOG:Creating locks\r\n");
    adcNotificationSemaphore = xSemaphoreCreateMutex();
    adcQueue = xQueueCreate(ADC_QUEUE_SIZE, ADC_ITEM_SIZE);
    UARTprintf("LOG:Created locks\r\n");

    xTaskCreate(ADCNotificationTask, (const portCHAR*) "ADCNotify", ADC_NOTIFICATION_STACK_SIZE, NULL, tskIDLE_PRIORITY + ADC_NOTIFICATION_PRIORITY, &adcNotifyTask);
    xTaskCreate(ADCTask, (const portCHAR*) "ADCRead", ADC_STACK_SIZE, NULL, tskIDLE_PRIORITY + ADC_PRIORITY, &adcReadTask);

    UARTprintf("LOG:ADC Initialised...\r\n");
}
