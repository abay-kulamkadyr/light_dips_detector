#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h> 
#include "UDP_Listener.h"
#include "shutdown.h"
#include <netdb.h>
#include <string.h>			
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include "utils.h"
#include "sampler.h"
#define PORT 12345
#define MAX_COMMAND_SIZE 100
#define NUMBER_OF_COMMANDS 7
#define MAX_UDP_SIZE 1500 
#define SAMPLES_SIZE_BYTES 8
static void* UDP_listener_thread(void* attr);
static pthread_t UDP_listener; 
static int socketDescriptor;
static bool requestToShutdown;
static struct options parseOptions (char* request);
static struct sockaddr_in sinRemote;
static unsigned int sin_len = sizeof(sinRemote);
static void processRequests(struct options* opts, char * message);

static struct options previous_opts;

static char listOf_commands[NUMBER_OF_COMMANDS][MAX_COMMAND_SIZE]={
    "help\n", 
    "count\n",
    "get \n",
    "length\n", 
    "history\n",
    "stop\n", 
    "\n",
};
char* itoa(int val, int base){
// adapted from http://www.strudel.org.uk/itoa/

	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}

int numPlaces (int n) {
// adapted from https://stackoverflow.com/questions/1068849/how-do-i-determine-the-number-of-digits-of-an-integer-in-c
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    /*      2147483647 is 2^31-1 - add more ifs as needed
       and adjust this final return as well. */
    return 10;
}
static void initOptions(struct options* getOption )
{
    getOption->option_help=false;
    getOption->option_count=false;
    getOption->option_getN=false;
    getOption->option_length=false;
    getOption->option_history=false;
    getOption->option_stop=false;
    getOption->option_repeatLast=false;

}
static struct options parseOptions (char* request)
{
    //creating a struct of options to infer which option has been requested
    struct options passedOptions;
    //initiallizing all options to false initially 
    initOptions(&passedOptions);
    int options=100;
    // int ret;
    //char localCopy_Request[MAX_COMMAND_SIZE];
    
    char turnary [10];
    turnary[0]=request[3];
    turnary[1]=request[4];
    turnary[2]=request[5];
    turnary[3]=request[6];
    turnary[4]=request[7];
    turnary[5]=0;


    int N=atoi(turnary);

    if(N!=0)
    {
        int numOfDigits=numPlaces(N);
        
        char* snum= itoa(N,10);
        for(int i=0;i<numOfDigits;i++)
        {
            listOf_commands[2][4+i]=snum[i];
        }
        listOf_commands[2][4+numOfDigits]=0x0A;
        listOf_commands[2][4+numOfDigits+1]=0;

    }
    for(int i=0;i<NUMBER_OF_COMMANDS;i++)
    {
        if((strcmp(listOf_commands[i], request))==0)
        {
            options=i;
            break;
        }
    }
   
    char* messageTx;
    switch (options)
    {
        case 0:
        passedOptions.option_help=true;
            break;
        case 1:
        passedOptions.option_count=true;
        break;
        case 2:
        passedOptions.option_getN=true; 
        break;
        case 3:
        passedOptions.option_length=true;
        break;
        case 4:
        passedOptions.option_history=true;
        break;
        case 5:
        passedOptions.option_stop=true;
        break;
        case 6:
        passedOptions.option_repeatLast=true;
        break;
        default:
        messageTx= "Sorry the specified options are not currently supported\n";
        sin_len = sizeof(sinRemote);
        sendto( socketDescriptor,
        messageTx, strlen(messageTx),
        0,
        (struct sockaddr *) &sinRemote, sin_len); 
    }
    
    return passedOptions;
}

