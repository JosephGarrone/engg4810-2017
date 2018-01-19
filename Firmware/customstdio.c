//*****************************************************************************
//
// uartstdio.c - Utility driver to provide simple UART console functions.
//
// Copyright (c) 2007-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the Tiva Utility Library.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "customstdio.h"

//*****************************************************************************
//
//! \addtogroup uartstdio_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The base address of the chosen UART.
//
//*****************************************************************************
static uint32_t g_ui32Base = 0;
static uint32_t g_ui32Base2 = 0;

//*****************************************************************************
//
// A mapping from an integer between 0 and 15 to its ASCII character
// equivalent.
//
//*****************************************************************************
static const char * const g_pcHex = "0123456789abcdef";

//*****************************************************************************
//
// The list of possible base addresses for the console UART.
//
//*****************************************************************************
static const uint32_t g_ui32UARTBase[3] =
{
    UART0_BASE, UART1_BASE, UART2_BASE
};

//*****************************************************************************
//
// The list of UART peripherals.
//
//*****************************************************************************
static const uint32_t g_ui32UARTPeriph[3] =
{
    SYSCTL_PERIPH_UART0, SYSCTL_PERIPH_UART1, SYSCTL_PERIPH_UART2
};

//*****************************************************************************
//
//! Configures the UART console.
//!
//! \param ui32PortNum is the number of UART port to use for the serial console
//! (0-2)
//! \param ui32Baud is the bit rate that the UART is to be configured to use.
//! \param ui32SrcClock is the frequency of the source clock for the UART
//! module.
//!
//! This function will configure the specified serial port to be used as a
//! serial console.  The serial parameters are set to the baud rate
//! specified by the \e ui32Baud parameter and use 8 bit, no parity, and 1 stop
//! bit.
//!
//! This function must be called prior to using any of the other UART console
//! functions: UARTprintf() or UARTgets().  This function assumes that the
//! caller has previously configured the relevant UART pins for operation as a
//! UART rather than as GPIOs.
//!
//! \return None.
//
//*****************************************************************************
void
UARTStdioConfig(uint32_t ui32PortNum, uint32_t ui32Baud, uint32_t ui32SrcClock)
{
    ASSERT((ui32PortNum == 0) || (ui32PortNum == 1) ||
           (ui32PortNum == 2));


    if(!MAP_SysCtlPeripheralPresent(g_ui32UARTPeriph[ui32PortNum]))
    {
        return;
    }

    g_ui32Base = g_ui32UARTBase[ui32PortNum];


    MAP_SysCtlPeripheralEnable(g_ui32UARTPeriph[ui32PortNum]);

    MAP_UARTConfigSetExpClk(g_ui32Base, ui32SrcClock, ui32Baud,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_WLEN_8));

    MAP_UARTEnable(g_ui32Base);
}
void
UARTStdioConfig2(uint32_t ui32PortNum, uint32_t ui32Baud, uint32_t ui32SrcClock)
{
    ASSERT((ui32PortNum == 0) || (ui32PortNum == 1) ||
           (ui32PortNum == 2));


    if(!MAP_SysCtlPeripheralPresent(g_ui32UARTPeriph[ui32PortNum]))
    {
        return;
    }

    g_ui32Base2 = g_ui32UARTBase[ui32PortNum];


    MAP_SysCtlPeripheralEnable(g_ui32UARTPeriph[ui32PortNum]);

    MAP_UARTConfigSetExpClk(g_ui32Base2, ui32SrcClock, ui32Baud,
                            (UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_WLEN_8));

    MAP_UARTEnable(g_ui32Base2);
}
//*****************************************************************************
//
//! Writes a string of characters to the UART output.
//!
//! \param pcBuf points to a buffer containing the string to transmit.
//! \param ui32Len is the length of the string to transmit.
//!
//! This function will transmit the string to the UART output.  The number of
//! characters transmitted is determined by the \e ui32Len parameter.  This
//! function does no interpretation or translation of any characters.  Since
//! the output is sent to a UART, any LF (/n) characters encountered will be
//! replaced with a CRLF pair.
//!
//! Besides using the \e ui32Len parameter to stop transmitting the string, if
//! a null character (0) is encountered, then no more characters will be
//! transmitted and the function will return.
//!
//! In non-buffered mode, this function is blocking and will not return until
//! all the characters have been written to the output FIFO.  In buffered mode,
//! the characters are written to the UART transmit buffer and the call returns
//! immediately.  If insufficient space remains in the transmit buffer,
//! additional characters are discarded.
//!
//! \return Returns the count of characters written.
//
//*****************************************************************************
int
UARTwrite(const char *pcBuf, uint32_t ui32Len)
{
    unsigned int uIdx;

    ASSERT(g_ui32Base != 0);
    ASSERT(pcBuf != 0);

    for(uIdx = 0; uIdx < ui32Len; uIdx++)
    {
        if(pcBuf[uIdx] == '\n')
        {
            MAP_UARTCharPut(g_ui32Base, '\r');
        }

        MAP_UARTCharPut(g_ui32Base, pcBuf[uIdx]);
    }

    return(uIdx);
}
int
UARTwrite2(const char *pcBuf, uint32_t ui32Len)
{
    unsigned int uIdx;

    ASSERT(g_ui32Base2 != 0);
    ASSERT(pcBuf != 0);

    for(uIdx = 0; uIdx < ui32Len; uIdx++)
    {
        if(pcBuf[uIdx] == '\n')
        {
            MAP_UARTCharPut(g_ui32Base2, '\r');
        }

        MAP_UARTCharPut(g_ui32Base2, pcBuf[uIdx]);
    }

    return(uIdx);
}

