#ifndef Display_H_
#define Display_H_

#include <Arduino.h>

#define SCRSAVERTIMEOUT 10 // minutes
#define SCRSAVERAUTO 0
#define SCRSAVERDEMO 1
#define SCRSAVEREND  2


void begindisplay(void);
void updateLeds(uint8_t vuL, uint8_t vuR, uint8_t ppmL, uint8_t ppmR);
void setcolors(void);  

void changedisplmode(void);
void changecolor(void);
void changescrsv(void);
void changedimmer(void);
void showmodenumber(uint8_t programmode);

bool screensaver(uint8_t demomode, uint8_t audiolevel=0);
void fadetoblack(void);
void scrsaverRainbow(bool initFade);
void flashleds(long color);

void savetoeeprom(void);
void geteeprom(void);


#endif
