#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "displayDipsOnSeg.h"
#include "HardwareModule/14-segDisplay.h"
#include "terminalOutput.h"
#include "utils.h"
#define LEFT_DISPLAY_LINUXNUMBER 61
#define RIGHT_DISPLAY_LINUXNUMBER 44
#define REG_OUTA 0x14
#define REG_OUTB 0x15
static void* display_thread(void* attr);
static void* sample_dips_thread_func(void* attr);
static bool requestToShutDown=false;
static pthread_t output_thread;
static pthread_t sample_dips;
static int numberOfDips;
static int leftDigit, rightDigit, ternary;

static unsigned char REG_OUTA_DIGITS[10]=
{
    0xa1,   //0
    0x80,   //1
    0x31,   //2
    0xb0,   //3
    0x90,   //4
    0xb0,   //5
    0xb1,   //6
    0x80,   //7 
    0xb1,   //8
    0xb0    //9

};
static unsigned char REG_OUTB_DIGITS[10]=
{
    0x86,   //0
    0x02,   //1
    0x0e,   //2
    0x0e,   //3
    0x8a,   //4
    0x8c,   //5
    0x8c,   //6
    0x06,   //7
    0x8e,   //8
    0x8e    //9
};
/*
Separate thread to samples number of dips 10 times a second
*/
static void* sample_dips_thread_func(void* attr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    while(!requestToShutDown)
    {
        numberOfDips= getCurrentNumOfDips();
        ternary=numberOfDips;
        /*
        extract left and right digits
        */
        rightDigit=ternary%10;
        ternary/=10;
        leftDigit=ternary%10;

        sleep_ms(100);

    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    sleep_ms(1);
    return NULL;
}

static void* display_thread(void* attr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    
    while(!requestToShutDown)
    {
        
        
        if(numberOfDips<10){
            TurnOffDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            TurnOffDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);
            writeI2cReg(REG_OUTA, REG_OUTA_DIGITS[0]);
            writeI2cReg(REG_OUTB, REG_OUTB_DIGITS[0]);
            TurnOnDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            sleep_ms(5);

            TurnOffDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            TurnOffDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);

            writeI2cReg(REG_OUTA, REG_OUTA_DIGITS[numberOfDips]);
            writeI2cReg(REG_OUTB, REG_OUTB_DIGITS[numberOfDips]);
            TurnOnDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);
            sleep_ms(5);
        }
        else if(numberOfDips>=10 &&numberOfDips<=99)
        {
            TurnOffDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            TurnOffDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);
            writeI2cReg(REG_OUTA, REG_OUTA_DIGITS[leftDigit]);
            writeI2cReg(REG_OUTB, REG_OUTB_DIGITS[leftDigit]);
            TurnOnDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            sleep_ms(5);

            TurnOffDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            TurnOffDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);

            writeI2cReg(REG_OUTA, REG_OUTA_DIGITS[rightDigit]);
            writeI2cReg(REG_OUTB, REG_OUTB_DIGITS[rightDigit]);
            TurnOnDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);
            sleep_ms(5);
        }
        else
        {
            TurnOffDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            TurnOffDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);
            writeI2cReg(REG_OUTA, REG_OUTA_DIGITS[9]);
            writeI2cReg(REG_OUTB, REG_OUTB_DIGITS[9]);
            TurnOnDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            sleep_ms(5);

            TurnOffDisplayDigit(LEFT_DISPLAY_LINUXNUMBER);
            TurnOffDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);

            writeI2cReg(REG_OUTA, REG_OUTA_DIGITS[9]);
            writeI2cReg(REG_OUTB, REG_OUTB_DIGITS[9]);
            TurnOnDisplayDigit(RIGHT_DISPLAY_LINUXNUMBER);
            sleep_ms(5);

        }
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    sleep_ms(1);
    return NULL;

}
void DisplaySig_Init(void)
{   
    SegDisplay_Init();
    numberOfDips=0;
    requestToShutDown=false;
    pthread_create(&output_thread, NULL, display_thread, NULL);
    pthread_create(&sample_dips,NULL, sample_dips_thread_func,NULL);
    
}
void DisplaySig_Destroy(void)
{
    requestToShutDown=true;
    pthread_cancel(output_thread);
    pthread_join(output_thread,NULL);
    pthread_cancel(sample_dips);
    pthread_join(sample_dips,NULL);
    SegDisplay_Destroy();
}