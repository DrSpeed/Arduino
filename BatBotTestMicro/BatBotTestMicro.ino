/*
 * This is the code for the Arduino Micro w/accel., neopixel and VS1053
 * The MISO/MOSI/CLK pins are different on the micro from mego or uno.
 *
 * This code assumes (hardcoded) a couple wav files on SD card 9.wav, 10.wav.
 *  tHE VS1053 likes MP3s better than WAVs. Use Audacity, load all and 
 *  do a export multiple
 */

#include <Wire.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>

#include <Adafruit_VS1053.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_NeoPixel.h>


#define LED_PIN 13

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

// Force to trigger sound
#define SOUND_THRESH_1 20   // 2 m/Sec
#define SOUND_THRESH_2 80   // 8 m/Sec -just under a G

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
#define GRAVITY_SCALED  98  // Gravity is 9.8 meters/second we keep it scaled by 10


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

//  -- SETUP --
void setup(void) 
{
  // Debugging console
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);   // LED PIN

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


/*
 * This code chooses what file to play when robot is stationary.
 * There is a folder on the SD card with files named 1.mp3 ... 50.mp3
 * we randomly choose one.
 */ 
#define N_SAMPLES     50
#define RANDOM_FILES_FOLDER "/wavs/"
#define MP3_SUFFIX    ".mp3"

int lastSample = -1;  // Keep track of the last one so we never play the same one twice

String getFile(){
  
  int x = random(1, N_SAMPLES);
  while (x == lastSample){  // no dups
    x = random(1, N_SAMPLES);  // try again
  }
  lastSample = x; // update variable

  String s = String(RANDOM_FILES_FOLDER);
  s += String(x);
  s += MP3_SUFFIX;
  
  return s;
}


long nextRandomTime = 4000;
boolean playingRandomSample = false;

//  Bracket time for random sample playback in millisec
#define RAND_MIN 10000
#define RAND_MAX 15000

// -- LOOP --
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

  // Accelerometer for in meters/sec
  float xSqr = event.acceleration.x * event.acceleration.x;
  float ySqr = event.acceleration.y * event.acceleration.y;
  float zSqr = event.acceleration.z * event.acceleration.z;  

  // Sum and normalize the vectors
  long fBang = sqrt( (double)(xSqr + ySqr + zSqr) );

  int bang = round(fBang * 10.0);  // increase sensitivty, scale int
  
  // Cancel out gravity. There will ALWAYS be gravity for on the chip
  bang -= GRAVITY_SCALED;

  bang = min(bang, MAX_FORCE); // clamp

  
  if (bang > historicForce){
    historicForce = bang;
  } else {
    historicForce -= FORCE_DECAY;
  }

  // Do neopixels
  setRing(historicForce);

  if (connectedToAudioPlayer){

    if (bang >  SOUND_THRESH_1){  // play a bang sound

      if (playingRandomSample){ // a bang stops random sample
	musicPlayer.stopPlaying();
	playingRandomSample = false;
      }

      if (musicPlayer.stopped()){  // If we're already playing bang sound, keep it going

	digitalWrite(LED_PIN, HIGH);   // turn the LED on
	if (bang >= SOUND_THRESH_2){
	  musicPlayer.startPlayingFile("10.wav");
	} else {
	  musicPlayer.startPlayingFile("9.wav");
	}
	digitalWrite(LED_PIN, LOW);   // turn the LED on
      }
    } else {  // check to see if we should play some random sound
      manageAmbientSounds();
    }
  }else {
    Serial.println("No sound player");
  }
  manageVolume();
  delay(10); // maybe even faster?
}

void manageAmbientSounds(){
  // we're not already playing a ran
  if (musicPlayer.stopped()  &&  (nextRandomTime <= millis())){
    //Serial.println("Playing random sound");
    digitalWrite(LED_PIN, HIGH);   // turn the LED on
    playRandom();
    playingRandomSample = true;  // what we're playing is not important
    nextRandomTime = millis() + random(RAND_MIN, RAND_MAX);
    digitalWrite(LED_PIN, LOW);    // turn the LED off
  }
}


void playRandom(){
  // GET A RANDOM FILE
  String str = getFile();
  int str_len = str.length() + 1; 

  // Prepare the character array (the buffer) 
  char char_array[str_len];

  // Copy it over 
  str.toCharArray(char_array, str_len);

  musicPlayer.startPlayingFile(char_array);
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



