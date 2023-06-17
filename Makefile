
#compiler setup
CC=arm-linux-gnueabihf-gcc
CFLAGS=-std=c99 -Wall -g -pthread -D _POSIX_C_SOURCE=200809L -Werror
PROGRAM_NAME= light_sampler
all: 
	$(CC) $(CFLAGS) main.c ./HardwareModule/lightSensor.c ./HardwareModule/potentiometer.c sampler.c shutdown.c utils.c bufferAnalyzer.c bufferResize.c terminalOutput.c ./HardwareModule/14-segDisplay.c displayDipsOnSeg.c UDP_Listener.c -o $(PROGRAM_NAME) 
	mkdir $(HOME)/cmpt433/public/myApps
	cp $(PROGRAM_NAME) $(HOME)/cmpt433/public/myApps/

clean:
	rm -f *.o *.obj $(PROGRAM_NAME) $(HOME)/cmpt433/public/assignment2/$(PROGRAM_NAME)
