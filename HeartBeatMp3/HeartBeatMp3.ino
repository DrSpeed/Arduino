#include <stdarg.h>
#include <SPI.h>            // To talk to the SD card and MP3 chip
#include <SdFat.h>          // SD card file system
#include <SFEMP3Shield.h>   // MP3 decoder chip

#define PAUSE_VOL  40  // If the volume gets this low, pause the player


// And a few outputs we'll be using:

const int ROT_LEDR = 10; // Red LED in rotary encoder (optional)
const int EN_GPIO1 = A2; // Amp enable + MIDI/MP3 mode select
const int SD_CS = 9;     // Chip Select for SD card

// Create library objects:

SFEMP3Shield MP3player;
SdFat sd;

int nAudioFileTypes = 3;
char *  audioFileTypes[] = {".mp3", ".m4a", ".mid"};

int nMusicFiles;
char **allMusicFiles;

// Where we are at in the filesystem
int nDirs;  // in filesystem
int curDir;
char *dirName = 0x0;
int curSong; // in folder
char *songName = 0x0;
int nSongs; // in folder

void serialPrint(char *fmt, ... ){
        char tmp[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(tmp, 128, fmt, args);
        va_end (args);
        Serial.print(tmp);
}

void setup()
{
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
  Serial.println(F("Lilypad MP3 Player sketch, vol(A0) down-->Pause, (A4)button-->advance"));
  
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
  MP3player.setVolume(10,10);

  countDirectories();
  curDir = 0; // start at begining
  getDirectoryName();
  countSongs();
  Serial.print("Dir:");
  Serial.println(dirName);
  getSong();
  
  // Turn on the amplifier chip:
  digitalWrite(EN_GPIO1,HIGH);
  delay(2);

  String d = dirName;
  String trackName =  String(d + "/" + songName);  // Concatenate to get full filename
  Serial.print("playing: ");
  Serial.println(trackName);

  char buf[64];
  trackName.toCharArray(buf, trackName.length() + 1);


  result = MP3player.playMP3(buf);
  // sjw: check result here?

}

int currentVolume = -1;
int tracknum = 0;
void loop()
{
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
	Serial.println(F("--ADVANCE--"));      
	break;
      }
      delay(1);
      waitAround++;
    } 
  }

  if (!MP3player.isPlaying() || advance)
    {
      Serial.println("setting up next track");
      if (curSong < (nSongs-1)){
	curSong++;
      } else {  // different folder
	if (curDir < (nDirs - 1)){
	  curDir++;
	} else {
	  curDir = 0;
	}
	curSong = 0;
	getDirectoryName();
      }

      serialPrint("curSong: %d", curSong);
      getSong(); // always need a new song

      char buf[64];
      String d = dirName;

      // Concatenate to get full filename
      String trackName =  String(d + "/" + songName);  
      trackName.toCharArray(buf, trackName.length() + 1);

      byte result = MP3player.playMP3(buf);
      if(result != 0)
	{
	  Serial.print(F("error "));
	  Serial.print(result);
	  Serial.print(F(" when trying to play track "));
	  Serial.println(buf);
	}
      else
	{
	  Serial.print(F("playing "));
	  Serial.println(buf);
	}
      //      MP3player.stopTrack();
    }
  
  delay(100);

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

/* set global */
void countDirectories(){
  SdFile file;
  sd.chdir("/",true);
  int count = 0;
  while (file.openNext(sd.vwd(),O_READ))
    {
      if (file.isDir()) {
	count++;
      }
      file.close();
    }
  nDirs = count;
  serialPrint("#dirs: %d\n", nDirs); 
}

void getDirectoryName()
{
  SdFile file;
  sd.chdir("/",true);
  int count = 0;
  while (file.openNext(sd.vwd(),O_READ))
    {
      if (file.isDir()) {
	if (count == curDir){  // This is the dir we want
	  if (dirName != 0x0){
	    free(dirName);  // clean up old one
	  }
	  char tempfilename[13];
	  file.getFilename(tempfilename);
	  // set global
	  dirName = (char *) malloc(  (unsigned int)( (sizeof(char) * strlen(tempfilename)) + 1));
	  strcpy(dirName, tempfilename);
	  file.close();
	  return;
	}
	count++;
      }
      file.close();
    }
}

void countSongs(){
  SdFile file;

  sd.chdir(dirName, true);

  int count = 0;
  while (file.openNext(sd.vwd(),O_READ))
    {
      if (!file.isDir()){
	char tempfilename[13];
	file.getFilename(tempfilename);    // get filename
	if (isMusicFile(tempfilename))
	  {
	    serialPrint("Found audio file: %s\n", tempfilename);
	    count++;
	  }
      }
      file.close();
    }
  sd.chdir("/", true);
  
  serialPrint("nSongs: %d\n", nSongs);
  nSongs = count;
}


/*
 * Update the global with the name of the song at curSong index
 */
void getSong()
{
  SdFile file;
  sd.chdir(dirName, true);
  int count = 0;

  serialPrint("gettings song: %d\n", curSong);
  while (file.openNext(sd.vwd(),O_READ))
    {
      if (!file.isDir()){
	char tempfilename[13];
	file.getFilename(tempfilename);    // get filename
	if (isMusicFile(tempfilename)){
	  serialPrint("checking for #%d song: %s\n", count, tempfilename);
	  if (count == curSong){  // This is the dir we want
	    if (songName != 0x0){
	      free(songName);  // clean up old one
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
  Serial.print("Free ram: ");
  Serial.println(fr);

}
