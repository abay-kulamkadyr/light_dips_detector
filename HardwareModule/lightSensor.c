#include <stdio.h>
#include <stdlib.h>
#include "lightSensor.h"
#define A2D_VOLTAGE1_FILE "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define MAX_PATH_SIZE 1024
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4000
static FILE* lightSensor_devFile;
void lightSensorInit(void)
{
    char lightSensor_filename[MAX_PATH_SIZE];
    sprintf(lightSensor_filename, A2D_VOLTAGE1_FILE);

    lightSensor_devFile= fopen(lightSensor_filename, "r");
    if (lightSensor_devFile==NULL)
    {
        printf("FILEOPEN ERROR: Unable to open file for read: %s\n", lightSensor_filename);
        exit(-1);
    }

}
double getLightSensorReadings(void)
{
    if(lightSensor_devFile==NULL)
    {
        printf("Voltage1 (light_sensor) device file is not open\n");
        exit(-1);
    }
    //moving the file pointer to the beginning and flushing the file pointer
    fseek(lightSensor_devFile, 0, SEEK_SET);
    fflush(lightSensor_devFile);
    int a2dReading=0;
    int itemsRead=fscanf(lightSensor_devFile, "%d", &a2dReading);
    if(itemsRead<0){
        printf("ERROR: Unable to read values from voltage input file.\n");
    }
    //converting quantize value to voltages
    double voltage= ((double)a2dReading/A2D_MAX_READING)*A2D_VOLTAGE_REF_V;
    return voltage;
}
void lightSensorDestroy(void)
{
    fclose(lightSensor_devFile);
}