#ifndef __CUSTOMSTDIO_H__
#define __CUSTOMSTDIO_H__

#include <stdarg.h>

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void UARTStdioConfig(uint32_t ui32Port, uint32_t ui32Baud,
                            uint32_t ui32SrcClock);
extern void UARTStdioConfig2(uint32_t ui32Port, uint32_t ui32Baud,
                            uint32_t ui32SrcClock);
extern int UARTgets(char *pcBuf, uint32_t ui32Len);
extern int UARTgets2(char *pcBuf, uint32_t ui32Len);
extern char UARTgetc(void);
extern char UARTgetc2(void);
extern char UARTcharAvail(void);
extern char UARTcharAvail2(void);
extern void UARTprintf(const char *pcString, ...);
extern void UARTvprintf(const char *pcString, va_list vaArgP);
extern int UARTwrite(const char *pcBuf, uint32_t ui32Len);
extern int UARTwrite2(const char *pcBuf, uint32_t ui32Len);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __UARTSTDIO_H__
