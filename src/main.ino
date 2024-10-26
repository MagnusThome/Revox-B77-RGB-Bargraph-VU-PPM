#include <FastLED.h>
#include <ADCInput.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include "RunningMedian.h"
#include "TrueRMS.h"

#define DEBUG

#define INPUTGPIO_L     27
#define INPUTGPIO_R     28
#define OVRINPUTGPIO_L  2
#define OVRINPUTGPIO_R  3

#define LEDBARGPIO_L    14
#define LEDBARGPIO_R    15

#define INMAX           4096
#define FULLSCALE       INMAX/2
#define INZERODB        775
#define PPMNOISE        20    // ADC PPM noise floor   

#define NUMLEDS         36

// DISPLAY MODES
#define PPM_DOT_AND_VU_BAR 0
#define VU_BAR             1
#define VU_DOT             2
#define PPM_BAR            3
#define PPM_DOT            4
int displaymode;
int colormode;

#define EEPROMADDRMODE  0
#define EEPROMADDRCOLOR 2

CRGB ledL[NUMLEDS];
CRGB ledR[NUMLEDS];
CRGB ledBAK[NUMLEDS];
CRGB ledDOT[NUMLEDS];
CRGB ledBAR[NUMLEDS];

#define SAMPLERATE   48000
#define UPDATEFREQ   12
#define RMS_WINDOW   SAMPLERATE/UPDATEFREQ
#define BIASBUF      255
#define PPMFILTERBUF 10

ADCInput adc(INPUTGPIO_L,INPUTGPIO_R);
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

#define BUTTONGPIO 8
#define LONG_PRESS 1000
Button myBtn(BUTTONGPIO);
int programmode = 0;

int actualSampleRate;

float thresholds[NUMLEDS] = {
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
1.7783*INZERODB    // +5dB
};




// -------------------------------------------------------------------------------------

void setup() {
  delay(500);
#ifdef DEBUG
  Serial.begin(115200);
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
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_L>(ledL, NUMLEDS);
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_R>(ledR, NUMLEDS);
  adc.begin(SAMPLERATE); 
  findDcBias();
  EEPROM.begin(256);
  myBtn.begin();
  displaymode = EEPROM.read(EEPROMADDRMODE);
  colormode = EEPROM.read(EEPROMADDRCOLOR);
  setcolors();
}

  


// -------------------------------------------------------------------------------------

void loop() {
  unsigned long loopnow = millis();
  sampleAudio();
  actualSampleRate++;

  while(programmode==2) {
    testleds();
    checkbutton();
  }

  // UPDATE LEDS
  static unsigned long timer1;
  if (loopnow - timer1 >= 1000/UPDATEFREQ ) {  
    timer1 = loopnow;
    refreshPPM();
    refreshRMS();
    vuBallistics();
    ppmBallistics();
    updateLeds();
#ifdef DEBUG
    Serial.printf("%12d %4d", adcL-dcBiasL, adcR-dcBiasR ); // just random single samples
    Serial.printf("%12d %4d %4d", ppmL, rmsL, vuL );
    Serial.printf("%12d %4d %4d", ppmR, rmsR, vuR );
    Serial.printf("%12.3f kHz", (float) actualSampleRate*UPDATEFREQ/2000 );
    Serial.println("\t\t\t0");
#endif
    actualSampleRate=0;
    checkbutton();
  }
}


// -------------------------------------------------------------------------------------

