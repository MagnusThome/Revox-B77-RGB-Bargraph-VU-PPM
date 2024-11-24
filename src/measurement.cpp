#include "measurement.h"

#include <ADCInput.h>
#include "RunningMedian.h"


#define INPUTGPIO_L     27
#define INPUTGPIO_R     28
#define OVRINPUTGPIO_L  2
#define OVRINPUTGPIO_R  3

#define SAMPLERATE      48000
#define ADCBUFFER       2048

#define PPMFILTERBUF    16
#define PPMNOISE        16  // ADC PPM BIAS noise floor   

#define INMAX           4096
#define FULLSCALE       INMAX/2
#define L 0
#define R 1



// -------------------------------------------------------------------------------------

ADCInput adc(INPUTGPIO_R,INPUTGPIO_L);

RunningMedian ppmFiltL = RunningMedian(PPMFILTERBUF);
RunningMedian ppmFiltR = RunningMedian(PPMFILTERBUF);

int adcIn[NUMCHANNELS];
int dcBias[NUMCHANNELS];

long avgSum[NUMCHANNELS] = {0,0};
int avgCntr = 0;
int avg[NUMCHANNELS];

int64_t rmsSum[NUMCHANNELS] = {0,0};
int rmsCntr = 0;
int rms[NUMCHANNELS];

int maxVal[NUMCHANNELS] = {0,0};
int peak[NUMCHANNELS];

int actualSampleRate = 0;



// -------------------------------------------------------------------------------------



void startadc(void) {
  ppmFiltL.clear();
  ppmFiltR.clear();
  adc.setFrequency(SAMPLERATE);
  adc.setBuffers(4, ADCBUFFER);
  adc.begin(); 
  delay(500);
  sampleAudio();
  findDcBias(3);
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

    avgSum[L] += left;
    avgSum[R] += rght;
    avgCntr++;

    rmsSum[L] += left*left;
    rmsSum[R] += rght*rght;
    rmsCntr++;

    if (left>maxVal[L]) {
      if (left<PPMNOISE) { left=0; }  // Noise gate for ADC noise floor
      ppmFiltL.add(left);
      maxVal[L] = left;
    }
    if (rght>maxVal[R]) {
      if (rght<PPMNOISE) { rght=0; }  // Noise gate for ADC noise floor
      ppmFiltR.add(rght);
      maxVal[R] = rght;
    }
    actualSampleRate++;
  }
}


void refreshAVG(void) {
  avg[L] = avgSum[L]/avgCntr;
  avg[R] = avgSum[R]/avgCntr;
  avgSum[L] = 0;
  avgSum[R] = 0;
  avgCntr = 0;
}


void refreshRMS(void) {
  rms[L] = sqrt(rmsSum[L]/rmsCntr);
  rms[R] = sqrt(rmsSum[R]/rmsCntr);
  rmsSum[L] = 0; 
  rmsSum[R] = 0; 
  rmsCntr = 0;
}


void refreshPPM(void) {
  peak[L] = constrain((int)ppmFiltL.getMedian()*0.7079, 0, FULLSCALE);   // Quasi PPM at -3dB
  peak[R] = constrain((int)ppmFiltR.getMedian()*0.7079, 0, FULLSCALE);   // Quasi PPM at -3dB
  ppmFiltL.clear();
  ppmFiltR.clear();
  maxVal[L] = 0;
  maxVal[R] = 0;
}


int avgBallistics(uint8_t ch) {
  float p_gain = 0.25;
  float i_gain = 0.25;
  static int pos[NUMCHANNELS] = {0,0};
  int acc[NUMCHANNELS] = {0,0};
  int err[NUMCHANNELS] = {0,0};
  err[ch] = avg[ch]-pos[ch];
  acc[ch] += i_gain*err[ch];
  pos[ch] += (int)(p_gain*err[ch] + acc[ch]);
  pos[ch]--;
  pos[ch] = constrain(pos[ch], 0, FULLSCALE);
  return pos[ch];
}


int rmsBallistics(uint8_t ch) {
  float p_gain = 0.25;
  float i_gain = 0.25;
  static int pos[NUMCHANNELS] = {0,0};
  int acc[NUMCHANNELS] = {0,0};
  int err[NUMCHANNELS] = {0,0};
  err[ch] = rms[ch]-pos[ch];
  acc[ch] += i_gain*err[ch];
  pos[ch] += (int)(p_gain*err[ch] + acc[ch]);
  pos[ch]--;
  pos[ch] = constrain(pos[ch], 0, FULLSCALE);
  return pos[ch];
}


int ppmBallistics(uint8_t ch) {
  #define DROPRATE 0.9085  // 0.9441 = -6dB per sec // 0.9085 = -10dB per sec // at 12Hz function call rate
  // x root of 0.3350 where x is number of calls to this function per second
  static int prevPeak[NUMCHANNELS] = {0,0};
  if (peak[ch]<(int)prevPeak[ch]*DROPRATE) { 
    prevPeak[ch] = max(0, (int)prevPeak[ch]*DROPRATE); 
  }
  else { 
    prevPeak[ch] = constrain(peak[ch], 0, FULLSCALE ); 
  }
  return prevPeak[ch];
}


void findDcBias(uint8_t runs) {
  uint32_t sumL = 0;
  uint32_t sumR = 0;
  delay(100);
  for (uint32_t i=0; i<(SAMPLERATE*runs); i++ ) {  
    sumL += adc.read();
    sumR += adc.read();
  }
  dcBias[L] = (int)sumL/(SAMPLERATE*runs);
  dcBias[R] = (int)sumR/(SAMPLERATE*runs);
}


void debugMeasurement(void) {
  Serial.printf("    offset: %3d %3d", dcBias[L]-(INMAX/2), dcBias[R]-(INMAX/2) );
  Serial.printf("    adc: %5d %5d", adcIn[L]-dcBias[L], adcIn[R]-dcBias[R] ); // just random single samples
  Serial.printf("    avg: %4d %4d", avg[L], avg[R] );
  Serial.printf("    rms: %4d %4d", rms[L], rms[R] );
  Serial.printf("    peak: %4d %4d", peak[L], peak[R] );
  Serial.printf("    %5.1f kHz   ", (float)actualSampleRate*UPDATEFREQ/1000 );
}
