#include <FastLED.h>
#include <ADCInput.h>
#include "RunningMedian.h"
#include "TrueRMS.h"

//#define DEBUG

#define LEDBARGPIO_L    14
#define POSINPUTGPIO_L  27
#define NEGINPUTGPIO_L  26
#define OVRINPUTGPIO_L  2

#define LEDBARGPIO_R    15
#define POSINPUTGPIO_R  28
#define NEGINPUTGPIO_R  29
#define OVRINPUTGPIO_R  3

#define BUTTONGPIO      8

#define INMAX           4096
#define INZERODB        775
#define PPMNOISE        20    // ADC PPM noise floor   

#define NUM_LEDS        36

// DISPLAY MODES
#define PPM_DOT            0
#define PPM_BAR            1
#define VU_BAR             2
#define PPM_DOT_AND_VU_BAR 3

// COLOR SETS
#define DOT          0
#define BAR          1
#define DOT_AND_BAR  2

CRGB ledL[NUM_LEDS];
CRGB ledR[NUM_LEDS];
CRGB ledBAK[NUM_LEDS];
CRGB ledPPM[NUM_LEDS];
CRGB ledRMS[NUM_LEDS];

#define SAMPLERATE   48000
#define PRINTFREQ    12
#define RMS_WINDOW   SAMPLERATE/PRINTFREQ
#define BIASBUF      255
#define PPMFILTERBUF 10

ADCInput adc(POSINPUTGPIO_L,POSINPUTGPIO_R);
Rms2 readRmsL; 
Rms2 readRmsR; 
RunningMedian adcDcBiasL = RunningMedian(BIASBUF);
RunningMedian adcDcBiasR = RunningMedian(BIASBUF);
RunningMedian ppmFiltL   = RunningMedian(PPMFILTERBUF);
RunningMedian ppmFiltR   = RunningMedian(PPMFILTERBUF);
int adcL, adcR;
int dcBiasL, dcBiasR;
int maxL,  maxR;
int rmsL,  rmsR;
int peakL, peakR;
int ppmL,  ppmR;
int vuL,   vuR;

int displaymode;
int actualSampleRate;

float thresholds[NUM_LEDS+1] = {
0.0100*INZERODB,    // -40dB
0.0126*INZERODB,    // -38dB
0.0158*INZERODB,    // -36dB
0.0200*INZERODB,    // -34dB
0.0251*INZERODB,    // -32dB
0.0316*INZERODB,    // -30dB
0.0398*INZERODB,    // -28dB
0.0501*INZERODB,    // -26dB
0.0631*INZERODB,    // -24dB
0.0794*INZERODB,    // -22dB
0.1000*INZERODB,    // -20dB
0.1122*INZERODB,    // -19dB
0.1259*INZERODB,    // -18dB
0.1413*INZERODB,    // -17dB
0.1585*INZERODB,    // -16dB
0.1778*INZERODB,    // -15dB
0.1995*INZERODB,    // -14dB
0.2239*INZERODB,    // -13dB
0.2512*INZERODB,    // -12dB
0.2818*INZERODB,    // -11dB
0.3162*INZERODB,    // -10dB
0.3548*INZERODB,    // -9dB
0.3981*INZERODB,    // -8dB
0.4467*INZERODB,    // -7dB
0.5012*INZERODB,    // -6dB
0.5623*INZERODB,    // -5dB
0.6310*INZERODB,    // -4dB
0.7079*INZERODB,    // -3dB
0.7943*INZERODB,    // -2dB
0.8913*INZERODB,    // -1dB
1.0000*INZERODB,    // 0dB
1.1220*INZERODB,    // +1dB
1.2589*INZERODB,    // +2dB
1.4126*INZERODB,    // +3dB
1.5849*INZERODB,    // +4dB
1.7783*INZERODB,    // +5dB
1.9953*INZERODB     // +6dB
};




// -------------------------------------------------------------------------------------

