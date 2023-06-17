#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "sampler.h"
#include "utils.h"
#include "shutdown.h"
#include "HardwareModule/lightSensor.h"
#define EPX_SMOOTHING_ALPHA 0.01 
#define INITIAL_HISTORY_SIZE 1000





/*
--------------GLOBAL STATIC VARIABLES----------------------
*/
static bool requestToShutdown=false;
static long long numSamplesSinceStarted;
static bool isHistoryFull;
static long long numSamplesInHistory;
static double averageOfReadings;
static pthread_t sampler_thread;
static double* history_buf;
static int historySize;
static pthread_mutex_t mutex_HistorySizeController=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_HistorySize= PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_HistoryBuffer=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_HistoryBuffer= PTHREAD_COND_INITIALIZER;

//sampling thread
static void* Sampler_thread(void*  attr);

static int previous_HistorySize= INITIAL_HISTORY_SIZE;
void Sampler_startSampling()
{
    //Initialization of variables
    lightSensorInit();
    historySize=INITIAL_HISTORY_SIZE;
    history_buf= (double*)malloc(sizeof(double)*historySize);
    memset(history_buf, 0, historySize*sizeof(double));
    requestToShutdown=false;
    int err=pthread_create(&sampler_thread, NULL, Sampler_thread, NULL);
    
    if(err)
    {
        printf("Creating of the sampler thread has failed\n");
        exit(-1);
    }
}




static void history_HasChanged_interrupt(int previous_historySize, int* index_where_terminated)
{
    //allocating memory to the new buffer
    double * newHistory_buff= (double*) malloc(sizeof(double)*historySize);
    if(newHistory_buff==NULL)
    {
        printf("ERROR: couldn't allocate memory for new history size\n");
        exit(-1);
    }
    memset(newHistory_buff, 0, historySize*sizeof(double));
    
    //When the size decreases from N to M (M < N), take the most recent M samples.
    //The most recent samples will start at index 0 of the new history buffer
    if (previous_historySize>historySize && numSamplesInHistory!=0){
    
        int counter=0;
        int position=*index_where_terminated-1;
        while (position>=previous_historySize)
            position=position-previous_historySize;
        while (position<0)
            position=position+previous_historySize;
        int termination_point=(numSamplesInHistory<historySize)?(numSamplesInHistory):(historySize);
        
        while (counter!=termination_point)
        {
            newHistory_buff[counter]=history_buf[position];           
            position--;
            while (position>=previous_historySize)
                position= position-previous_historySize;
            while (position<0)
                position=position+previous_historySize;
            counter++;    
        } 
     
    //Reset index where we were inserting at to point to the new position in the new buffer 
        *(index_where_terminated)= termination_point;
        numSamplesInHistory= termination_point;
        isHistoryFull=(numSamplesInHistory>=historySize);
   
    }

    //When the size increases from N to M (M > N), take all N samples
    else{
        
        for(int i=0;i<numSamplesInHistory;i++)
        {
            newHistory_buff[i]=history_buf[i];
           
        }

        isHistoryFull=false;
        *index_where_terminated=numSamplesInHistory; 
    }

    free(history_buf);
    history_buf= newHistory_buff;
    
}
static void* Sampler_thread(void*  attr)
{
    //Disabling cancellation for a while
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    
    int newestSampleIndex=0;
    numSamplesSinceStarted=0;
    isHistoryFull=false;
    numSamplesInHistory=0;
    averageOfReadings=getLightSensorReadings();
   
    while(!requestToShutdown)
    {
        //Before sampling check if there is no request for accessing the history buffer
        pthread_mutex_lock(&mutex_HistoryBuffer);
        
        //When change in history size is occured, block the history size on mutex and resize the history buffer. 
        pthread_mutex_lock(&mutex_HistorySizeController);
        {
            if(previous_HistorySize!=historySize){
                history_HasChanged_interrupt(previous_HistorySize, &newestSampleIndex);
                previous_HistorySize=historySize;
            }  
        }
        //once done processing the request to change the size unlock the mutex on the 
        pthread_mutex_unlock(&mutex_HistorySizeController);
    
       
        /*Circular buffer implementation
        _____________________________________________________________
        */
        while(newestSampleIndex>=previous_HistorySize)
        {
            newestSampleIndex=newestSampleIndex-previous_HistorySize;
            numSamplesInHistory=previous_HistorySize;
            isHistoryFull=true;
        }
        while (newestSampleIndex<0)
        { 
            newestSampleIndex=newestSampleIndex+previous_HistorySize;   
        }
        //___________________________________________________________
        

        //sleep for 1 ms between samples
        sleep_ms(1);
        history_buf[newestSampleIndex]=getLightSensorReadings();

        
        
        /*
        Calculating information about the samples
        _____________________________________________________________
        */
        if(!isHistoryFull){
            numSamplesInHistory++;
        }
        averageOfReadings=(1-EPX_SMOOTHING_ALPHA)*averageOfReadings+ EPX_SMOOTHING_ALPHA*history_buf[newestSampleIndex];
        newestSampleIndex++;    
        numSamplesSinceStarted++;
        /*
        _____________________________________________________________
        */



        //Signal that the the buffer is now full
        if(isHistoryFull){

            pthread_cond_signal(&cond_HistoryBuffer);
        } 
        pthread_mutex_unlock(&mutex_HistoryBuffer);   
    }
    
    // reset the thread to be cancellable 
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    
    pthread_cond_signal(&cond_HistoryBuffer);
   
    //cancellation point 
    sleep_ms(1);
    return NULL;
    
}


