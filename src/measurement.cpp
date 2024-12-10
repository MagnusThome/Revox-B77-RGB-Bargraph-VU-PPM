#include "measurement.h"

#include <ADCInput.h>
#include "RunningMedian.h"


#define INPUTGPIO_L     27
#define INPUTGPIO_R     28
#define OVRINPUTGPIO_L  2
#define OVRINPUTGPIO_R  3

#define SAMPLERATE      48000
#define ADCBUFFER       2048

#define PPMNOISECUTOFF  22
#define PPMNOISEOFFSET  20

#define USEPPMFILTER    0
#define PPMFILTERBUF    16

#define INMAX           4095
#define FULLSCALE       2047
#define L 0
#define R 1



// -------------------------------------------------------------------------------------

ADCInput adc(INPUTGPIO_R,INPUTGPIO_L);

RunningMedian ppmFiltL = RunningMedian(PPMFILTERBUF);
RunningMedian ppmFiltR = RunningMedian(PPMFILTERBUF);

int adcIn[NUMCHANNELS];
int dcBias[NUMCHANNELS];

int64_t avgSum[NUMCHANNELS] = {0,0};
int avgCntr = 0;
float avg[NUMCHANNELS];

int64_t rmsSum[NUMCHANNELS] = {0,0};
int rmsCntr = 0;
float rms[NUMCHANNELS];

int maxVal[NUMCHANNELS] = {0,0};
float peak[NUMCHANNELS];

int actualSampleRate = 0;



// -------------------------------------------------------------------------------------



void startadc(void) {
  ppmFiltL.clear();
  ppmFiltR.clear();
  adc.setFrequency(SAMPLERATE);
  adc.setBuffers(4, ADCBUFFER);
  adc.begin(); 
}


void stopadc(void) {
  adc.end();
}


void sampleAudio(void) {
  actualSampleRate = 0;
  while (adc.available() && actualSampleRate < SAMPLERATE/UPDATEFREQ) {
    adcIn[L] = adc.read();
    adcIn[R] = adc.read();
    int left = constrain(abs(adcIn[L]-dcBias[L]), 0, FULLSCALE);
    int rght = constrain(abs(adcIn[R]-dcBias[R]), 0, FULLSCALE);

    avgSum[L] += (int64_t)left;
    avgSum[R] += (int64_t)rght;
    avgCntr++;

    rmsSum[L] += (int64_t)(left*left);
    rmsSum[R] += (int64_t)(rght*rght);
    rmsCntr++;

    if (left>maxVal[L]) {
      if (left<PPMNOISECUTOFF) { left=0; }  // Noise gate for ADC noise floor
      ppmFiltL.add(left);
      maxVal[L] = left;
    }
    if (rght>maxVal[R]) {
      if (rght<PPMNOISECUTOFF) { rght=0; }  // Noise gate for ADC noise floor
      ppmFiltR.add(rght);
      maxVal[R] = rght;
    }
    actualSampleRate++;
  }
}


// AVG is 2*val/pi (0.6366) and RMS is val/sqr of 2 (0.7071). 
// The difference between AVG and RMS is 0.6366/0.7071 = 0.9003

void refreshAVG(void) {
  avg[L] = (avgSum[L]/avgCntr)/0.6366;
  avg[R] = (avgSum[R]/avgCntr)/0.6366;
  avgSum[L] = 0;
  avgSum[R] = 0;
  avgCntr = 0;
}


void refreshRMS(void) {
  rms[L] = sqrt(rmsSum[L]/rmsCntr)/0.7071;
  rms[R] = sqrt(rmsSum[R]/rmsCntr)/0.7071;
  rmsSum[L] = 0; 
  rmsSum[R] = 0; 
  rmsCntr = 0;
}


void refreshPPM(void) {
  if(USEPPMFILTER) {
    peak[L] = constrain(ppmFiltL.getMedian()-PPMNOISEOFFSET, 0, FULLSCALE);
    peak[R] = constrain(ppmFiltR.getMedian()-PPMNOISEOFFSET, 0, FULLSCALE);
    ppmFiltL.clear();
    ppmFiltR.clear();
  }
  else {
    peak[L] = maxVal[L]-PPMNOISEOFFSET;
    peak[R] = maxVal[R]-PPMNOISEOFFSET;
  }
  maxVal[L] = 0;
  maxVal[R] = 0;
}


float avgBallistics(uint8_t ch) {
  float p_gain = 0.25;
  float i_gain = 0.25;
  static float pos[NUMCHANNELS] = {0,0};
  float acc[NUMCHANNELS] = {0,0};
  float err[NUMCHANNELS] = {0,0};
  err[ch] = avg[ch]-pos[ch];
  acc[ch] += i_gain*err[ch];
  pos[ch] += (p_gain*err[ch] + acc[ch]);
  pos[ch] -= 0.1;
  pos[ch] = constrain(pos[ch], 0.0, FULLSCALE);
  return pos[ch];
}


float rmsBallistics(uint8_t ch) {
  float p_gain = 0.25;
  float i_gain = 0.25;
  static float pos[NUMCHANNELS] = {0,0};
  float acc[NUMCHANNELS] = {0,0};
  float err[NUMCHANNELS] = {0,0};
  err[ch] = rms[ch]-pos[ch];
  acc[ch] += i_gain*err[ch];
  pos[ch] += (p_gain*err[ch] + acc[ch]);
  pos[ch] -= 0.1;
  pos[ch] = constrain(pos[ch], 0.0, FULLSCALE);
  return pos[ch];
}


float ppmBallistics(uint8_t ch) {
  // x root of 0.3350 where x is number of calls to this function per second
  //#define DROPRATE 0.9730  // 0.9730 = -10dB per sec // at 40Hz function call rate
  #define DROPRATE 0.9085  // 0.9441 = -6dB per sec // 0.9085 = -10dB per sec // at 12Hz function call rate
  static float prevPeak[NUMCHANNELS] = {0,0};
  if (peak[ch]<prevPeak[ch]*DROPRATE) { 
    prevPeak[ch] = max(0, prevPeak[ch]*DROPRATE); 
  }
  else { 
    prevPeak[ch] = constrain(peak[ch], 0, FULLSCALE ); 
  }
  return prevPeak[ch];
}


void findDcBias(uint8_t flushruns) {
  uint8_t runs = 1 + flushruns;
  uint32_t sumL = 0;
  uint32_t sumR = 0;
  for (uint8_t i=0; i<runs; i++ ) {  
    sumL = 0;
    sumR = 0;
    for (uint32_t i=0; i<SAMPLERATE; i++ ) {  
      sumL += adc.read();
      sumR += adc.read();
    }
  }
  dcBias[L] = (int)sumL/SAMPLERATE;
  dcBias[R] = (int)sumR/SAMPLERATE;
}


void debugMeasurement(void) {
  Serial.printf("    offset: %3d %3d", dcBias[L]-(INMAX/2), dcBias[R]-(INMAX/2) );
  Serial.printf("    adc: %5d %5d", adcIn[L]-dcBias[L], adcIn[R]-dcBias[R] ); // just random single samples
  Serial.printf("    left: %6.0f %6.0f %6.0f", avg[L], rms[L], peak[L] );
  Serial.printf("    right: %6.0f %6.0f %6.0f", avg[R], rms[R], peak[R] );
  Serial.printf("    %5.1f kHz   ", (float)actualSampleRate*UPDATEFREQ/1000 );
}





// -----
