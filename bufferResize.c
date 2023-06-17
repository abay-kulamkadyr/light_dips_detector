#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bufferResize.h"
#include "sampler.h"
#include "shutdown.h"
#include "utils.h"
#include "HardwareModule/potentiometer.h"
#define MIN_BUFFER_SIZE 1
static pthread_t resize_th;
static void* resizeBuffer_thread(void* attr);
static bool request_toShutdown;
static int getPOTvalue;
void BufferResize_Init()
{
    potentiometerInit();
    getPOTvalue=0;
    request_toShutdown=false;
    pthread_create(&resize_th, NULL, resizeBuffer_thread, NULL);
}

void BufferResize_Destroy()
{
    request_toShutdown=true;
    pthread_cancel(resize_th);
    pthread_join(resize_th,NULL);
    potentionmeterDestroy();
   
}
int getCurrentPOTReading()
{
    return getPOTvalue;
}
static void* resizeBuffer_thread(void* attr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    int previous_POT_value= getPOTReadings();

    while (!request_toShutdown)
    {
        sleep_ms(1000);
        getPOTvalue= getPOTReadings();
        if(getPOTvalue<MIN_BUFFER_SIZE)
        {
            getPOTvalue=1;
        }
        if(previous_POT_value!=getPOTvalue){
            Sampler_setHistorySize(getPOTvalue);
        }
        previous_POT_value= getPOTvalue;
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    sleep_ms(1);
    return NULL;
}
