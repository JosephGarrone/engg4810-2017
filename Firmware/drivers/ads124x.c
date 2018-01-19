#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "driverlib/rom.h"
#include "driverlib/pwm.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "spi.h"
#include "ads124x.h"
#include "defines.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "print.h"
#include "utils.h"

int DRDY_State = HIGH;

uint8_t MUX0_val;
uint8_t VBIAS_val;
uint8_t MUX1_val;
uint8_t SYS0_val;
uint8_t IDAC0_val;
uint8_t IDAC1_val;

extern xSemaphoreHandle spiSemaphore;

// Waits until DRDY Pin is falling (see Interrupt setup).
// Some commands like WREG, RREG need the DRDY to be low.
void WaitForDRDY() {
    while (DRDY_State) {
        continue;
    }

    taskENTER_CRITICAL();
    DRDY_State = HIGH;
    taskEXIT_CRITICAL();
}

void DRDYInterruptHandler(void) {
    uint32_t status = 0;

    status = GPIOIntStatus(DRDY_PORT, true);
    GPIOIntClear(DRDY_PORT, status);

    if((status & DRDY_PIN_INT) == DRDY_PIN_INT){
        DRDY_State = LOW;
    }
    DRDY_State = LOW;
}

void ConfigureDRDYInterrupt() {
    GPIOIntTypeSet(DRDY_PORT, DRDY, GPIO_FALLING_EDGE);
    GPIOIntRegister(DRDY_PORT, DRDYInterruptHandler);
    GPIOIntEnable(DRDY_PORT, DRDY_PIN_INT);
}

void ResetHigh() {
    GPIOPinWrite(RESET_PORT, RESET, RESET);
}

void ResetLow() {
    GPIOPinWrite(RESET_PORT, RESET, LOW);
}

void CSHigh() {
    DeselectADC();
    //GPIOPinWrite(CS_PORT, CS, CS);
}

void CSLow() {
    SelectADC();
    //GPIOPinWrite(CS_PORT, CS, LOW);
}

void StartHigh() {
    GPIOPinWrite(START_PORT, START, START);
}

void StartLow() {
    GPIOPinWrite(START_PORT, START, LOW);
}

void DRDYHigh() {
    GPIOPinWrite(DRDY_PORT, DRDY, DRDY);
}

void DRDYLow() {
    GPIOPinWrite(DRDY_PORT, DRDY, LOW);
}

uint8_t DRDYState() {
    return GPIOPinRead(DRDY_PORT, DRDY);
}

void AwaitDRDY() {
    while (DRDYState()) {
        continue;
    }
    DelayUSec(10);
    while (!DRDYState()) {
        continue;
    }
}

void ADCCalibrate() {
    ADCSendCMD(SYSOCAL);
    DelayMSec(10);
    ADCSendCMD(SELFOCAL);
    DelayMSec(10);
    ADCSendCMD(SYSGCAL);
    DelayMSec(10);
}

void ConfigureADC() {
    if (spiSemaphore == NULL || xSemaphoreTake(spiSemaphore, 0) == pdTRUE) {
        GPIOPinTypeGPIOOutput(CS_PORT, CS);
        GPIOPinTypeGPIOInput(DRDY_PORT, DRDY);
        GPIOPinTypeGPIOOutput(START_PORT, START);
        GPIOPinTypeGPIOOutput(RESET_PORT, RESET);

        ResetHigh();
        CSHigh();
        StartHigh();

        DelayUSec(10);

        ADCReset();

        DelayUSec(100);

        ADCSetRegister(SYS0, DOR3_2000);

        DelayMSec(1);

        ADCCalibrate();

        if (spiSemaphore != NULL) {
            xSemaphoreGive(spiSemaphore);
        }
    }
}