void sampleAudio(void) {
  adcL = adc.read();
  adcR = adc.read();
  int left = constrain(abs(adcL-dcBiasL), 0, FULLSCALE);
  int rght = constrain(abs(adcR-dcBiasR), 0, FULLSCALE);
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
  peakL = constrain((int)ppmFiltL.getAverage()*0.7079, 0, FULLSCALE);   // Quasi PPM at -3dB
  peakR = constrain((int)ppmFiltR.getAverage()*0.7079, 0, FULLSCALE);   // Quasi PPM at -3dB
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
  posL = constrain(posL, 0, FULLSCALE);
  posR = constrain(posR, 0, FULLSCALE);
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
  static int vuDotL,  vuDotR;

  ppmDotL = 0;
  while (  ppmDotL<NUMLEDS  &&  ppmL>thresholds[ppmDotL] ) {
    ppmDotL++;
  }
  ppmDotL--;   // from -1 (nothing) to 35
 
  ppmDotR = 0;
  while (  ppmDotR<NUMLEDS  &&  ppmR>thresholds[ppmDotR] ) {
    ppmDotR++;
  }
  ppmDotR--;   // from -1 (nothing) to 35

  vuDotL = 0;
  while (  vuDotL<NUMLEDS  &&  vuL>thresholds[vuDotL] ) {
    vuDotL++;
  }
  vuDotL--;   // from -1 (nothing) to 35
 
  vuDotR = 0;
  while (  vuDotR<NUMLEDS  &&  vuR>thresholds[vuDotR] ) {
    vuDotR++;
  }
  vuDotR--;   // from -1 (nothing) to 35


     
  for(int pos=0; pos<NUMLEDS; pos++) { 

    switch (displaymode) {

      case PPM_DOT:
        if (ppmDotL == pos) { ledL[pos] = ledDOT[pos]; }
        else                { ledL[pos] = ledBAK[pos]; }
        if (ppmDotR == pos) { ledR[pos] = ledDOT[pos]; }
        else                { ledR[pos] = ledBAK[pos]; }
        break;
    
      case PPM_BAR:
        if (ppmDotL >= pos) { ledL[pos] = ledBAR[pos]; }
        else                { ledL[pos] = ledBAK[pos]; }
        if (ppmDotR >= pos) { ledR[pos] = ledBAR[pos]; }
        else                { ledR[pos] = ledBAK[pos]; }
        break;
      
      case VU_DOT:
        if (vuDotL == pos)  { ledL[pos] = ledDOT[pos]; }
        else                { ledL[pos] = ledBAK[pos]; }
        if (vuDotR == pos)  { ledR[pos] = ledDOT[pos]; }
        else                { ledR[pos] = ledBAK[pos]; }
        break;
      
      case VU_BAR:
        if (vuDotL >= pos)  { ledL[pos] = ledBAR[pos]; }
        else                { ledL[pos] = ledBAK[pos]; }
        if (vuDotR >= pos)  { ledR[pos] = ledBAR[pos]; }
        else                { ledR[pos] = ledBAK[pos]; }
        break;
      
      case PPM_DOT_AND_VU_BAR:
        if (vuDotL >= pos)  { ledL[pos] = ledBAR[pos]; }
        else                { ledL[pos] = ledBAK[pos]; }
        if (vuDotR >= pos)  { ledR[pos] = ledBAR[pos]; }
        else                { ledR[pos] = ledBAK[pos]; }
        if (ppmDotL == pos) { ledL[pos] = ledDOT[pos]; }
        if (ppmDotR == pos) { ledR[pos] = ledDOT[pos]; }
        break;

      default:
        displaymode = 0;
    }
  
  }
  
  FastLED.show();

}

// -------------------------------------------------------------------------------------