void UDP_Init(void)
{
    requestToShutdown=false;
    struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                   // Connection may be from network
	sin.sin_addr.s_addr = htonl(INADDR_ANY);    // Host to Network long
	sin.sin_port = htons(PORT);                 // Host to Network short
	
	// Create the socket for UDP
    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

	// Bind the socket to the port (PORT) that we specify
	bind (socketDescriptor, (struct sockaddr*) &sin, sizeof(sin));	

    pthread_create(&UDP_listener, NULL, UDP_listener_thread, NULL);

}
void UDP_Destroy(void)
{
    requestToShutdown=true;
    pthread_cancel(UDP_listener);
    pthread_join(UDP_listener, NULL);
    close(socketDescriptor);

}
static void processRequests(struct options* opts, char* messageRx)
{

    if(opts->option_help)
    {
        char messageTx[]="Accepted command examples: \n"
                        "count\t\t\t--display total number of samples taken\n"
                        "length\t\t\t-- display number of samples in history (both max, and current).\n"
                        "history\t\t\t -- display the full sample history being saved.\n"
                        "get 10\t\t\t-- display the 10 most recent history values.\n"
                        "dips\t\t\t-- display number of dips\n"
                        "stop\t\t\t-- cause the server program to end.\n"
                        "<enter>\t\t\t-- repeat last command.\n";
        
     
		// Transmit a reply:
		sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);
    }
    else if(opts->option_count)
    {
        long long numSamples= Sampler_getNumSamplesTaken();

        char messageTx[MAX_UDP_SIZE];
        sprintf(messageTx,"Number of samples taken=%lld\n", numSamples);
        sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);

    }
    else if(opts->option_getN)
    {
        int length =0;
        double * history_buffer= Sampler_getHistory(&length);
        char turnary[10];
        turnary[0]=messageRx[3];
        turnary[1]=messageRx[4];
        turnary[2]=messageRx[5];
        turnary[3]=messageRx[6];
        turnary[4]=messageRx[7];
        turnary[5]=0;


        int N=atoi(turnary);
        char messageTx[MAX_UDP_SIZE];
       int totalNumBytesWritten;
        if(N>=146)
        {
            sprintf(messageTx,"Sorry, this command only supports up to 146 samples.");
        }
        else
        {
            int numSamplesWritten=0;
            char convertSampleToString[SAMPLES_SIZE_BYTES];
            totalNumBytesWritten=0;
            int counterForNewLineCharacter=0;

            while(numSamplesWritten!=N){

                while (totalNumBytesWritten<MAX_UDP_SIZE)
                {
                    totalNumBytesWritten+=sprintf(convertSampleToString,"%0.3lf, ", history_buffer[numSamplesWritten]);
                    
                    if(totalNumBytesWritten>=MAX_UDP_SIZE)
                        {
                            break;
                        }
                        strcpy(messageTx+totalNumBytesWritten-SAMPLES_SIZE_BYTES+1, convertSampleToString);  

                        if(counterForNewLineCharacter==19)
                        {
                            *(messageTx+totalNumBytesWritten)='\n';
                            totalNumBytesWritten++;
                            counterForNewLineCharacter=0;
                        }                  
                        counterForNewLineCharacter++;
                        
                        numSamplesWritten++;
                        if(numSamplesWritten==N)
                        {
                            break;
                        }

                }
            }
        }
        messageTx[totalNumBytesWritten-2]='\n';
        messageTx[totalNumBytesWritten-1]=0;
        sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);
        free(history_buffer);
    }
    else if(opts->option_length)
    {
        int maxSize= Sampler_getHistorySize();
        int samples_inHistory= Sampler_getNumSamplesInHistory();
        char messageTx[MAX_UDP_SIZE];
        sprintf(messageTx, "History can hold \t%d samples.\nCurrently holding\t %d samples.\n", maxSize, samples_inHistory);
        sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);
    }
    else if(opts->option_history)
    {
        int length =0;
        int counterForNewLineCharacter=0;
        int totalNumBytesWritten=0;
        double * history_buffer= Sampler_getHistory(&length);
        char messageTx[MAX_UDP_SIZE];
        char convertSampleToString[SAMPLES_SIZE_BYTES];
        int numSamplesWritten=0;

        /*
        1 UDP packet can hold 178 samples
        1500/20 = 75 '\n' characters in total
        (1500-75-1)/8 =178 samples, where each sample is 8 bytes
        */
        //Convert samples to string
        
        while(numSamplesWritten!=length)
        {
            while(totalNumBytesWritten<MAX_UDP_SIZE) {
           
                    totalNumBytesWritten+=sprintf(convertSampleToString,"%0.3lf, ", history_buffer[numSamplesWritten]);
                    if(totalNumBytesWritten>=MAX_UDP_SIZE)
                    {
                        break;
                    }
                    strcpy(messageTx+totalNumBytesWritten-SAMPLES_SIZE_BYTES+1, convertSampleToString);  

                    if(counterForNewLineCharacter==19)
                    {
                        *(messageTx+totalNumBytesWritten)='\n';
                        totalNumBytesWritten++;
                        counterForNewLineCharacter=0;
                    }                  
                    counterForNewLineCharacter++;
                    
                    numSamplesWritten++;
                    if(numSamplesWritten==length)
                    {
                        break;
                    }
            }
            if(numSamplesWritten==length)
            {
                messageTx[totalNumBytesWritten-6]='\n';
                messageTx[totalNumBytesWritten-7]=0;


            }
            messageTx[totalNumBytesWritten-7]=0;
            sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);
            sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);
            totalNumBytesWritten=0;
           
        }
        free(history_buffer);   
    }
    
    else if(opts->option_repeatLast)
    {
        if(!previous_opts.option_repeatLast){
            processRequests(&previous_opts, messageRx);
        }
        
        else{

            char* messageTx;
            messageTx= "Sorry the specified options are not currently supported\n";
            sin_len = sizeof(sinRemote);
            sendto( socketDescriptor,
            messageTx, strlen(messageTx),
            0,
            (struct sockaddr *) &sinRemote, sin_len); 
        }

    }
    else
    {
        return;
    }
}
static void* UDP_listener_thread(void* attr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
    initOptions(&previous_opts);
    previous_opts.option_repeatLast=true;
    while(!requestToShutdown)
    {
        // Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		
		char messageRx[MAX_COMMAND_SIZE];
        // Pass buffer size - 1 for max # bytes so room for the null (string data)
        int bytesRx = recvfrom(socketDescriptor,
			messageRx, MAX_COMMAND_SIZE - 1, 0,
			(struct sockaddr *) &sinRemote, &sin_len);
        messageRx[bytesRx] = 0;
        if(strncmp(listOf_commands[5],messageRx, MAX_COMMAND_SIZE)==0){
            unlock_main_thread();
            break;
        }
        struct options opts= parseOptions(messageRx);

        if(!opts.option_repeatLast)
        {
            previous_opts = opts;
            previous_opts.option_repeatLast=false;
        }
        processRequests(&opts, messageRx);

    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    sleep_ms(1);
    return NULL;
}