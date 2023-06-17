#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "bufferAnalyzer.h"
#include "sampler.h"
#include "utils.h"
#define THREASHOLD 0.1
#define MIN_HISTORY_SIZE 200
#define HISTERESIS 0.03

static int numDips_detected;
static double* every200thSample_buff=NULL;
static int every200thSample_buffLen;
static void LightDips_FindEvery200thSample(double * history_buf, int* length);

enum LightDipState
{
    LightDipOccured, 
    LightDipNotOccured  
};

void LightDips_Init()
{
    numDips_detected=0;
   
}

int LightDips_getNumDips()
{
    return numDips_detected;
}
static void LightDips_FindEvery200thSample(double * history_buf, int* length)
{
    if(*length<200)
    {
        every200thSample_buffLen=0;
        every200thSample_buff=NULL;
        return;
    }
    int numOfSamples=0; 
    every200thSample_buff= (double*)malloc(sizeof(double)*21);
    memset(every200thSample_buff,0.0, sizeof(double)*21);
    for(int i=199;i<*length;i=i+199)
    {
        every200thSample_buff[numOfSamples]=history_buf[i];
        numOfSamples++;
    }
    every200thSample_buffLen= numOfSamples;
}

double* LightDips_getEvery200thSample(int* length)
{
    *length= every200thSample_buffLen;
    return every200thSample_buff;
}

void LightDips_analyzer()
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    
    int length;
    int index;
    enum LightDipState lightDetector_state=LightDipNotOccured;
    double * current_history= Sampler_getHistory(&length);
    double current_averageReading= Sampler_getAverageReading();
    
    if(current_history==NULL){
        return;
    }
    if(length<MIN_HISTORY_SIZE){
        every200thSample_buff=NULL;
        every200thSample_buffLen=0;
        free(current_history);
        return;
    }

    index=0;
    while (index<=length-1)
    {
        //Using hysteresis, if light dip was detected before, to update the number of dips
        //we have to exceed the previous threshold
        if(((current_averageReading-current_history[index]))>THREASHOLD){  
            
            if(lightDetector_state==LightDipNotOccured ){
                numDips_detected++;
                lightDetector_state=LightDipOccured;
            }
            else{
                if((current_averageReading-current_history[index])>(THREASHOLD+HISTERESIS)){

                    numDips_detected++;
                }
                else{
                    lightDetector_state=LightDipNotOccured;
                }
            }
            
            // Another dip cannot be detected until the light level returns above the threshold(Average Reading)
            if(lightDetector_state==LightDipNotOccured ){
                while((current_history[index]-current_averageReading)<THREASHOLD && index<=length-2)
                {   
                    index++; 
                }
            }
            else{

                while((current_history[index]-current_averageReading)<THREASHOLD+HISTERESIS && index<=length-2)
                {   
                    index++; 
                }
            }
        }

        index++;     
    }
    //get every 200th sample
    LightDips_FindEvery200thSample(current_history, &length);
   
    free(current_history);
} 
void LightDips_countDown()
{
    numDips_detected--;
}
