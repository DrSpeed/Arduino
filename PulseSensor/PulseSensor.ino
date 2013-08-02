#include <stdarg.h>
#include <SPI.h>            // To talk to the SD card and MP3 chip
#include <SdFat.h>          // SD card file system
#include <SFEMP3Shield.h>   // MP3 decoder chip

#define PAUSE_VOL  40  // If the volume gets this low, pause the player


// And a few outputs we'll be using:
const int ROT_LEDR = 10; // Red LED in rotary encoder (optional)
const int EN_GPIO1 = A2; // Amp enable + MIDI/MP3 mode select
const int SD_CS = 9;     // Chip Select for SD card


// MP3 Player and SD CARD
SFEMP3Shield MP3player;
SdFat sd;

int nAudioFileTypes = 3;
char *  audioFileTypes[] = {".mp3", ".m4a", ".mid"};

// Where we are at in the filesystem
int curBpmFolder = 60; // what we're playing now
int curSong; // index in folder of what what we're playing now
char *songName = 0x0;
int nSongs; // count in folder we're listening to

/*
 * Note, all debugging is commented out, seems to break when not connected to serial
 */

void serialPrint(char *fmt, ... ){
        char tmp[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(tmp, 128, fmt, args);
        va_end (args);
	//        Serial.print(tmp);
}



/*
>> Pulse Sensor Amped 1.1 <<
This code is for Pulse Sensor Amped by Joel Murphy and Yury Gitman
    www.pulsesensor.com 
    >>> Pulse Sensor purple wire goes to Analog Pin 0 <<<
Pulse Sensor sample aquisition and processing happens in the background via Timer 2 interrupt. 2mS sample rate.
PWM on pins 3 and 11 will not work when using this code, because we are using Timer 2!
The following variables are automatically updated:
Signal :    int that holds the analog signal data straight from the sensor. updated every 2mS.
IBI  :      int that holds the time interval between beats. 2mS resolution.
BPM  :      int that holds the heart rate value, derived every beat, from averaging previous 10 IBI values.
QS  :       boolean that is made true whenever Pulse is found and BPM is updated. User must reset.
Pulse :     boolean that is true when a heartbeat is sensed then false in time with pin13 LED going out.

This code is designed with output serial data to Processing sketch "PulseSensorAmped_Processing-xx"
The Processing sketch is a simple data visualizer. 
All the work to find the heartbeat and determine the heartrate happens in the code below.
Pin 13 LED will blink with heartbeat.
If you want to use pin 13 for something else, adjust the interrupt handler
It will also fade an LED on pin fadePin with every beat. Put an LED and series resistor from fadePin to GND.
Check here for detailed code walkthrough:
http://pulsesensor.myshopify.com/pages/pulse-sensor-amped-arduino-v1dot1

Code Version 02 by Joel Murphy & Yury Gitman  Fall 2012
This update changes the HRV variable name to IBI, which stands for Inter-Beat Interval, for clarity.
Switched the interrupt to Timer2.  500Hz sample rate, 2mS resolution IBI value.
Fade LED pin moved to pin 5 (use of Timer2 disables PWM on pins 3 & 11).
Tidied up inefficiencies since the last version. 
*/


//  VARIABLES FOR PULSE MEASUREMENT
int pulsePin = A5;                 // Pulse Sensor purple wire connected to analog pin 0
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int lastBpm = 66;                 // Store the last heart rate

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.



/*
 * Called once at startup
 */
void setup(){
  //  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  //  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(9600); //115200);             // we agree to talk fast!

  // SJW _TEST_
  //  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 


   // UN-COMMENT THE NEXT LINE IF YOU ARE POWERING The Pulse Sensor AT LOW VOLTAGE, 
   // AND APPLY THAT VOLTAGE TO THE A-REF PIN
   //analogReference(EXTERNAL);   


  byte result;

  pinMode(ROT_LEDR,OUTPUT);
  digitalWrite(ROT_LEDR,HIGH);  // HIGH = off

  // Trigger
  pinMode(A4, INPUT);
  digitalWrite(A4,HIGH);

  // The board uses a single I/O pin to select the
  // mode the MP3 chip will start up in (MP3 or MIDI),
  // and to enable/disable the amplifier chip:
  pinMode(EN_GPIO1,OUTPUT);
  digitalWrite(EN_GPIO1,LOW);  // MP3 mode / amp off

  Serial.begin(9600);
  //  Serial.println(F("Lilypad MP3 Player sketch, vol(A0) down-->Pause, (A4)button-->advance"));
  
  //--- Initialize the SD card; SS = pin 9, half speed at first-------
  //  Serial.print(F("initialize SD card... "));
  result = sd.begin(SD_CS, SPI_HALF_SPEED); // 1 for success
  if (result != 1) // Problem initializing the SD card
  {
    //    Serial.print(F("error, halting (missing card?)"));
    errorBlink(1); // Halt forever, blink LED if present.
  }
  else
    //    Serial.println(F("success!"));

  //---Start up the MP3 library---------------------------
    //  Serial.print(F("initialize MP3 chip... "));
  result = MP3player.begin(); // 0 or 6 for success
  // Check the result, see the library readme for error codes.
  if ((result != 0) && (result != 6)) // Problem starting up
  {
    Serial.print(F("error code "));
    Serial.print(result);
    Serial.print(F(", halting."));
    errorBlink(result); // Halt forever, blink red LED if present.
  }
  else
    Serial.println(F("success!"));


  // Set the VS1053 volume. 0 is loudest, 255 is lowest (off):
  MP3player.setVolume(1,1); // start quiet for now

  countSongs();
  //  Serial.print("Dir:");
  //  Serial.println(curBpmFolder);
  getSong();
  
  // Turn on the amplifier chip:
  digitalWrite(EN_GPIO1,HIGH);
  delay(2);


  //  Serial.print("song: ");
  //  Serial.println(songName);

  //  Serial.print("folder: ");
  //  Serial.println(curBpmFolder);


  String trackName = String(curBpmFolder);
  trackName = trackName + String("/");
  trackName = trackName + String(songName);
  char buf[64];
  trackName.toCharArray(buf, trackName.length() + 1);

  //  Serial.print("playing: ");
  //  Serial.println(buf);


  // Set this bfor we start playing
  int anal0 = analogRead(A0);
  int knobVolume = map(anal0, 0, 1023, 255, 0);  // 0:loud  255: quiet/off
  MP3player.setVolume(knobVolume, knobVolume);
  // set up nSongs variable every time we change directories
  countSongs();

  // start playing
  result = MP3player.playMP3(buf);
  // sjw: check result here?
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
  freeRam();


}


int currentVolume = -1;
int tracknum = 0;

int getBpmFolder(int bpm){
  if (bpm < 80){
    return 60;
  } else if (bpm >= 140){
    return 140;
  } else {
   return (bpm/20) * 20;
  }
}


void loop(){
  //  sendDataToProcessing('S', Signal);     // send Processing the raw Pulse Sensor data
  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
        fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
        sendDataToProcessing('B',BPM);   // send heart rate with a 'B' prefix
	//        sendDataToProcessing('Q',IBI);   // send time between beats with a 'Q' prefix
        QS = false;                      // reset the Quantified Self flag for next time    
	lastBpm = BPM;
     }
  
  delay(500);                             //  take a break 20 definitely did not work, 100 pretty good

  // Check knob and button-------------------------
  int anal0 = analogRead(A0);
  int knobVolume = map(anal0, 0, 1023, 255, 0);  // 0:loud  255: quiet/off
  bool advance = false;      // advance?

  if (knobVolume != currentVolume){
    MP3player.setVolume(knobVolume, knobVolume);
    
    if ((knobVolume <= PAUSE_VOL) && (currentVolume > PAUSE_VOL) &&  MP3player.isPlaying() )
      MP3player.pauseMusic();

    if ((knobVolume > PAUSE_VOL) && (currentVolume <= PAUSE_VOL) &&  !MP3player.isPlaying() )
      MP3player.resumeMusic();
      
    currentVolume = knobVolume;
  } else { // check advace button
    int waitAround = 0;
    while(waitAround < 50){
      if (digitalRead(A4) == LOW){
	advance = true;
	MP3player.stopTrack();
	//	Serial.println(F("--ADVANCE--"));      
	break;
      }
      delay(1);
      waitAround++;
    } 
  }
  //---------------------------------------

  noInterrupts();
  bool playing = MP3player.isPlaying();
  interrupts();  

  if (!playing || advance)
    {
      int latestFolder = getBpmFolder(lastBpm);

      //      Serial.println("setting up next track");

      // Bpm in same range--------------------
      if (curBpmFolder == latestFolder){
	if (curSong < (nSongs-1)){
	  curSong++;
	} else {
	  curSong = 0; // start over in same folder
	}
      } else {
	// different BPM == different folder
	curBpmFolder = latestFolder;
	curSong = 0;
      }

      //      serialPrint("curSong: %d", curSong);
      getSong(); // always need a new song

      // Concatenate to get full filename
      String trackName = String(curBpmFolder);
      trackName = trackName + String("/");
      trackName = trackName + String(songName);
      char buf[64];
      trackName.toCharArray(buf, trackName.length() + 1);

      noInterrupts();
      byte result = MP3player.playMP3(buf);
      interrupts();
      if(result != 0)
	{
	  //	  Serial.print(F("error "));
	  //	  Serial.print(result);
	  //	  Serial.print(F(" when trying to play track "));
	  //	  Serial.println(buf);
	}
      else
	{
	  //	  Serial.print(F("playing "));
	  //	  Serial.println(buf);
	}
      //      MP3player.stopTrack();
    }

}