void setup() {
  delay(500);
#ifdef DEBUG
  begin(115200);
  delay(500);
  Serial.println("Boot VU meter");
#endif
  pinMode(BUTTONGPIO, INPUT_PULLUP);
  readRmsL.begin(INMAX, RMS_WINDOW, ADC_12BIT, BLR_OFF, CNT_SCAN);
  readRmsL.start();
  readRmsR.begin(INMAX, RMS_WINDOW, ADC_12BIT, BLR_OFF, CNT_SCAN);
  readRmsR.start();
  ppmFiltL.clear();
  ppmFiltR.clear();
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_L>(ledL, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_R>(ledR, NUM_LEDS);
  adc.begin(SAMPLERATE); 
  findDcBias();
  displaymode = PPM_DOT_AND_VU_BAR;
  setcolors(DOT_AND_BAR);
}

  


// -------------------------------------------------------------------------------------

void loop() {
  unsigned long loopnow = millis();
  sampleAudio();
  actualSampleRate++;

  static unsigned long timer2;
  if (loopnow - timer2 >= 1000/PRINTFREQ ) {   // PRINTFREQ
    timer2 = loopnow;
    refreshPPM();
    refreshRMS();
    vuBallistics();
    ppmBallistics();
    updateLeds();
#ifdef DEBUG
    //Serial.printf("%12d %4d", adcL-dcBiasL, adcR-dcBiasR ); // just random single samples
    Serial.printf("%12d %4d %4d", ppmL, rmsL, vuL );
    Serial.printf("%12d %4d %4d", ppmR, rmsR, vuR );
    //Serial.printf("%12.3f kHz", (float) actualSampleRate*PRINTFREQ/2000 );
    Serial.println("\t\t\t0");
#endif
    actualSampleRate=0;
  }

}


// -------------------------------------------------------------------------------------


void sampleAudio(void) {
  adcL = adc.read();
  adcR = adc.read();
  int left = constrain(abs(adcL-dcBiasL), 0, INMAX/2);
  int rght = constrain(abs(adcR-dcBiasR), 0, INMAX/2);
  readRmsL.update(left); 
  readRmsR.update(rght);
  if (left>maxL) {
    if (left<PPMNOISE) { left=0; }   // Noise gate for ADC noise floor
    ppmFiltL.add(left);
    maxL = left;
  }
  if (rght>maxR) {
    if (rght<PPMNOISE) { rght=0; }  // Noise gate for ADC noise floor
    ppmFiltR.add(rght);
    maxR = rght;
  }
}


void refreshRMS(void) {
  readRmsL.publish();
  readRmsR.publish();
  rmsL = readRmsL.rmsVal;
  rmsR = readRmsR.rmsVal;
}


void refreshPPM(void) {
  peakL = (int)ppmFiltL.getAverage()*0.7079;   // Quasi PPM at -3dB
  peakR = (int)ppmFiltR.getAverage()*0.7079;   // Quasi PPM at -3dB
  ppmFiltL.clear();
  ppmFiltR.clear();
  maxL = 0;
  maxR = 0;
}


void ppmBallistics(void) {
  #define DROPRATE 0.9085  // 0.9441 = -6dB per sec // 0.9085 = -10dB per sec // at 12Hz function call rate
  if (peakL<(int)ppmL*DROPRATE) { ppmL = max(0, (int)ppmL*DROPRATE); }
  else                          { ppmL = peakL; }
  if (peakR<(int)ppmR*DROPRATE) { ppmR = max(0, (int)ppmR*DROPRATE); }
  else                          { ppmR = peakR; }
}


void vuBallistics(void) {
  float p_gain = 0.25;
  float i_gain = 0.25;
  int accL=0,  accR=0;
  int errL=0,  errR=0;
  static int posL=0, posR=0;
  errL=rmsL-posL;
  errR=rmsR-posR;
  accL+=i_gain*errL;
  accR+=i_gain*errR;
  posL+=(int)(p_gain*errL+accL);
  posR+=(int)(p_gain*errR+accR);
  posL = constrain(posL, 0, INMAX/2);
  posR = constrain(posR, 0, INMAX/2);
  vuL = posL;
  vuR = posR;
}


void findDcBias(void) {
  for (int i=0; i<BIASBUF*100; i++ ) {  
    adcDcBiasL.add(adc.read());
    adcDcBiasR.add(adc.read());
  }
  dcBiasL = (int)adcDcBiasL.getAverage();
  dcBiasR = (int)adcDcBiasR.getAverage();
#ifdef DEBUG
  Serial.printf("dcBias %d %d\n", dcBiasL, dcBiasR );
#endif
}


// -------------------------------------------------------------------------------------

void updateLeds(void) {
  
  static int ppmDotL,  ppmDotR;
  static int avgDotL,  avgDotR;

  ppmDotL = 0;
  while (  ppmDotL<NUM_LEDS  &&  ppmL>thresholds[ppmDotL] ) {
    ppmDotL++;
  }
  ppmDotL--;   // from -1 (nothing) to 35
 
  ppmDotR = 0;
  while (  ppmDotR<NUM_LEDS  &&  ppmR>thresholds[ppmDotR] ) {
    ppmDotR++;
  }
  ppmDotR--;   // from -1 (nothing) to 35

  avgDotL = 0;
  while (  avgDotL<NUM_LEDS  &&  vuL>thresholds[avgDotL] ) {
    avgDotL++;
  }
  avgDotL--;   // from -1 (nothing) to 35
 
  avgDotR = 0;
  while (  avgDotR<NUM_LEDS  &&  vuR>thresholds[avgDotR] ) {
    avgDotR++;
  }
  avgDotR--;   // from -1 (nothing) to 35


     
  for(int dot=0; dot<NUM_LEDS; dot++) { 

    switch (displaymode) {

      case PPM_DOT:
        if (ppmDotL == dot) { ledL[dot] = ledPPM[dot]; }
        else                { ledL[dot] = ledBAK[dot]; }
        if (ppmDotR == dot) { ledR[dot] = ledPPM[dot]; }
        else                { ledR[dot] = ledBAK[dot]; }
        break;
    
      case PPM_BAR:
        if (ppmDotL >= dot) { ledL[dot] = ledPPM[dot]; }
        else                { ledL[dot] = ledBAK[dot]; }
        if (ppmDotR >= dot) { ledR[dot] = ledPPM[dot]; }
        else                { ledR[dot] = ledBAK[dot]; }
        break;
      
      case VU_BAR:
        if (avgDotL >= dot) { ledL[dot] = ledRMS[dot]; }
        else                { ledL[dot] = ledBAK[dot]; }
        if (avgDotR >= dot) { ledR[dot] = ledRMS[dot]; }
        else                { ledR[dot] = ledBAK[dot]; }
        break;
      
      case PPM_DOT_AND_VU_BAR:
        if (avgDotL >= dot) { ledL[dot] = ledRMS[dot]; }
        else                { ledL[dot] = ledBAK[dot]; }
        if (avgDotR >= dot) { ledR[dot] = ledRMS[dot]; }
        else                { ledR[dot] = ledBAK[dot]; }
        if (ppmDotL == dot) { ledL[dot] = ledPPM[dot]; }
        if (ppmDotR == dot) { ledR[dot] = ledPPM[dot]; }
        break;
      
    }
  
  }
  
  FastLED.show();

}




// CREATE DIFFERENT COLOR SETUPS
void setcolors(int set) {  

  for(int dot=0; dot<NUM_LEDS; dot++) { 
    switch (set) {

      case DOT:
        if (dot>29)      { ledBAK[dot] = CRGB::Red;    }
        else if (dot>24) { ledBAK[dot] = CRGB::Yellow; }
        else             { ledBAK[dot] = CRGB::Green;  }
        ledRMS[dot] = ledBAK[dot];
        ledRMS[dot] %= 100;
        ledPPM[dot] = CRGB::Cyan;
        ledPPM[dot] %= 100;
        ledBAK[dot] %= 30;
        break;
        
      case BAR:
        if (dot>29)      { ledBAK[dot] = CRGB::Red;    }
        else if (dot>24) { ledBAK[dot] = CRGB::Yellow; }
        else             { ledBAK[dot] = CRGB::Green;  }
        ledRMS[dot] = ledBAK[dot];
        ledRMS[dot] %= 100;
        ledPPM[dot] = ledBAK[dot];
        ledPPM[dot] %= 60;
        ledBAK[dot] %= 10;
        break;

      case DOT_AND_BAR:
        if (dot>29)      { ledBAK[dot] = CRGB::Red;    }
        else if (dot>24) { ledBAK[dot] = CRGB::Yellow; }
        else             { ledBAK[dot] = CRGB::DarkGreen;  }
        ledRMS[dot] = ledBAK[dot];
        ledRMS[dot] %= 150;
        ledPPM[dot] = CRGB::White;
        ledPPM[dot] %= 100;
        ledBAK[dot] %= 15;
        break;
    }
  }
  
}



// -------------
