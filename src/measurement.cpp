#include "measurement.h"

#include <ADCInput.h>
#include "RunningMedian.h"
#include "TrueRMS.h"


#define INPUTGPIO_L     27
#define INPUTGPIO_R     28
#define OVRINPUTGPIO_L  2
#define OVRINPUTGPIO_R  3

#define ADCBUFFER       64
#define SAMPLERATE      48000
#define RMSWINDOW       SAMPLERATE/UPDATEFREQ
#define PPMFILTERBUF    10

#define INMAX           4096
#define FULLSCALE       INMAX/2
#define PPMNOISE        20    // ADC PPM BIAS noise floor   
#define L 0
#define R 1



// -------------------------------------------------------------------------------------

ADCInput adc(INPUTGPIO_R,INPUTGPIO_L);
Rms2 readRmsL; 
Rms2 readRmsR; 
RunningMedian ppmFiltL = RunningMedian(PPMFILTERBUF);
RunningMedian ppmFiltR = RunningMedian(PPMFILTERBUF);

uint8_t adcin[NUMCHANNELS];
uint8_t dcBias[NUMCHANNELS];
uint8_t maxval[NUMCHANNELS];
uint8_t rms[NUMCHANNELS];
uint8_t peak[NUMCHANNELS];
uint8_t prevpeak[NUMCHANNELS];



// -------------------------------------------------------------------------------------

void beginmeasurement(void) {
  readRmsL.begin(INMAX, RMSWINDOW, ADC_12BIT, BLR_OFF, CNT_SCAN);
  readRmsR.begin(INMAX, RMSWINDOW, ADC_12BIT, BLR_OFF, CNT_SCAN);
  readRmsL.start();
  readRmsR.start();
  ppmFiltL.clear();
  ppmFiltR.clear();
  startadc();
  delay(500);
  findDcBias(3);
}


void startadc(void) {
  adc.setFrequency(SAMPLERATE);
  adc.setBuffers(4, ADCBUFFER);
  adc.begin(); 
}


void stopadc(void) {
  adc.end();
}


void sampleAudio(void) {
  adcin[L] = adc.read();
  adcin[R] = adc.read();
  uint8_t left = constrain(abs(adcin[L]-dcBias[L]), 0, FULLSCALE);
  uint8_t rght = constrain(abs(adcin[R]-dcBias[R]), 0, FULLSCALE);
  readRmsL.update(left); 
  readRmsR.update(rght);
  if (left>maxval[L]) {
    if (left<PPMNOISE) { left=0; }   // Noise gate for ADC noise floor
    ppmFiltL.add(left);
    maxval[L] = left;
  }
  if (rght>maxval[R]) {
    if (rght<PPMNOISE) { rght=0; }  // Noise gate for ADC noise floor
    ppmFiltR.add(rght);
    maxval[R] = rght;
  }
}


void refreshRMS(void) {
  readRmsL.publish();
  readRmsR.publish();
  rms[L] = readRmsL.rmsVal;
  rms[R] = readRmsR.rmsVal;
}


void refreshPPM(void) {
  peak[L] = constrain((int)ppmFiltL.getAverage()*0.7079, 0, FULLSCALE);   // Quasi PPM at -3dB
  peak[R] = constrain((int)ppmFiltR.getAverage()*0.7079, 0, FULLSCALE);   // Quasi PPM at -3dB
  ppmFiltL.clear();
  ppmFiltR.clear();
  maxval[L] = 0;
  maxval[R] = 0;
}


uint8_t ppmBallistics(uint8_t ch) {
  #define DROPRATE 0.9085  // 0.9441 = -6dB per sec // 0.9085 = -10dB per sec // at 12Hz function call rate
  if (peak[ch]<(int)prevpeak[ch]*DROPRATE) { 
    prevpeak[ch] = max(0, (uint8_t)prevpeak[ch]*DROPRATE); 
  }
  else { 
    prevpeak[ch] = peak[ch]; 
  }
  return prevpeak[ch];
}


uint8_t vuBallistics(uint8_t ch) {
  float p_gain = 0.25;
  float i_gain = 0.25;
  int acc[NUMCHANNELS] = {0,0};
  int err[NUMCHANNELS] = {0,0};
  static int pos[NUMCHANNELS] = {0,0};
  
  err[ch] = rms[ch]-pos[ch];
  acc[ch] += i_gain*err[ch];
  pos[ch] += (int)(p_gain*err[ch] + acc[ch]);
  pos[ch]--;
  pos[ch] = constrain(pos[ch], 0, FULLSCALE);
  return pos[ch];
}


void findDcBias(int runs) {
  uint32_t sum[NUMCHANNELS] = {0,0};
  delay(50);
  for (int i=0; i<(SAMPLERATE*runs); i++ ) {  
    sum[L] += adc.read();
    sum[R] += adc.read();
  }
  dcBias[L] = (int)sum[L]/(SAMPLERATE*runs);
  dcBias[R] = (int)sum[R]/(SAMPLERATE*runs);
}