//*****************************************************************************
//
//! A simple UART based get string function, with some line processing.
//!
//! \param pcBuf points to a buffer for the incoming string from the UART.
//! \param ui32Len is the length of the buffer for storage of the string,
//! including the trailing 0.
//!
//! This function will receive a string from the UART input and store the
//! characters in the buffer pointed to by \e pcBuf.  The characters will
//! continue to be stored until a termination character is received.  The
//! termination characters are CR, LF, or ESC.  A CRLF pair is treated as a
//! single termination character.  The termination characters are not stored in
//! the string.  The string will be terminated with a 0 and the function will
//! return.
//!
//! In both buffered and unbuffered modes, this function will block until
//! a termination character is received.  If non-blocking operation is required
//! in buffered mode, a call to UARTPeek() may be made to determine whether
//! a termination character already exists in the receive buffer prior to
//! calling UARTgets().
//!
//! Since the string will be null terminated, the user must ensure that the
//! buffer is sized to allow for the additional null character.
//!
//! \return Returns the count of characters that were stored, not including
//! the trailing 0.
//
//*****************************************************************************
int
UARTgets(char *pcBuf, uint32_t ui32Len)
{
    uint32_t ui32Count = 0;
    int8_t cChar;
    static int8_t bLastWasCR = 0;

    ASSERT(pcBuf != 0);
    ASSERT(ui32Len != 0);
    ASSERT(g_ui32Base != 0);

    ui32Len--;

    while(1)
    {
        cChar = MAP_UARTCharGet(g_ui32Base);

        if(cChar == '\b')
        {
            if(ui32Count)
            {
                UARTwrite("\b \b", 3);
                ui32Count--;
            }

            continue;
        }

        if((cChar == '\n') && bLastWasCR)
        {
            bLastWasCR = 0;
            continue;
        }

        if((cChar == '\r') || (cChar == '\n') || (cChar == 0x1b))
        {
            if(cChar == '\r')
            {
                bLastWasCR = 1;
            }
            break;
        }

        if(ui32Count < ui32Len)
        {
            pcBuf[ui32Count] = cChar;
            ui32Count++;
            MAP_UARTCharPut(g_ui32Base, cChar);
        }
    }

    pcBuf[ui32Count] = 0;

    UARTwrite("\r\n", 2);

    return(ui32Count);
}
int
UARTgets2(char *pcBuf, uint32_t ui32Len)
{
    uint32_t ui32Count = 0;
    int8_t cChar;
    static int8_t bLastWasCR = 0;

    ASSERT(pcBuf != 0);
    ASSERT(ui32Len != 0);
    ASSERT(g_ui32Base2 != 0);

    ui32Len--;

    while(1)
    {
        cChar = MAP_UARTCharGet(g_ui32Base2);

        if(cChar == '\b')
        {
            if(ui32Count)
            {
                UARTwrite2("\b \b", 3);
                ui32Count--;
            }

            continue;
        }

        if((cChar == '\n') && bLastWasCR)
        {
            bLastWasCR = 0;
            continue;
        }

        if((cChar == '\r') || (cChar == '\n') || (cChar == 0x1b))
        {
            if(cChar == '\r')
            {
                bLastWasCR = 1;
            }
            break;
        }

        if(ui32Count < ui32Len)
        {
            pcBuf[ui32Count] = cChar;
            ui32Count++;
            MAP_UARTCharPut(g_ui32Base2, cChar);
        }
    }

    pcBuf[ui32Count] = 0;

    UARTwrite2("\r\n", 2);

    return(ui32Count);
}