void Sampler_stopSampling()
{
    requestToShutdown=true;
    pthread_cancel(sampler_thread);
    pthread_join(sampler_thread, NULL);
    
    
    pthread_mutex_destroy(&mutex_HistorySizeController);
    pthread_cond_destroy(&cond_HistorySize);
    pthread_mutex_destroy(&mutex_HistoryBuffer);
    pthread_cond_destroy(&cond_HistoryBuffer);
    
    lightSensorDestroy();
    free(history_buf);
    history_buf=NULL;
    
}

void Sampler_setHistorySize(int newSize)
{
    if(newSize>0)
    {   
            
        if(requestToShutdown)
        {
            return; 
        }
        pthread_mutex_lock(&mutex_HistorySizeController);
        {
          historySize= newSize;
          
        }
        pthread_mutex_unlock(&mutex_HistorySizeController);

    }
    else
    {
        printf("ERROR, passed an invalid history size \n");
        
        exit(-1);
    }
}
int Sampler_getHistorySize()
{
    return historySize;
}
// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `length` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: provides both data and size to ensure consistency.
double* Sampler_getHistory(int *length)
{
    double *copy;
   
    //Acquire the mutex for the history buffer first
    pthread_mutex_lock(&mutex_HistoryBuffer);
    {
        if(requestToShutdown)
        {
            pthread_mutex_unlock(&mutex_HistoryBuffer);
            return NULL;
        }  
        //wait until the buffer is filled with samples     
        while(!isHistoryFull)
        {
            pthread_cond_wait(&cond_HistoryBuffer, &mutex_HistoryBuffer);
            if(requestToShutdown)
            {
                pthread_mutex_unlock(&mutex_HistoryBuffer);
                return NULL;
            }
        }
        //lock the mutex which controls the history size
        pthread_mutex_lock(&mutex_HistorySizeController);
        {
            copy =(double*)malloc(sizeof(double)*historySize);
          
            memset(copy, 0, historySize*sizeof(double));
            for(int i=0;i<historySize;i++)
            {
                copy[i]= history_buf[i];
                
            }
            *length= historySize;
        }
        pthread_mutex_unlock(&mutex_HistorySizeController);
    }
    pthread_mutex_unlock(&mutex_HistoryBuffer);
    
    return copy;
}

// Returns how many valid samples are currently in the history.
// May be less than the history size if the history is not yet full.
int Sampler_getNumSamplesInHistory()
{
    return numSamplesInHistory;
}
// Get the average light level (not tied to the history).
double Sampler_getAverageReading()
{
    return averageOfReadings;
}
// Get the total number of light level samples taken so far.
long long Sampler_getNumSamplesTaken()
{
    return numSamplesSinceStarted;
}


