#ifndef Display_H_
#define Display_H_

#include <Arduino.h>
#include <FastLED.h>

#define SCRSAVERTIMEOUT 10 // minutes
#define SCRSAVERAUTO 0
#define SCRSAVERDEMO 1
#define SCRSAVEREND  2
#define AVG 0
#define RMS 1
#define PPM 2


void begindisplay(void);
void updateLeds(float vuL, float vuR, float ppmL, float ppmR);
void setcolors(void);  

void changedisplmode(void);
void changecolor(void);
void changescrsv(void);
void changedimmer(void);
void showmodenumber(uint8_t programmode);

bool screensaver(uint8_t demomode, float audiolevel=0);
void fadetoblack(void);
void scrsaverRainbow(bool initFade);
void flashleds(long color);

void savetoeeprom(void);
void geteeprom(void);


#endif
