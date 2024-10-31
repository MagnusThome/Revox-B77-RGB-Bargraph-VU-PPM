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

#define LEDBARGPIO_L    15
#define LEDBARGPIO_R    14

#define INMAX           4096
#define FULLSCALE       INMAX/2
#define INZERODB        775
#define PPMNOISE        20    // ADC PPM noise floor   

// DISPLAY MODES
#define PPM_DOT_AND_VU_BAR 0
#define VU_BAR             1
#define VU_DOT             2
#define PPM_BAR            3
#define PPM_DOT            4
int displmode;
int colormode;
int scrsvmode;

#define EEPROMADDRVUPPM 0
#define EEPROMADDRCOLOR 2
#define EEPROMADDRSCRSV 4

#define NUMLEDS         36
#define LEFT            0
#define RGHT            1

CRGB led[2][NUMLEDS];
CRGB ledBAK[NUMLEDS];
CRGB ledDOT[NUMLEDS];
CRGB ledBAR[NUMLEDS];

#define ADCBUFFER    64
#define SAMPLERATE   48000
#define UPDATEFREQ   12
#define RMSWINDOW    SAMPLERATE/UPDATEFREQ
#define PPMFILTERBUF 10
#define SCRSAVERTIMEOUT 10 // minutes
#define SCRSAVERAUTO 0
#define SCRSAVERDEMO 1

ADCInput adc(INPUTGPIO_L,INPUTGPIO_R);
Rms2 readRmsL; 
Rms2 readRmsR; 
RunningMedian ppmFiltL = RunningMedian(PPMFILTERBUF);
RunningMedian ppmFiltR = RunningMedian(PPMFILTERBUF);
int adcL, adcR;
int dcBiasL, dcBiasR;
int maxL,  maxR;
int rmsL,  rmsR;
int peakL, peakR;
int ppmL,  ppmR;
int vuL,   vuR;

#define BUTTONGPIO 8
#define LONG_PRESS 500
#define PROGRAMMODETIMEOUT 15 // seconds
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
1.7783*INZERODB     // +5dB
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
  readRmsL.begin(INMAX, RMSWINDOW, ADC_12BIT, BLR_OFF, CNT_SCAN);
  readRmsR.begin(INMAX, RMSWINDOW, ADC_12BIT, BLR_OFF, CNT_SCAN);
  readRmsL.start();
  readRmsR.start();
  ppmFiltL.clear();
  ppmFiltR.clear();
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_L>(led[LEFT], NUMLEDS);
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_R>(led[RGHT], NUMLEDS);
  EEPROM.begin(256);
  myBtn.begin();
  displmode = EEPROM.read(EEPROMADDRVUPPM);
  colormode = EEPROM.read(EEPROMADDRCOLOR);
  scrsvmode = EEPROM.read(EEPROMADDRSCRSV);
  setcolors();
  adc.setFrequency(SAMPLERATE);
  adc.setBuffers(4, ADCBUFFER);
  adc.begin(); 
  delay(1500);
  findDcBias();
}

  


// -------------------------------------------------------------------------------------

