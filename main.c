#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include "shutdown.h"
#include "terminalOutput.h"
#include "sampler.h"
#include "bufferResize.h"
#include "bufferAnalyzer.h"
#include "utils.h"
#include "displayDipsOnSeg.h"
#include "UDP_Listener.h"
int main()
{
    
    pthread_mutex_t main_block_mutex=PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t main_block_condvar=PTHREAD_COND_INITIALIZER;
    Sampler_startSampling();
    BufferResize_Init();
    Terminal_Init();
    DisplaySig_Init();
    UDP_Init();
    //Blocking the main thread until some module calls unlock_main_thread() 
    lock_main_thread(&main_block_mutex, &main_block_condvar);
    pthread_mutex_destroy(&main_block_mutex);
    pthread_cond_destroy(&main_block_condvar);
    Terminal_Destroy();
    BufferResize_Destroy();
    Sampler_stopSampling();
    DisplaySig_Destroy();
    UDP_Destroy();
    return 0;
}