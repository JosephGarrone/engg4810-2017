#ifndef __DEFINES_H
#define __DEFINES_H

// Task stack sizes
#define INPUT_STACK_SIZE 512
#define SCREEN_STACK_SIZE 512
#define BUTTON_STACK_SIZE 128
#define ADC_STACK_SIZE 512
#define ADC_NOTIFICATION_STACK_SIZE 128
#define SYSTEM_STACK_SIZE 768
#define VOLTAGE_STACK_SIZE 640
#define SYS_BLINK_STACK_SIZE 128

// Task priorities
#define INPUT_PRIORITY 2
#define SCREEN_PRIORITY 4
#define BUTTON_PRIORITY 2
#define ADC_PRIORITY 2
#define ADC_NOTIFICATION_PRIORITY 2
#define SYSTEM_PRIORITY 2
#define SYS_BLINK_PRIORITY 2

// General defines
#define BUFFER_SIZE 128
#define HIGH 1
#define LOW 0
#define LED_BLINK_DURATION 100

// Clock defines
#define CLOCK_FREQ 50000000 // 50MHz Clock
#define PWM_DIVIDER 64 // PWM Divider
#define LED_FREQ 10000 // 10kHz LED flicker

#endif