void sendDataToProcessing(char symbol, int data ){
  //    Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
  //    Serial.println(data);                // the data to send culminating in a carriage return
  }



void errorBlink(int blinks)
{
  // The following function will blink the red LED in the rotary
  // encoder (optional) a given number of times and repeat forever.
  // This is so you can see any startup error codes without having
  // to use the serial monitor window.

  int x;

  while(true) // Loop forever
  {
    for (x=0; x < blinks; x++) // Blink the given number of times
    {
      digitalWrite(ROT_LEDR,LOW); // Turn LED ON
      delay(250);
      digitalWrite(ROT_LEDR,HIGH); // Turn LED OFF
      delay(250);
    }
    delay(1500); // Longer pause between blink-groups
  }
}

void countSongs(){
  SdFile file;

  String dirName =  String(curBpmFolder);  // Concatenate to get full filename
  char buf[64];
  dirName.toCharArray(buf, dirName.length() + 1);


  //  Serial.print("cur bpm folder");
  //  Serial.println(curBpmFolder);
  //  serialPrint("Checking for songs in: %s\n", buf);

  sd.chdir((const char*)buf, true);
  int count = 0;
  while (file.openNext(sd.vwd(),O_READ))
    {
      if (!file.isDir()){
	char tempfilename[13];
	file.getFilename(tempfilename);    // get filename
	if (isMusicFile(tempfilename))
	  {
	    //	    serialPrint("Found audio file: %s\n", tempfilename);
	    count++;
	  }
      }
      file.close();
    }
  sd.chdir("/", true);
  
  //  serialPrint("nSongs: %d\n", nSongs);
  nSongs = count;
}


