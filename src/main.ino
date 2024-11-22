#include <Arduino.h>
#include <JC_Button.h>
#include "measurement.h"
#include "display.h"
#include "overdrivelamp.h"

#define DEBUG

#define NUMCHANNELS 2
#define L 0
#define R 1

#define BUTTONGPIO 8
#define LONG_PRESS 500
#define PROGRAMMODETIMEOUT 15 // seconds
Button myBtn(BUTTONGPIO);
uint8_t programmode = 0;
int level[3][NUMCHANNELS];


void setup() {
#ifdef DEBUG
  delay(200);
  Serial.begin(115200);
  delay(200);
  Serial.println("Boot VU meter");
#endif
  pinMode(BUTTONGPIO, INPUT_PULLUP);
  myBtn.begin();
  geteeprom();
  begindisplay();
  startadc();
  beginoverdrivelamp();
}


void loop() {
  unsigned long loopnow = millis();
  static unsigned long looptimer;
  if (loopnow - looptimer >= 1000/UPDATEFREQ ) {  
    looptimer = loopnow;
    sampleAudio();
    refreshAVG();
    refreshRMS();
    refreshPPM();
    for (uint8_t ch=0;ch<NUMCHANNELS;ch++) {
      level[AVG][ch] = avgBallistics(ch);
      level[RMS][ch] = rmsBallistics(ch);
      level[PPM][ch] = ppmBallistics(ch);
    }
    //if (detectOverdrive(L) || detectOverdrive(R)) { do something }
    //else { or something else }
	
    if ( !screensaver(SCRSAVERAUTO, level[RMS][L]+level[RMS][R]) ) {
      updateLeds(level[RMS][L], level[RMS][R], level[PPM][L], level[PPM][R]);
    }
	
    checkbutton();
    showmodenumber(programmode);
#ifdef DEBUG
    debugMeasurement();
    Serial.printf("    AVG: %4d %4d", level[AVG][L], level[AVG][R] );
    Serial.printf("    RMS: %4d %4d", level[RMS][L], level[RMS][R] );
    Serial.printf("    PPM: %4d %4d", level[PPM][L], level[PPM][R] );
    Serial.printf("    %2ld ms", (millis()-loopnow) );
    Serial.printf("\t0\n");
#endif
  }
}


void checkbutton(void) {
  static bool prevWasLong = false;
  unsigned long loopnow = millis();
  #define MAXPROGRAMMODES 4
  
  static unsigned long buttontimeout;
  if (  programmode>0  &&  loopnow-buttontimeout>=PROGRAMMODETIMEOUT*1000  ) {  
    flashleds(0x333333);
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
        changedimmer();
        break;
      case 3:
        changedisplmode();
        break;
      case 4:
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
      showmodenumber(programmode);
      screensaver(SCRSAVEREND);  // just one single call to end screensaver if active
    }
    if(programmode>1) {
      stopadc();
      savetoeeprom();
      startadc();
      findDcBias(3);
    }
    if (programmode>MAXPROGRAMMODES) programmode = 0;
  }
}
