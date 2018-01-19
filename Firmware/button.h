#ifndef __BUTTON_H
#define __BUTTON_H

#define B1_PORT GPIO_PORTC_BASE
#define B1_PIN GPIO_PIN_7
#define B1_SHIFT 7
#define B1_POS 0
#define B2_PORT GPIO_PORTD_BASE
#define B2_PIN GPIO_PIN_6
#define B2_SHIFT 6
#define B2_POS 1
#define B3_PORT GPIO_PORTD_BASE
#define B3_PIN GPIO_PIN_7
#define B3_SHIFT 7
#define B3_POS 2
#define B4_PORT GPIO_PORTF_BASE
#define B4_PIN GPIO_PIN_4
#define B4_SHIFT 4
#define B4_POS 3
#define B5_PORT GPIO_PORTF_BASE
#define B5_PIN GPIO_PIN_2
#define B5_SHIFT 2
#define B5_POS 4
#define B6_PORT GPIO_PORTF_BASE
#define B6_PIN GPIO_PIN_3
#define B6_SHIFT 3
#define B6_POS 5
#define B7_PORT GPIO_PORTB_BASE
#define B7_PIN GPIO_PIN_3
#define B7_SHIFT 3
#define B7_POS 6
#define B8_PORT GPIO_PORTC_BASE
#define B8_PIN GPIO_PIN_4
#define B8_SHIFT 4
#define B8_POS 7
#define B9_PORT GPIO_PORTC_BASE
#define B9_PIN GPIO_PIN_5
#define B9_SHIFT 5
#define B9_POS 8
#define B10_PORT GPIO_PORTC_BASE
#define B10_PIN GPIO_PIN_6
#define B10_SHIFT 6
#define B10_POS 9

#define BUTTON_ITEM_SIZE sizeof(ButtonType)
#define BUTTON_QUEUE_SIZE 50

void ButtonTaskInit();

#endif
