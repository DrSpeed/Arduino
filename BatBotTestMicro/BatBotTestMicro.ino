#include <Wire.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>

#include <Adafruit_VS1053.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_NeoPixel.h>


//---AUDIO PLAYER-------------------------------------------
#define CLK  9 // SPI Clock, shared with SD card
#define MISO 11 // Input data, from VS1053/SD card
#define MOSI 10 // Output data, to VS1053/SD card

#define RESET 13 // VS1053 reset pin (output)
#define CS    10 // VS1053 chip select pin (output)
#define DCS   8 // VS1053 Data/command select pin (output)
#define DREQ   0 // VS1053 Data request pin (into Arduino)
#define CARDCS 6 // Card chip select pin

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(RESET, CS, DCS, DREQ, CARDCS);

bool connectedToAudioPlayer = false;
int volume =0; //keeper of the volume, if the analog input changes we update the device 
#define VOL_KNOB  A0


//------ NEOPIXEL STUFF---------------------------------------------
#define NEOPIXEL_DIGIAL_PIN 5
#define NUMBER_OF_NEOPIXELS 16
#define MIN_BRIGHT          25
#define MAX_BRIGHT          250

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_NEOPIXELS, NEOPIXEL_DIGIAL_PIN, NEO_GRB + NEO_KHZ800);


//--- SENSOR CHIP --------------------------------------------------
/* Assign a unique ID to this sensor at the same time----- */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

#define MAX_FORCE 270   // 3 Gees.  We scale all force by 10 (to integer) 270 is (9m/Sec * 3)*10
#define FORCE_SCALE 10   // We keep force as an integer, but we scale to improve precision

int historicForce = 0;
#define FORCE_DECAY  5 // arbituary value to scale brightness over time (fade out)


/*
void displaySensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}
*/

void setup(void) 
{
  // Debugging console
  Serial.begin(9600);
  
  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  
  /* Display some basic information on this sensor */
  //displaySensorDetails();

  // Neopixel-----
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  initializeSound();

}

/*
 * Connect to sound device
 */
void initializeSound(){

  if (!musicPlayer.begin()) {
    Serial.println("VS1053 not found");
    return;
  }

  Serial.println("VS1053 WAS found!");
  connectedToAudioPlayer = true;
  SD.begin(CARDCS); // initialise the SD card
  musicPlayer.setVolume(10,10);  // Set volume for AMP

  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
  Serial.println("Done initializing music player");

  manageVolume();

}


// Force to trigger sound
#define SOUND_THRESH_1 20   
#define SOUND_THRESH_2 80   

void loop(void) 
{
  /* Get a new sensor event */ 
  sensors_event_t event; 
  accel.getEvent(&event);
 
  /* Display the results (acceleration is measured in m/s^2) */
  /*
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");Serial.println("m/s^2 ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");Serial.println("m/s^2 ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
  */

  float xSqr = event.acceleration.x * event.acceleration.x;
  float ySqr = event.acceleration.y * event.acceleration.y;
  float zSqr = event.acceleration.z * event.acceleration.z;

  long fBang = sqrt( (double)(xSqr + ySqr + zSqr) );

  int bang = round(fBang * 10.0);  // increase sensitivty, scale int
  bang -= 90; // remove gravity

  bang = min(bang, MAX_FORCE); // clamp

  if (bang > 0){
    Serial.print("Bang: ");
    Serial.println(bang);
  }
  
  if (bang > historicForce){
    historicForce = bang;
  } else {
    historicForce -= FORCE_DECAY;
 }

  // Do neopixels
  setRing(historicForce);

  if (bang >  SOUND_THRESH_1){
    if (connectedToAudioPlayer){
      Serial.println("Playing sound");

      if (musicPlayer.stopped())

	if (bang >= SOUND_THRESH_2){
	  musicPlayer.startPlayingFile("10.wav");
	} else {
	  musicPlayer.startPlayingFile("9.wav");
	}
    } else {
      Serial.println("No sound player");
    }
  }
  manageVolume();
  delay(10); // maybe even faster?
}



void setRing(int force){

  // Brightness maps (linear) to force
  strip.setBrightness(map(force, 0, MAX_FORCE, MIN_BRIGHT, MAX_BRIGHT)); 

  uint16_t ledColor;
  ledColor = map(force, 0, MAX_FORCE, 0, 255);  // map force to 0...255

  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(ledColor)); 
  }
  strip.show();

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


void manageVolume(){
  if (!connectedToAudioPlayer)
    return;

  // The volume goes 0 (loud) .. 100 (quiet)
  // To avoid jitter, we get a value from 1..25 and multiply by 4
  int curVol = map(analogRead(VOL_KNOB), 0, 1023, 25, 0) * 4;
  if (curVol != volume){
    volume = curVol;
    musicPlayer.setVolume(volume, volume);
    /*
    Serial.print("Volume ");
    Serial.println(volume);
    */
  }

}
