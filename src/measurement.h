#ifndef Measurement_H_
#define Measurement_H_

#include <Arduino.h>

#define UPDATEFREQ   12
#define NUMCHANNELS  2


void beginmeasurement(void);
void startadc(void);
void stopadc(void);
void findDcBias(int runs);
void sampleAudio(void);
void refreshRMS(void);
void refreshPPM(void);
uint8_t ppmBallistics(uint8_t channel);
uint8_t vuBallistics(uint8_t channel);



#endif
