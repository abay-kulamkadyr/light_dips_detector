#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "14-segDisplay.h"
#include "../utils.h"
#define MAX_SIZE 1024
#define EXPORT_FILE    "/sys/class/gpio/export"
#define I2C_DEVICE_ADDRESS 0x20
#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_OUTA 0x14
#define REG_OUTB 0x15
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
static void setPinDirection(int linuxPinNumber);
static int i2cFileDesc;
/*
Initialization
*/
static int initI2cBus(char* bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0) {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFileDesc;
}

/*
    Writing a register
*/
void writeI2cReg(unsigned char regAddr, unsigned char value)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if (res != 2) {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }

}
void SegDisplay_Init()
{
    
    /*
    Exporting GPIO pins
    */
    FILE *pExportFile= fopen(EXPORT_FILE, "w");
    if(!pExportFile)
    {
        printf("ERROR opening file %s\n",EXPORT_FILE);
        exit(-1);
    }
    fprintf(pExportFile,"61");
    fprintf(pExportFile,"44");
    fclose(pExportFile);
    //Allow enough time for GPIO export to complete
    sleep_ms(300);
    setPinDirection(61);
    setPinDirection(44);
    /*
    Drive a 1 to the GPIO pin to turn on the digit
    */
    TurnOnDisplayDigit(61);
    TurnOnDisplayDigit(44);
    
    /*
    Enabling /dev/i2c-1 (hardware bus IC2C1)
    */
    FILE* fp_P9_18;
    FILE* fp_P9_17;
    char pinConfig_P9_18[MAX_SIZE]="config-pin P9_18 i2c";
    char pinConfig_P9_17[MAX_SIZE]="config-pin P9_17 i2c";
    fp_P9_17=popen(pinConfig_P9_17,"w");
    fp_P9_18=popen(pinConfig_P9_18,"w");
    pclose(fp_P9_17);
    pclose(fp_P9_18);
    
    /*
    Set direction of both 8 bit ports on I2C GPIO extender to be outputs
    */
    
    i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    writeI2cReg(REG_DIRA, 0x00);
    writeI2cReg(REG_DIRB, 0x00);
    /*
    Turnoff both digits initially
    */
    writeI2cReg(REG_OUTA, 0x00);
    writeI2cReg(REG_OUTB, 0x00);
   

}
static void setPinDirection(int linuxPinNumber)
{
    char buff[MAX_SIZE];
    sprintf(buff, "/sys/class/gpio/gpio%d/direction",linuxPinNumber);
    FILE * pDirectionFile= fopen(buff, "w");
    if(!pDirectionFile)
    {
        printf("Error opening file %s\n", buff);
    }
    fprintf(pDirectionFile,"out");
    fclose(pDirectionFile);
}
void TurnOnDisplayDigit(int linuxPinNumber)
{
    char buff[MAX_SIZE];
    sprintf(buff, "/sys/class/gpio/gpio%d/value",linuxPinNumber);
    FILE * pDirectionFile= fopen(buff, "w");
    if(!pDirectionFile)
    {
        printf("Error opening file %s\n", buff);
    }
    fprintf(pDirectionFile, "1");
    fclose(pDirectionFile);
}
void TurnOffDisplayDigit(int linuxPinNumber)
{
    char buff[MAX_SIZE];
    sprintf(buff, "/sys/class/gpio/gpio%d/value",linuxPinNumber);
    FILE * pDirectionFile= fopen(buff, "w");
    if(!pDirectionFile)
    {
        printf("Error opening file %s\n", buff);
    }
    fprintf(pDirectionFile, "0");
    fclose(pDirectionFile);
}
void SegDisplay_Destroy()
{
    writeI2cReg(REG_OUTA, 0x00);
    writeI2cReg(REG_OUTB, 0x00);
    close(i2cFileDesc);
}