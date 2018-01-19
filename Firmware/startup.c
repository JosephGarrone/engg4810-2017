#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "third_party/TivaLCD/display.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/fpu.h"
#include "drivers/rgb.h"
#include "drivers/ads124x.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "system.h"
#include "FreeRTOS.h"
#include "customstdio.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "defines.h"
#include "spi.h"
#include "relay.h"

// High frequency timer
volatile unsigned long ulHighFrequencyTimerTicks = 0UL;
#define timerINTERRUPT_FREQUENCY ( 20000UL ) //20khz timer

// Capture overflow
void vApplicationStackOverflowHook(xTaskHandle *pxTask, char *pcTaskName)
{
    while(1)
    {
        UARTprintf("Overflow detected for %s\r\n", pcTaskName);
    }
}

void ConfigureUART(void)
{
    // Enable UART0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

    // Configure GPIO Pins for UART mode.
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
    ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Use the internal 16MHz oscillator as the UART clock source.
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);

    // Initialize the UART for console I/O.
    UARTStdioConfig(0, 9600, 16000000);
    UARTStdioConfig2(1, 9600, 16000000);
}

void UnlockPins() {
    HWREG(GPIO_PORTA_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTA_BASE + GPIO_O_CR) = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;

    HWREG(GPIO_PORTB_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTB_BASE + GPIO_O_CR) = GPIO_PIN_3;

    HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;

    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = GPIO_PIN_6 | GPIO_PIN_7;

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
}

void StartPeripherals() {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}

void Timer0IntHandler( void )
{
    TimerIntClear( TIMER0_BASE, TIMER_TIMA_TIMEOUT );
    ulHighFrequencyTimerTicks++;
}

void StartHighFrequencyTimer() {
    unsigned long ulFrequency;

    SysCtlPeripheralEnable( SYSCTL_PERIPH_TIMER0 );
    TimerConfigure( TIMER0_BASE, TIMER_CFG_PERIODIC );

    IntPrioritySet( INT_TIMER0A, 0 );

    portDISABLE_INTERRUPTS();

    ulFrequency = configCPU_CLOCK_HZ / timerINTERRUPT_FREQUENCY;
    TimerLoadSet( TIMER0_BASE, TIMER_A, ulFrequency );
    IntEnable( INT_TIMER0A );
    TimerIntEnable( TIMER0_BASE, TIMER_TIMA_TIMEOUT );

    TimerEnable( TIMER0_BASE, TIMER_A );
}

int main(void)
{
    // Enable the FPU
    ROM_FPUEnable();

    // Enable Float use in interrupts
    ROM_FPULazyStackingEnable();

    // Set the clocking to run at 50 MHz from the PLL.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Start all peripheral clocks
    StartPeripherals();

    // Initialize the UART and configure it for 9600, 8-N-1 operation.
    ConfigureUART();

    // Enable all locked pins
    UnlockPins();

    // Initialise the SPI
    ConfigureSPI();

    // Configure the relays
    ConfigureRelays();

    // Start the system
    SystemTaskInit();

    // Start the timer
    StartHighFrequencyTimer();

    // Start the scheduler.  This should not return.
    vTaskStartScheduler();
}