// CREATE DIFFERENT COLOR SETUPS
void setcolors(void) {  

  for(int pos=0; pos<NUMLEDS; pos++) { 
    switch (colormode) {

      case 0:
        if (pos>29)      { ledBAK[pos] = CRGB::Red;    }
        else if (pos>24) { ledBAK[pos] = CRGB::Yellow; }
        else             { ledBAK[pos] = CRGB::Green;  }
        ledBAR[pos] = ledBAK[pos];
        ledBAR[pos] %= 150;
        ledDOT[pos] = CRGB::Cyan;
        ledDOT[pos] %= 80;
        ledBAK[pos] %= 20;
        break;
        
      case 1:
        if (pos>29)      { ledBAK[pos] = CRGB::Red;    }
        else if (pos>24) { ledBAK[pos] = CRGB::Yellow; }
        else             { ledBAK[pos] = CRGB::Green;  }
        ledBAR[pos] = ledBAK[pos];
        ledBAR[pos] %= 80;
        ledDOT[pos] = ledBAK[pos];
        ledDOT[pos] %= 180;
        ledBAK[pos] %= 10;
        break;

      case 2:
        if (pos>29)      { ledBAK[pos] = CRGB::Red;    }
        else if (pos>24) { ledBAK[pos] = CRGB::Yellow; }
        else             { ledBAK[pos] = CRGB::Green;  }
        ledBAR[pos] = CRGB::White; 
        ledBAR[pos] %= 30;
        ledDOT[pos] = ledBAK[pos];
        ledDOT[pos] %= 180;
        ledBAK[pos] %= 30;
        break;

      case 3:
        if (pos>29)      { ledBAK[pos] = CRGB::Purple;    }
        else if (pos>24) { ledBAK[pos] = CRGB::Teal; }
        else             { ledBAK[pos] = CRGB::DarkGray;  }
        ledBAR[pos] = ledBAK[pos];
        ledBAR[pos] %= 70;
        ledDOT[pos] = CRGB::Red;
        ledDOT[pos] %= 60;
        ledBAK[pos] %= 20;
        break;
        
      case 4:
        ledBAK[pos] = CRGB::DarkGray;
        ledBAR[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledBAR[pos] %= 50;
        ledDOT[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledDOT[pos] %= 255;
        ledBAK[pos] %= 5;
        break;

      case 5:
        ledBAK[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledBAR[pos] = ledBAK[pos];
        ledBAR[pos] %= 100;
        ledDOT[pos] = CRGB::Red;
        ledDOT[pos] %= 150;
        ledBAK[pos] %= 20;
        break;

      case 6:
        ledBAK[pos] = CRGB::Black;
        ledBAR[pos].setHue(map(pos, 0, NUMLEDS, 300, -20)); 
        ledBAR[pos] %= 100;
        ledDOT[pos] = CRGB::White;
        ledDOT[pos] %= 150;
        ledBAK[pos] %= 10;
        break;

      default:
        colormode = 0;

    }
  }
}


// -------------------------------------------------------------------------------------

void checkbutton() {
  static bool longpressed = false;
  
  myBtn.read();
  if (myBtn.wasReleased() && longpressed) {
    longpressed = false;
  }
  else if (myBtn.wasReleased()) {
    switch (programmode) {
  
      case 0:
        flashleds(CRGB::Gray);
        changecolor();
        break;
  
      case 1:
        flashleds(CRGB::Blue);
        changedisplaymode();
        break;
      
      default:
        programmode = 0;
    }
  }
  
  if (myBtn.pressedFor(LONG_PRESS) && !longpressed) {
    longpressed = true;
    flashleds(CRGB::White);
    flashleds(CRGB::White);
    programmode++;
  }
}


void changedisplaymode(void) {
  displaymode++;
  adc.end(); 
  EEPROM.write(EEPROMADDRMODE, displaymode);
  EEPROM.commit();
  adc.begin(SAMPLERATE); 
}

void changecolor(void) {
  colormode++;
  adc.end(); 
  EEPROM.write(EEPROMADDRCOLOR, colormode);
  EEPROM.commit();
  adc.begin(SAMPLERATE); 
  setcolors();
}


void flashleds(long color) {
  for (int i=0; i<NUMLEDS; i++) {
    ledL[i] = color;
    ledL[i] %= 100;
    ledR[i] = color;
    ledR[i] %= 100;
  }
  FastLED.show();
  delay(50);
  for (int i=0; i<NUMLEDS; i++) {
    ledL[i] = 0x000000;
    ledR[i] = 0x000000;
  }
  FastLED.show();
  delay(50);
}


void testleds(void) {
  static uint8_t color=0;
  for(int i=0; i<NUMLEDS; i++) {
    ledL[i].setHue(color+(i*3));
    ledR[i].setHue(color+(i*3)+(NUMLEDS*3));
  }
  FastLED.show();
  delay(20);
  color++;
}



// --------- THE END -------------------------------------------------------------------