//*****************************************************************************
//
//! Read a single character from the UART, blocking if necessary.
//!
//! This function will receive a single character from the UART and store it at
//! the supplied address.
//!
//! In both buffered and unbuffered modes, this function will block until a
//! character is received.  If non-blocking operation is required in buffered
//! mode, a call to UARTRxAvail() may be made to determine whether any
//! characters are currently available for reading.
//!
//! \return Returns the character read.
//
//*****************************************************************************
char
UARTgetc(void)
{
    return(MAP_UARTCharGetNonBlocking(g_ui32Base));
}
char
UARTgetc2(void) {
    return(MAP_UARTCharGetNonBlocking(g_ui32Base2));
}
char
UARTcharAvail(void) {
    return(MAP_UARTCharsAvail(g_ui32Base));
}
char
UARTcharAvail2(void) {
    return(MAP_UARTCharsAvail(g_ui32Base2));
}
//*****************************************************************************
//
//! A simple UART based vprintf function supporting \%c, \%d, \%p, \%s, \%u,
//! \%x, and \%X.
//!
//! \param pcString is the format string.
//! \param vaArgP is a variable argument list pointer whose content will depend
//! upon the format string passed in \e pcString.
//!
//! This function is very similar to the C library <tt>vprintf()</tt> function.
//! All of its output will be sent to the UART.  Only the following formatting
//! characters are supported:
//!
//! - \%c to print a character
//! - \%d or \%i to print a decimal value
//! - \%s to print a string
//! - \%u to print an unsigned decimal value
//! - \%x to print a hexadecimal value using lower case letters
//! - \%X to print a hexadecimal value using lower case letters (not upper case
//! letters as would typically be used)
//! - \%p to print a pointer as a hexadecimal value
//! - \%\% to print out a \% character
//!
//! For \%s, \%d, \%i, \%u, \%p, \%x, and \%X, an optional number may reside
//! between the \% and the format character, which specifies the minimum number
//! of characters to use for that value; if preceded by a 0 then the extra
//! characters will be filled with zeros instead of spaces.  For example,
//! ``\%8d'' will use eight characters to print the decimal value with spaces
//! added to reach eight; ``\%08d'' will use eight characters as well but will
//! add zeroes instead of spaces.
//!
//! The type of the arguments in the variable arguments list must match the
//! requirements of the format string.  For example, if an integer was passed
//! where a string was expected, an error of some kind will most likely occur.
//!
//! \return None.
//
//*****************************************************************************
void
UARTvprintf(const char *pcString, va_list vaArgP)
{
    uint32_t ui32Idx, ui32Value, ui32Pos, ui32Count, ui32Base, ui32Neg;
    char *pcStr, pcBuf[16], cFill;

    ASSERT(pcString != 0);

    while(*pcString)
    {

        for(ui32Idx = 0;
            (pcString[ui32Idx] != '%') && (pcString[ui32Idx] != '\0');
            ui32Idx++)
        {
        }

        UARTwrite(pcString, ui32Idx);
        UARTwrite2(pcString, ui32Idx);

        pcString += ui32Idx;

        if(*pcString == '%')
        {
            pcString++;

            ui32Count = 0;
            cFill = ' ';

again:

            switch(*pcString++)
            {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                {
                    if((pcString[-1] == '0') && (ui32Count == 0))
                    {
                        cFill = '0';
                    }

                    ui32Count *= 10;
                    ui32Count += pcString[-1] - '0';

                    goto again;
                }

                case 'c':
                {
                    ui32Value = va_arg(vaArgP, uint32_t);

                    UARTwrite((char *)&ui32Value, 1);
                    UARTwrite2((char *)&ui32Value, 1);

                    break;
                }

                case 'd':
                case 'i':
                {
                    ui32Value = va_arg(vaArgP, uint32_t);
                    ui32Pos = 0;

                    if((int32_t)ui32Value < 0)
                    {
                        ui32Value = -(int32_t)ui32Value;
                        ui32Neg = 1;
                    }
                    else
                    {
                        ui32Neg = 0;
                    }

                    ui32Base = 10;

                    goto convert;
                }

                case 's':
                {
                    pcStr = va_arg(vaArgP, char *);

                    for(ui32Idx = 0; pcStr[ui32Idx] != '\0'; ui32Idx++)
                    {
                    }

                    UARTwrite(pcStr, ui32Idx);
                    UARTwrite2(pcStr, ui32Idx);

                    if(ui32Count > ui32Idx)
                    {
                        ui32Count -= ui32Idx;
                        while(ui32Count--)
                        {
                            UARTwrite(" ", 1);
                            UARTwrite2(" ", 1);
                        }
                    }

                    break;
                }

                case 'u':
                {
                    ui32Value = va_arg(vaArgP, uint32_t);
                    ui32Pos = 0;
                    ui32Base = 10;
                    ui32Neg = 0;

                    goto convert;
                }

                case 'x':
                case 'X':
                case 'p':
                {
                    ui32Value = va_arg(vaArgP, uint32_t);
                    ui32Pos = 0;
                    ui32Base = 16;
                    ui32Neg = 0;
convert:
                    for(ui32Idx = 1;
                        (((ui32Idx * ui32Base) <= ui32Value) &&
                         (((ui32Idx * ui32Base) / ui32Base) == ui32Idx));
                        ui32Idx *= ui32Base, ui32Count--)
                    {
                    }

                    if(ui32Neg)
                    {
                        ui32Count--;
                    }

                    if(ui32Neg && (cFill == '0'))
                    {
                        pcBuf[ui32Pos++] = '-';
                        ui32Neg = 0;
                    }

                    if((ui32Count > 1) && (ui32Count < 16))
                    {
                        for(ui32Count--; ui32Count; ui32Count--)
                        {
                            pcBuf[ui32Pos++] = cFill;
                        }
                    }

                    if(ui32Neg)
                    {
                        pcBuf[ui32Pos++] = '-';
                    }

                    for(; ui32Idx; ui32Idx /= ui32Base)
                    {
                        pcBuf[ui32Pos++] =
                            g_pcHex[(ui32Value / ui32Idx) % ui32Base];
                    }

                    UARTwrite(pcBuf, ui32Pos);
                    UARTwrite2(pcBuf, ui32Pos);

                    break;
                }

                case '%':
                {
                    UARTwrite(pcString - 1, 1);
                    UARTwrite2(pcString - 1, 1);
                    break;
                }

                default:
                {
                    UARTwrite("ERROR", 5);
                    UARTwrite2("ERROR", 5);
                    break;
                }
            }
        }
    }
}