void loop() {
  unsigned long loopnow = millis();
  sampleAudio();
  actualSampleRate++;

  static unsigned long looptimer;
  if (loopnow - looptimer >= 1000/UPDATEFREQ ) {  
    looptimer = loopnow;
    refreshPPM();
    refreshRMS();
    vuBallistics();
    ppmBallistics();
    if (!screensaver(SCRSAVERAUTO)) {
      updateLeds();
    }
    showmodenumber();
#ifdef DEBUG
    Serial.printf("%12d %4d", dcBiasL-2048, dcBiasR-2048 );
    Serial.printf("%12d %4d", adcL-dcBiasL, adcR-dcBiasR ); // just random single samples
    Serial.printf("%12d %4d %4d", rmsL, vuL, ppmL );
    Serial.printf("%12d %4d %4d", rmsR, vuR, ppmR );
    Serial.printf("%12.3f kHz", (float)actualSampleRate*UPDATEFREQ/2000 );
    Serial.printf("\t\t%d\t0\n", programmode);
#endif
    checkbutton();
    actualSampleRate=0;
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
  long sumL = 0;
  long sumR = 0;
  delay(500);
  for (int i=0; i<(SAMPLERATE*2); i++ ) {  
    sumL += adc.read();
    sumR += adc.read();
  }
  dcBiasL = (int)sumL/(SAMPLERATE*2);
  dcBiasR = (int)sumR/(SAMPLERATE*2);
}


// -------------------------------------------------------------------------------------

void updateLeds(void) {
  static int ppmDotL,  ppmDotR;
  static int vuDotL,  vuDotR;

  ppmDotL = 0;
  while (  ppmDotL<NUMLEDS  &&  ppmL>thresholds[ppmDotL] ) {
    ppmDotL++;
  }
  ppmDotL--;   // -1 (all off) and then 0 to 35
 
  ppmDotR = 0;
  while (  ppmDotR<NUMLEDS  &&  ppmR>thresholds[ppmDotR] ) {
    ppmDotR++;
  }
  ppmDotR--;   // -1 (all off) and then 0 to 35

  vuDotL = 0;
  while (  vuDotL<NUMLEDS  &&  vuL>thresholds[vuDotL] ) {
    vuDotL++;
  }
  vuDotL--;   // -1 (all off) and then 0 to 35
 
  vuDotR = 0;
  while (  vuDotR<NUMLEDS  &&  vuR>thresholds[vuDotR] ) {
    vuDotR++;
  }
  vuDotR--;   // -1 (all off) and then 0 to 35


     
  for(int pos=0; pos<NUMLEDS; pos++) { 

    switch (displmode) {

      case PPM_DOT:
        if (ppmDotL == pos) { led[LEFT][pos] = ledDOT[pos]; }
        else                { led[LEFT][pos] = ledBAK[pos]; }
        if (ppmDotR == pos) { led[RGHT][pos] = ledDOT[pos]; }
        else                { led[RGHT][pos] = ledBAK[pos]; }
        break;
    
      case PPM_BAR:
        if (ppmDotL >= pos) { led[LEFT][pos] = ledBAR[pos]; }
        else                { led[LEFT][pos] = ledBAK[pos]; }
        if (ppmDotR >= pos) { led[RGHT][pos] = ledBAR[pos]; }
        else                { led[RGHT][pos] = ledBAK[pos]; }
        break;
      
      case VU_DOT:
        if (vuDotL == pos)  { led[LEFT][pos] = ledDOT[pos]; }
        else                { led[LEFT][pos] = ledBAK[pos]; }
        if (vuDotR == pos)  { led[RGHT][pos] = ledDOT[pos]; }
        else                { led[RGHT][pos] = ledBAK[pos]; }
        break;
      
      case VU_BAR:
        if (vuDotL >= pos)  { led[LEFT][pos] = ledBAR[pos]; }
        else                { led[LEFT][pos] = ledBAK[pos]; }
        if (vuDotR >= pos)  { led[RGHT][pos] = ledBAR[pos]; }
        else                { led[RGHT][pos] = ledBAK[pos]; }
        break;
      
      case PPM_DOT_AND_VU_BAR:
        if (vuDotL >= pos)  { led[LEFT][pos] = ledBAR[pos]; }
        else                { led[LEFT][pos] = ledBAK[pos]; }
        if (ppmDotL == pos) { led[LEFT][pos] = ledDOT[pos]; }
        if (vuDotR >= pos)  { led[RGHT][pos] = ledBAR[pos]; }
        else                { led[RGHT][pos] = ledBAK[pos]; }
        if (ppmDotR == pos) { led[RGHT][pos] = ledDOT[pos]; }
        break;

      default:
        displmode = 0;
    }
  
  }
  
  FastLED.show();

}

// -------------------------------------------------------------------------------------

// CREATE DIFFERENT COLOR SETUPS
void setcolors(void) {  

  if (colormode>6) colormode = 0;

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
        ledBAK[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledBAR[pos] = ledBAK[pos];
        ledBAR[pos] %= 100;
        ledDOT[pos] = CRGB::Red;
        ledDOT[pos] %= 150;
        ledBAK[pos] %= 20;
        break;

      case 5:
        ledBAK[pos] = CRGB::DarkGray;
        ledBAR[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledBAR[pos] %= 50;
        ledDOT[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledDOT[pos] %= 255;
        ledBAK[pos] %= 5;
        break;

      case 6:
        ledBAK[pos] = CRGB::Black;
        ledBAR[pos].setHue(map(pos, 0, NUMLEDS, 300, -20)); 
        ledBAR[pos] %= 100;
        ledDOT[pos] = CRGB::White;
        ledDOT[pos] %= 150;
        ledBAK[pos] %= 10;
        break;
    }
  }
}


// -------------------------------------------------------------------------------------

void checkbutton(void) {
  static bool prevWasLong = false;
  unsigned long loopnow = millis();
  #define MAXPROGRAMMODES 3
  
  static unsigned long buttontimeout;
  if (  programmode>0  &&  loopnow-buttontimeout>=PROGRAMMODETIMEOUT*1000  ) {  
    flashleds(CRGB::DarkGrey);
    programmode = 0;
    delay(200);
  }

  myBtn.read();
  if (myBtn.wasReleased() && prevWasLong) {
    prevWasLong = false;
  }
  else if (myBtn.wasReleased()) {
    buttontimeout = loopnow;
    switch (programmode) {
  
      case 0:
        break;
        
      case 1:
        changecolor();
        break;
  
      case 2:
        changedisplmode();
        break;
      
      case 3:
        changescrsv();
        break;
      
      default:
        programmode = 0;
    }
  }
  
  if (myBtn.pressedFor(LONG_PRESS) && !prevWasLong) {
    prevWasLong = true;
    buttontimeout = loopnow;
    programmode++;
    if(programmode) {
      if(programmode==1) { led[LEFT][0] = CRGB::White; };
      showmodenumber();
      adc.end();  
      EEPROM.write(EEPROMADDRVUPPM, displmode);
      EEPROM.write(EEPROMADDRCOLOR, colormode);
      EEPROM.write(EEPROMADDRSCRSV, scrsvmode);
      EEPROM.commit();
      adc.setFrequency(SAMPLERATE);
      adc.setBuffers(4, ADCBUFFER);
      adc.begin(); 
      findDcBias();
    }
    if (programmode>MAXPROGRAMMODES) programmode = 0;
  }
}


void changedisplmode(void) {
  displmode++;
}

void changecolor(void) {
  colormode++;
  setcolors();
}


void changescrsv(void) {
  scrsvmode++;
  for (int i=0; i<200; i++) {
    screensaver(SCRSAVERDEMO);
  }
}


void showmodenumber(void) {
  if(!programmode) return;
  for (int pos=0; pos<=MAXPROGRAMMODES+3; pos++) {
    if ( pos == programmode-2 ) { led[LEFT][pos] = CRGB::Black; }
    if ( pos == programmode )   { led[LEFT][pos] = CRGB::White; }
  }
  FastLED.show();
}


// -------------------------------------------------------------------------------------

bool screensaver(bool demomode) {
  unsigned long loopnow = millis();
  static unsigned long looptimer;
  static bool wait = false;
  static bool initFade = true;


  if (demomode) delay(15);   // make led update frequency similar to live rate
  
  if ( !demomode  &&  (vuL+vuR>10) ) {
    wait = false;
    initFade = true;
    return false;
  }
  else if (!wait) {
    wait = true;
    looptimer = loopnow;
    return false;
  }
  else if ( demomode  ||  (loopnow - looptimer >= SCRSAVERTIMEOUT*60*1000) ) {  

    switch (scrsvmode) {
      
      case 0:
        return false;
        
      case 1:
        if(demomode) { scrsaverRainbow(0); }
        else         { scrsaverRainbow(initFade); }
        break;
        
      default:      
        scrsvmode = 0;
        return false;
    }
    initFade = false;
    return true;
  }  
  return false;
}


void fadetoblack(void) {
  for(int x=0; x<50; x++) {
    for(int i=0; i<NUMLEDS; i++) {
      led[LEFT][i].fadeToBlackBy(1);
      led[RGHT][i].fadeToBlackBy(1);
    }
    FastLED.show();
    delay(65);
  }
}


void scrsaverRainbow(bool initFade) {
  static uint8_t color = 0;
  static uint8_t brghtn;
  
  if (initFade) { 
    fadetoblack();    
    brghtn=0; 
  }
  else {
    brghtn=50; 
  }
  const float rainbowSpread = 1.7;
  for(int i=0; i<NUMLEDS; i++) {
    led[LEFT][i].setHue(color+(int)(i*rainbowSpread));
    led[LEFT][i] %= brghtn;
    led[RGHT][i].setHue(color+(int)(i*rainbowSpread)+(int)(NUMLEDS*rainbowSpread));
    led[RGHT][i] %= brghtn;
  }
  FastLED.show();
  delay(3);
  brghtn++;
  brghtn = constrain(brghtn, 0, 50);  color++;
}


// -------------------------------------------------------------------------------------

void flashleds(long color) {
  for (int i=0; i<NUMLEDS; i++) {
    led[LEFT][i] = color;
    led[LEFT][i] %= 30;
    led[RGHT][i] = color;
    led[RGHT][i] %= 30;
  }
  FastLED.show();
  delay(40);
  for (int i=0; i<NUMLEDS; i++) {
    led[LEFT][i] = 0x000000;
    led[RGHT][i] = 0x000000;
  }
  FastLED.show();
  delay(40);
}


// --------- THE END -------------------------------------------------------------------
