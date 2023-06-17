#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include "bufferResize.h"
#include "sampler.h"
#include "bufferAnalyzer.h"
#include "utils.h"

static void* stdout_thread(void* attr);
static pthread_t info_thread;
static bool requestToShutDown;
static int currentNumOfDips;

int getCurrentNumOfDips()
{
    return currentNumOfDips;
}
void Terminal_Init(void)
{
    requestToShutDown=false;
    pthread_create(&info_thread,NULL, stdout_thread, NULL);
    LightDips_Init();
}
void Terminal_Destroy(void)
{
    requestToShutDown=true;
    pthread_cancel(info_thread);
    pthread_join(info_thread,NULL);
}
/*
Line 1:
# light samples taken during the last second
The raw value read from the POT
Number of valid samples in the history
The averaged light level (from exponential smoothing), displayed as a voltage with 3
decimal places.
The number of light level dips that have been found in sample history
Line 2:
Display every 200th sample in the sample history.
Show the oldest of these samples on the left, the newest of these samples on the right.
*/
static void* stdout_thread(void* attr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    
    long long numSamplesPrevious=0;
    long long numSamplesCurrent=0;
    long long sampleTakenLastSecond=0;
    int RawValuePot=0;
    int validSamplesInHistory=0;
    double averageLightLevel=0;
    double previousNumOfDips=0;
    currentNumOfDips=0;
    int every200thSamplebuff_len=0;
    double* every200thSamplebuff=NULL;
    
    while(!requestToShutDown)
    {
        numSamplesPrevious= Sampler_getNumSamplesTaken();
        LightDips_analyzer();
        RawValuePot= Sampler_getHistorySize();
        validSamplesInHistory= Sampler_getNumSamplesInHistory();
        averageLightLevel= Sampler_getAverageReading();
        currentNumOfDips= LightDips_getNumDips();
        every200thSamplebuff= LightDips_getEvery200thSample(&every200thSamplebuff_len);

        sleep_ms(1000);
        numSamplesCurrent=Sampler_getNumSamplesTaken();
        sampleTakenLastSecond= numSamplesCurrent-numSamplesPrevious;
       
        //Count down if the program sits idle, i.e. when no light dips detected since last time
        if(currentNumOfDips==previousNumOfDips && currentNumOfDips>0)
        {
            LightDips_countDown();
        }

        printf("#samples taken: %lld\nPOT value: %d\n#valid samples: %d\navg light level: %0.3lf\n#dips: %d\n", 
                sampleTakenLastSecond,RawValuePot, validSamplesInHistory, averageLightLevel, currentNumOfDips);

        printf("\n");
        if(every200thSamplebuff!=NULL){
            
            for(int i=0;i<every200thSamplebuff_len;i++)
            {
                printf("%dth sample: %lf\n",(i+1)*200, every200thSamplebuff[i]);
            }
            printf("\n");
        }
        else
        {
            continue;
        }

        previousNumOfDips=currentNumOfDips;
        free(every200thSamplebuff);
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    sleep_ms(1);
    return NULL;
}