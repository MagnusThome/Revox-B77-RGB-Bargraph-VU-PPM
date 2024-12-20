#ifndef Measurement_H_
#define Measurement_H_

#include <Arduino.h>


#define PRINTFREQ     12
#define NUMCHANNELS   2

void startadc(void);
void stopadc(void);
void findDcBias(uint8_t flushruns = 0);
void sampleAudio(void);
void refreshAVG(void);
void refreshRMS(void);
void refreshPPM(void);
float avgBallistics(uint8_t ch);
float rmsBallistics(uint8_t ch);
float ppmBallistics(uint8_t ch);
void debugMeasurement(void);


#endif