//*****************************************************************************
//
//! A simple UART based printf function supporting \%c, \%d, \%p, \%s, \%u,
//! \%x, and \%X.
//!
//! \param pcString is the format string.
//! \param ... are the optional arguments, which depend on the contents of the
//! format string.
//!
//! This function is very similar to the C library <tt>fprintf()</tt> function.
//! All of its output will be sent to the UART.  Only the following formatting
//! characters are supported:
//!
//! - \%c to print a character
//! - \%d or \%i to print a decimal value
//! - \%s to print a string
//! - \%u to print an unsigned decimal value
//! - \%x to print a hexadecimal value using lower case letters
//! - \%X to print a hexadecimal value using lower case letters (not upper case
//! letters as would typically be used)
//! - \%p to print a pointer as a hexadecimal value
//! - \%\% to print out a \% character
//!
//! For \%s, \%d, \%i, \%u, \%p, \%x, and \%X, an optional number may reside
//! between the \% and the format character, which specifies the minimum number
//! of characters to use for that value; if preceded by a 0 then the extra
//! characters will be filled with zeros instead of spaces.  For example,
//! ``\%8d'' will use eight characters to print the decimal value with spaces
//! added to reach eight; ``\%08d'' will use eight characters as well but will
//! add zeroes instead of spaces.
//!
//! The type of the arguments after \e pcString must match the requirements of
//! the format string.  For example, if an integer was passed where a string
//! was expected, an error of some kind will most likely occur.
//!
//! \return None.
//
//*****************************************************************************
void
UARTprintf(const char *pcString, ...)
{
    va_list vaArgP;

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcString);

    UARTvprintf(pcString, vaArgP);

    //
    // We're finished with the varargs now.
    //
    va_end(vaArgP);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
