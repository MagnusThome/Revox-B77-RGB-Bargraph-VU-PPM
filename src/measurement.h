#ifndef Measurement_H_
#define Measurement_H_

#include <Arduino.h>


#define UPDATEFREQ   12
#define NUMCHANNELS  2

void startadc(void);
void stopadc(void);
void findDcBias(uint8_t runs);
void sampleAudio(void);
void refreshAVG(void);
void refreshRMS(void);
void refreshPPM(void);
int avgBallistics(uint8_t ch);
int rmsBallistics(uint8_t ch);
int ppmBallistics(uint8_t ch);
void debugMeasurement(void);


#endif
