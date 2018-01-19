#ifndef __SYSTEM_H
#define __SYSTEM_H

#define LED2_PORT GPIO_PORTE_BASE
#define LED2_PIN GPIO_PIN_2

#define LOGGING_BLINK_INTERVAL 500
#define STANDBY_BLINK_INTERVAL 3000

extern void SystemTaskInit();
extern void PrintSystem(const char *pcString, ...);
extern void PrintLog(const char *pcString, ...);
extern void PrintData(const char *pcString, ...);

#endif
