#include "overdrivelamp.h"


#define OVRINPUTGPIO_L  2
#define OVRINPUTGPIO_R  3

#define L 0
#define R 1

uint8_t channelgpio[] = {OVRINPUTGPIO_L, OVRINPUTGPIO_R};


// -------------------------------------------------------------------------------------

void beginoverdrivelamp(void) {
  pinMode(OVRINPUTGPIO_L, INPUT);
  pinMode(OVRINPUTGPIO_R, INPUT);
}


bool refreshoverdrivelamp(uint8_t ch) {
  return digitalRead(channelgpio[ch]);
}
