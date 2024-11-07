#ifndef Measurement_H_
#define Measurement_H_

#include <Arduino.h>


#define UPDATEFREQ   12
#define NUMCHANNELS  2

void beginmeasurement(void);
void startadc(void);
void stopadc(void);
void findDcBias(uint8_t runs);
void sampleAudio(void);
void refreshRMS(void);
void refreshPPM(void);
int ppmBallistics(uint8_t channel);
int vuBallistics(uint8_t channel);
void debugMeasurement(void);


#endif
