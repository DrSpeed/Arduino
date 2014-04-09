#include <Adafruit_NeoPixel.h>

#define PIN 10

const int xpin = A1;                  // x-axis of the accelerometer
const int ypin = A0;                  // y-axis
const int zpin = A3;    

#define A_CENTER  512

#define A_G 102
#define MAX_GS  2

#define INTENS  44

#define N_PIX 52
#define MIN_PCT 5

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIX, PIN, NEO_GRB + NEO_KHZ800);

uint32_t acc_color;
uint32_t dcc_color;
uint32_t acc_decay_color;
uint32_t dcc_decay_color;
uint32_t acc_bg_color;
uint32_t dcc_bg_color;

uint32_t tip_color;


void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // Setup colors
  acc_color = strip.Color(0, INTENS, 0);
  dcc_color = strip.Color(INTENS, 0, 0);

  acc_decay_color = strip.Color(0, INTENS/2, 0);
  dcc_decay_color = strip.Color(INTENS/2, 0, 0);

  acc_bg_color = strip.Color(0, 0, 0); // No bg color for accel
  dcc_bg_color = strip.Color(INTENS/4, 0, 0);

  tip_color = strip.Color(INTENS, INTENS, 0);

}

long lastTime = 0;
long lastLedTime = 0;
#define POLLING_INTERVAL 100
#define  MAX_VAL (A_G * MAX_GS)
#define DECAY_AMNT 10

long maxGs = 0;
long dimTime = 0;
#define DIM_DUR 750  // mSec

#define STATE_ACCL 0x01
#define STATE_DCCL 0x02

int state = STATE_ACCL;


void loop() {

  long nowTime = millis();

  if( nowTime < (lastTime + POLLING_INTERVAL)  ){
    return;
  }

  lastTime = nowTime;


  long val = analogRead(ypin) - A_CENTER;


  Serial.print(millis());
  Serial.print("\t");
  Serial.println(val);

  uint32_t dotColor;
  uint32_t bgColor;

  if (val < 0){
    if (state ==  STATE_ACCL)
      maxGs = 0;
    state = STATE_DCCL;
    dotColor = dcc_color;
    bgColor = dcc_bg_color;
  } else { // val >= 0 
    if (state ==  STATE_DCCL)
      maxGs = 0;
    state = STATE_ACCL;
    dotColor = acc_color;
    bgColor = acc_bg_color;
  }

  val = abs(val);

  if (val > maxGs){
    maxGs = val;
    dimTime = nowTime + DIM_DUR;
  } else {
    maxGs -= DECAY_AMNT;
    if (nowTime > dimTime)
      dotColor = state == STATE_DCCL? dcc_decay_color: acc_decay_color;
  }

  // Calculate percentage
  long pct = min(maxGs, MAX_VAL);
  pct = max(0, (100.0 * (double)pct / MAX_VAL)) ;  // Avoid math neg. numbers

  //Serial.print("Pct: ");
  //Serial.println(pct);
  
  // Display the current state, could be optimized for situations when
  // nothing is going on.
  fillStrip(pct, dotColor, bgColor);

}  // End loop


void clearStrip(){
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}


/*
 * fillString
 *   center split version
 *
 */
void fillStrip(int pct, uint32_t inputColor, uint32_t bgColor){

  int nPix = strip.numPixels();
  int midl = nPix/2;
  int maxPix = map(pct, 0, 100, 0, midl);

  bgColor = (pct > 10)? bgColor: 0x0;  // make blank if nothing going on

  for(uint16_t i=0; i<midl; i++) {
    uint32_t dotColor = bgColor;
    if (i < maxPix){
      if ( i > 2 && ( (i == (maxPix - 1)) || i == (maxPix -2)))
	dotColor = tip_color;
      else
	dotColor = inputColor;
    }
    strip.setPixelColor(midl + i, dotColor);
    strip.setPixelColor(midl - i, dotColor);
  }
  strip.show();
}



/* works
void fillStrip(int pct, uint32_t inputColor){

  int nPix = strip.numPixels();
  int maxPix = map(pct, 0, 100, 0, nPix);

  for(uint16_t i=0; i<strip.numPixels(); i++) {
    uint32_t dotColor = 0x0;
    if (i < maxPix){
      if ( (i >= (maxPix-2)) && (i>3) ){
	dotColor = tip_color;
      } else {
	dotColor = inputColor;
      }
    }
    strip.setPixelColor(i, dotColor);
  }
  strip.show();

}
*/


void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}