int32_t ADCGetConversion() {
    int32_t data = 0;
    uint32_t temp;

    if (spiSemaphore == NULL || xSemaphoreTake(spiSemaphore, 0) == pdTRUE) {
        AwaitDRDY();

        CSLow();

        SSIDataPut(SSI0_BASE, RDATA);

        DelayUSec(10);

        // Remove garbage from the FIFO
        ClearSPIIn();

        SSIDataPut(SSI0_BASE, NOP);
        SSIDataPut(SSI0_BASE, NOP);
        SSIDataPut(SSI0_BASE, NOP);

        // Wait until SSI is done txing
        ClearSPIOut();

        SSIDataGet(SSI0_BASE, &temp);
        data |= temp;
        data <<= 8;
        SSIDataGet(SSI0_BASE, &temp);
        data |= temp;
        data <<= 8;
        SSIDataGet(SSI0_BASE, &temp);
        data |= temp;

        CSHigh();

        if (data & (1 << 23)) {
            data |= 0xFF000000;
        }

        if (spiSemaphore != NULL) {
            xSemaphoreGive(spiSemaphore);
        }
    }

    return data;
}


// function to write a register value to the adc
void ADCSetRegister(uint8_t regAddress, uint8_t regValue) {
    uint8_t regValuePre = ADCReadRegister(regAddress);

    if (regValue != regValuePre && (spiSemaphore == NULL || xSemaphoreTake(spiSemaphore, 0) == pdTRUE)) {

        CSLow();

        AwaitDRDY();

        SSIDataPut(SSI0_BASE, SDATAC);

        ClearSPIOut();

        DelayUSec(10);

        // Remove garbage from the FIFO
        ClearSPIIn();

        SSIDataPut(SSI0_BASE, WREG | regAddress);
        SSIDataPut(SSI0_BASE, 0x00);
        SSIDataPut(SSI0_BASE, regValue);

        // Wait until SSI is done txing
        ClearSPIOut();

        DelayUSec(10);

        SSIDataPut(SSI0_BASE, RDATAC);

        ClearSPIOut();

        ClearSPIIn();

        CSHigh();

        if (regValue != ADCReadRegister(regAddress)) {
            PrintLog("Failed to write 0x%02X to 0x%02X, still 0x%02X\r\n", regValue, regAddress, ADCReadRegister(regAddress));
        }

        if (spiSemaphore != NULL) {
            xSemaphoreGive(spiSemaphore);
        }
    }
}


//function to read a register value from the adc
//argument: adress for the register to read
uint32_t ADCReadRegister(uint8_t regAddress) {
    uint32_t buffer = 0;

    if (spiSemaphore == NULL || xSemaphoreTake(spiSemaphore, 0) == pdTRUE) {
        CSLow();

        AwaitDRDY();

        DelayUSec(10);

        // Remove garbage from the FIFO
        ClearSPIIn();

        SSIDataPut(SSI0_BASE, RREG | regAddress);
        SSIDataPut(SSI0_BASE, 0x00);

        ClearSPIOut();

        DelayUSec(10);

        ClearSPIIn();

        SSIDataPut(SSI0_BASE, NOP);

        DelayUSec(10);

        // Wait until SSI is done txing
        ClearSPIOut();

        SSIDataGet(SSI0_BASE, &buffer);

        DelayUSec(10);

        CSHigh();

        PrintLog("Read 0x%02X = 0x%02X\r\n", regAddress, buffer);

        if (spiSemaphore != NULL) {
            xSemaphoreGive(spiSemaphore);
        }
    }

    return buffer;
}

/*
Sends a Command to the ADC
Like SELFCAL, GAIN, SYNC, WAKEUP
*/
void ADCSendCMD(uint8_t cmd) {
    if (spiSemaphore != NULL && xSemaphoreTake(spiSemaphore, 0) == pdTRUE) {
        AwaitDRDY();

        CSLow();

        DelayUSec(10);

        SSIDataPut(SSI0_BASE, cmd);

        DelayUSec(10);

        CSHigh();

        if (spiSemaphore != NULL) {
            xSemaphoreGive(spiSemaphore);
        }
    }
}


// function to reset the adc
void ADCReset() {
    if (spiSemaphore != NULL && xSemaphoreTake(spiSemaphore, 0) == pdTRUE) {
        CSLow();

        DelayUSec(10);

        SSIDataPut(SSI0_BASE, RESET_ADC);

        DelayMSec(2);

        SSIDataPut(SSI0_BASE, SDATAC);

        DelayUSec(100);

        CSHigh();

        if (spiSemaphore != NULL) {
            xSemaphoreGive(spiSemaphore);
        }
    }
}

