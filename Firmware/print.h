#ifndef __PRINT_H
#define __PRINT_H

#include <stdarg.h>

#define TAG_SYSTEM "SYSTEM:"
#define TAG_DATA "DATA:"
#define TAG_LOG "LOG:"

void PrintSystem(const char*, ...);
void PrintData(const char*, ...);
void PrintLog(const char*, ...);

#endif
