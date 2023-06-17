#ifndef _BUFFERANALYZER_H_
#define _BUFFERANALYZER_H_

void LightDips_Init(void);
void LightDips_analyzer(void);
int LightDips_getNumDips(void);
double* LightDips_getEvery200thSample(int* length);
void LightDips_countDown(void);

#endif