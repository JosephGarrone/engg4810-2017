#ifndef __ADS124x_H
#define __ADS124x_H

#define CS GPIO_PIN_6
#define CS_PORT GPIO_PORTB_BASE
#define START GPIO_PIN_4
#define START_PORT GPIO_PORTB_BASE
#define DRDY GPIO_PIN_6
#define DRDY_PORT GPIO_PORTA_BASE
#define DRDY_PIN_INT GPIO_INT_PIN_6
#define RESET GPIO_PIN_7
#define RESET_PORT GPIO_PORTB_BASE

/* ADS1248 Register (see p42 for Register Map) */
#define     BCS         0x00
#define     VBIAS       0X01
#define     MUX1        0x02
#define     SYS0        0x03
#define     OFC0        0X04
#define     OFC1        0x05
#define     OFC2        0x06
#define     FSC0        0x07
#define     FSC1        0x08
#define     FSC2        0x09
#define     IDAC0       0x0A

/* SPI COMMAND DEFINITIONS (p49) */
/*SYSTEM CONTROL */
#define     WAKEUP      0x00    //Exit Sleep Mode
#define     SLEEP       0x01    //Enter Sleep Mode
#define     SYNC        0x04    //Synchornize the A/D Conversion
#define     RESET_ADC   0x06    //Reset To Power UP values
#define     NOP         0xFF    //NO Operation
/*DATA READ*/
#define     RDATA       0x12    //Read data once
#define     RDATAC      0x14    //Read data continously
#define     SDATAC      0x16    //Stop reading data continously
/*READ REGISTER */
#define     RREG        0x20    //Read From Register
#define     WREG        0x40    //Write To Register
/*Calibration */
#define     SYSOCAL     0x60    //System Offset Calibration
#define     SYSGCAL     0x61    //System Gain Calibration
#define     SELFOCAL    0x62    //Self Offset Calibration

/* IDAC0- IDAC Control Register 0 (see p46 - bring together with bitwise OR | */
/* BIT7 - BIT6 - BIT5 - BIT4 -   BIT3    - BIT2  - BIT1  - BIT0 */
/* ID3  - ID2  - ID1  - ID0  - DRDY MODE - IMAG2 - IMAG1 - IMAG0 */
#define IDAC0_ID    144
/* DRDY MODE This bits sets the DOUT/DRDY pin functionality  */
#define     DRDY_ONLY     0b00000000    // Data out only (default)
#define     DRDY_BOTH     0b00001000    // Data out and Data ready functionality
/* SYS0 - System Control Register 0 (see p45 - bring together with bitwise OR | */
/* BIT7 - BIT6 - BIT5 - BIT4 - BIT3 - BIT2 - BIT1 - BIT0 */
/*  0   - PGA2 - PGA1 - PGA0 - DOR3 - DOR2 - DOR1 - DOR0 */
/* PGA2:0 These bits determine the gain of the PGA  */
#define     PGA2_0          0b00000000    // 1 (default)
#define     PGA2_2          0b00010000    // 2
#define     PGA2_4          0b00100000    // 4
#define     PGA2_8          0b00110000    // 8
#define     PGA2_16         0b01000000    // 16
#define     PGA2_32         0b01010000    // 32
#define     PGA2_64         0b01100000    // 64
#define     PGA2_128        0b01110000    // 128
/* DOR3:0 These bits select the output data rate of the ADC  */
#define     DOR3_5      0b00000000    // 5SPS (default)
#define     DOR3_10     0b00000001    // 10SPS
#define     DOR3_20     0b00000010    // 20SPS
#define     DOR3_40     0b00000011    // 40SPS
#define     DOR3_80     0b00000100    // 80SPS
#define     DOR3_160    0b00000101    // 160SPS
#define     DOR3_320    0b00000110    // 320SPS
#define     DOR3_640    0b00000111    // 640SPS
#define     DOR3_1000   0b00001000    // 1000SPS
#define     DOR3_2000   0b00001001    // 2000SPS
#define     VBIAS_1     0b00000010    // AIN1
#define     VBIAS_0     0b00000001    // AIN0


void DelayMSec(uint32_t);
void DelayUSec(uint32_t);
void ConfigureADC();
void ADCCalibrate();
int32_t ADCGetConversion();
void ADCSetRegister(uint8_t, uint8_t);
uint32_t ADCReadRegister(uint8_t);
void ADCSendCMD(uint8_t);
void ADCReset();

#endif
