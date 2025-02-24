#include "display.h"
#include "displaysteps.h"
#include "measurement.h"
#include <EEPROM.h>


#define LEDBARGPIO_L  15
#define LEDBARGPIO_R  14

#define NUMCHANNELS   2
#define LEFT 0
#define RGHT 1

#define PPM_DOT_AND_VU_BAR 0
#define VU_BAR             1
#define VU_DOT             2
#define PPM_BAR            3
#define PPM_DOT            4

#define EEPROMADDRVUPPM 0
#define EEPROMADDRCOLOR 2
#define EEPROMADDRSCRSV 4
#define EEPROMADDRDIMMR 6


CRGB led[NUMCHANNELS][NUMLEDS];
CRGB ledBAK[NUMLEDS];
CRGB ledDOT[NUMLEDS];
CRGB ledBAR[NUMLEDS];

uint8_t displmode;
uint8_t colormode;
int dimmer;
int scrsvmode;



void begindisplay(void) {
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_L>(led[LEFT], NUMLEDS);
  FastLED.addLeds<NEOPIXEL, LEDBARGPIO_R>(led[RGHT], NUMLEDS);
  FastLED.setBrightness(127+dimmer);
  setcolors();
  updateLeds(0,0,0,0);
}


void updateLeds(float vuL, float vuR, float ppmL, float ppmR) {
  static int ppmDotL,  ppmDotR;
  static int vuDotL,  vuDotR;

  FastLED.setBrightness(127+dimmer);

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
        ledDOT[pos] = CRGB::Cyan;
        ledDOT[pos] %= 140;
        ledBAK[pos] %= 40;
        break;
        
      case 1:
        if (pos>29)      { ledBAK[pos] = CRGB::Red;    }
        else if (pos>24) { ledBAK[pos] = CRGB::Yellow; }
        else             { ledBAK[pos] = CRGB::Green;  }
        ledBAR[pos] = ledBAK[pos];
        ledDOT[pos] = ledBAK[pos];
        ledBAK[pos] %= 20;
        break;

      case 2:
        if (pos>29)      { ledBAK[pos] = CRGB::Red;    }
        else if (pos>24) { ledBAK[pos] = CRGB::Yellow; }
        else             { ledBAK[pos] = CRGB::Green;  }
        ledBAR[pos] = CRGB::White; 
        ledBAR[pos] %= 50;
        ledDOT[pos] = ledBAK[pos];
        ledBAK[pos] %= 50;
        break;

      case 3:
        if (pos>29)      { ledBAK[pos] = CRGB::Purple;    }
        else if (pos>24) { ledBAK[pos] = CRGB::Teal; }
        else             { ledBAK[pos] = CRGB::DarkGray;  }
        ledBAR[pos] = ledBAK[pos];
        ledBAR[pos] %= 120;
        ledDOT[pos] = CRGB::Red;
        ledDOT[pos] %= 100;
        ledBAK[pos] %= 25;
        break;
        
      case 4:
        ledBAK[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledBAR[pos] = ledBAK[pos];
        ledBAR[pos] %= 170;
        ledDOT[pos] = CRGB::Red;
        ledBAK[pos] %= 25;
        break;

      case 5:
        ledBAK[pos] = CRGB::DarkGray;
        ledBAR[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledBAR[pos] %= 170;
        ledDOT[pos].setHue(map(pos, 0, NUMLEDS, 180, -20)); 
        ledBAK[pos] %= 30;
        break;

      case 6:
        ledBAK[pos] = CRGB::Black;
        ledBAR[pos].setHue(map(pos, 0, NUMLEDS, 300, -20)); 
        ledBAR[pos] %= 170;
        ledDOT[pos] = CRGB::White;
        break;
    }
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
  for (int i=0; i<150; i++) {
    screensaver(SCRSAVERDEMO);
  }
}


void changedimmer(void) {
  static bool up = false;
  if (up) { dimmer = dimmer+14; }
  else    { dimmer = dimmer-14; }
  if(dimmer<=-98) { dimmer=-98; up = true; }
  if(dimmer>=56)  { dimmer=56;  up = false;}
}


void showmodenumber(uint8_t programmode) {
  #define LEDSTEPS 5
  if(!programmode) { return; }
  for (int pos=0; pos<NUMLEDS; pos++) {
    if ( programmode-1 == (int)((pos-1)/LEDSTEPS) ) { led[RGHT][pos] = CRGB::White; }
  }
  FastLED.show();
}


// -------------------------------------------------------------------------------------

void geteeprom(void) {
  EEPROM.begin(256);
  scrsvmode = EEPROM.read(EEPROMADDRSCRSV);
  displmode = EEPROM.read(EEPROMADDRVUPPM);
  colormode = EEPROM.read(EEPROMADDRCOLOR);
  dimmer    = EEPROM.read(EEPROMADDRDIMMR);
}

void savetoeeprom(void) {
  EEPROM.write(EEPROMADDRVUPPM, displmode);
  EEPROM.write(EEPROMADDRCOLOR, colormode);
  EEPROM.write(EEPROMADDRSCRSV, scrsvmode);
  EEPROM.write(EEPROMADDRDIMMR, dimmer);
  EEPROM.commit();
}

// -------------------------------------------------------------------------------------

bool screensaver(uint8_t demomode, float audiolevel) {
  unsigned long loopnow = millis();
  static unsigned long looptimer;
  static bool wait = false;
  static bool initFade = true;


  if ( demomode==SCRSAVEREND ) {
    looptimer = loopnow;
    return false;
  }

  if ( demomode==SCRSAVERDEMO ) {
    delay(15);   // make led update frequency similar to live rate
    initFade = false;
  }
  
  if ( demomode==SCRSAVERAUTO  &&  (audiolevel>1.0) ) {
    wait = false;
    initFade = true;
    return false;
  }
  else if (!wait) {
    wait = true;
    looptimer = loopnow;
    return false;
  }
  else if ( demomode==SCRSAVERDEMO  ||  (loopnow - looptimer >= SCRSAVERTIMEOUT*60*1000) ) {  

    switch (scrsvmode) {
      
      case 0:
        return false;
        
      case 1:
        scrsaverRainbow(initFade);
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


void flashleds(long color) {
  for (int i=0; i<NUMLEDS; i++) {
    led[LEFT][i] = color;
    led[LEFT][i] %= 50;
    led[RGHT][i] = color;
    led[RGHT][i] %= 50;
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