/*
 * Update the global with the name of the song at curSong index
 */
void getSong()
{
  SdFile file;

  // This is this super clumsey arduino C workaround to get a concat to work.
  String dirName =  String(curBpmFolder);  // Concatenate to get full filename
  unsigned char buf[64];
  dirName.getBytes(buf, 64); // copy to buffer  buffer, size

  sd.chdir((const char*)buf, true);
  int count = 0;

  //  serialPrint("gettings song: %d\n", curSong);
  while (file.openNext(sd.vwd(),O_READ))
    {
      if (!file.isDir()){
	char tempfilename[13];
	file.getFilename(tempfilename);    // get filename
	if (isMusicFile(tempfilename)){
	  //	  serialPrint("checking for #%d song: %s\n", count, tempfilename);
	  if (count == curSong){  // This is the dir we want
	    if (songName != 0x0){
	      free(songName);  // clean up old one, don't forget this or KABOOM!
	    }
	    songName = (char *) malloc(  (unsigned int)( (sizeof(char) * strlen(tempfilename)) + 1));
	    strcpy(songName, tempfilename);
	    file.close();
	    sd.chdir("/", true);
	    return;
	  }
	  count++;

	} // is song
      } // is dir
      file.close();
    }
  sd.chdir("/", true);
}


/*
 * Attempt to derive (from the filename) if a file
 * is an audio file. There may be junk on the sdcard
 */
bool isMusicFile(char *filename)
{  
  if (filename[0] == '_')  //Thanks windows for leaving crap around
    return false;

  String fn = filename;
  fn.toLowerCase(); // normalize

  for (int i=0; i<nAudioFileTypes; i++){
    char * extension = audioFileTypes[i];
    if (fn.indexOf(extension) > 0){
      return true;
    }
  }
  return false;
}


int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  int fr = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
  //  Serial.print("Free ram: ");
  //  Serial.println(fr);

}
