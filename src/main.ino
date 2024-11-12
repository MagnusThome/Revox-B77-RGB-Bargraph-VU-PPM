#include <Arduino.h>
#include <JC_Button.h>
#include "measurement.h"
#include "display.h"
#include "overdrivelamp.h"

#define DEBUG

#define L 0
#define R 1

int vu[NUMCHANNELS];
int ppm[NUMCHANNELS];

#define BUTTONGPIO 8
#define LONG_PRESS 500
#define PROGRAMMODETIMEOUT 15 // seconds
Button myBtn(BUTTONGPIO);
uint8_t programmode = 0;

int actualSampleRate;



// -------------------------------------------------------------------------------------

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
  beginoverdrivelamp();
  beginmeasurement();
}

  


// -------------------------------------------------------------------------------------

void loop() {
  unsigned long loopnow = millis();
  
  sampleAudio();
  actualSampleRate++;
  
  static unsigned long looptimer;
  if (loopnow - looptimer >= 1000/UPDATEFREQ ) {  
    looptimer = loopnow;
    refreshRMS();
    refreshPPM();
    vu[L] = vuBallistics(L);
    vu[R] = vuBallistics(R);
    ppm[L] = ppmBallistics(L);
    ppm[R] = ppmBallistics(R);
    if ( !screensaver(SCRSAVERAUTO, vu[L]+vu[R]) ) {
      updateLeds(vu[L], vu[R], ppm[L], ppm[R]);
    }
    if (refreshoverdrivelamp(L) || refreshoverdrivelamp(R)) {
      // do something?
    }
    showmodenumber(programmode);
    checkbutton();
#ifdef DEBUG
    debugMeasurement();
    Serial.printf("%12d %5d", vu[L], vu[R] );
    Serial.printf("%12d %5d", ppm[L], ppm[R] );
    Serial.printf("%14.3f kHz", (float)actualSampleRate*UPDATEFREQ/2000 );
    Serial.printf("\t0\n");
#endif
    actualSampleRate=0;
  }
}





// -------------------------------------------------------------------------------------

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
