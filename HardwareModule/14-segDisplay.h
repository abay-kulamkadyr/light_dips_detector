#ifndef _14_SEG_DISPLAY_H_
#define _14_SEG_DISPLAY_H_
void SegDisplay_Init(void);
void SegDisplay_Destroy(void);
void TurnOnDisplayDigit(int linuxPinNumber);
void TurnOffDisplayDigit(int linuxPinNumber);
void writeI2cReg(unsigned char regAddr, unsigned char value);
#endif 