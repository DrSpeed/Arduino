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

int nDirs;
char **allDirs;

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
  Serial.print(F("initialize SD card... "));
  result = sd.begin(SD_CS, SPI_HALF_SPEED); // 1 for success
  if (result != 1) // Problem initializing the SD card
  {
    Serial.print(F("error, halting (missing card?)"));
    errorBlink(1); // Halt forever, blink LED if present.
  }
  else
    Serial.println(F("success!"));

  //---Start up the MP3 library---------------------------
  Serial.print(F("initialize MP3 chip... "));
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

  // Read the SD Card for audio files
  Serial.print(F("Collecting audio files... "));
  if (collectSongs() != true){
      Serial.print(F("Error collecting audio files (no files)."));
      Serial.print(F(", halting."));
      errorBlink(result); // Halt forever, blink red LED if present.
  }

  // Set the VS1053 volume. 0 is loudest, 255 is lowest (off):
  MP3player.setVolume(10,10);
  
  // Turn on the amplifier chip:
  digitalWrite(EN_GPIO1,HIGH);
  delay(2);
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
      Serial.print(F("Starting track"));
      Serial.println(tracknum);

      char * trackname = *(allMusicFiles + tracknum);

      byte result = MP3player.playMP3(trackname);

      Serial.println("setting up next track");
      // Next track
      if (tracknum >= nMusicFiles){
	tracknum = 0;
      } else {
	tracknum ++;
      }

      if(result != 0)
	{
	  Serial.print(F("error "));
	  Serial.print(result);
	  Serial.print(F(" when trying to play track "));
	  Serial.println(trackname);
	}
      else
	{
	  Serial.print(F("playing "));
	  Serial.println(trackname);
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

int countDirectories(SdFile file){
  sd.chdir("/",true);
  int count = 0;
  while (file.openNext(sd.vwd(),O_READ))
    {
      if (file.isDir()) {
	count++;
      }
      file.close();
    }
  return count;
}


void populateDirectories(SdFile file, char** allDirNames, int nDirs){

  char tempfilename[13];
  sd.chdir("/",true);
  int count = 0;

  while (file.openNext(sd.vwd(),O_READ))
    {
      file.getFilename(tempfilename);    // get filename
      if (file.isDir())
	{
	  char *allocd = (char *) malloc(  (unsigned int)( (sizeof(char) * strlen(tempfilename)) + 1));
	  
	  strcpy(allocd, tempfilename);

	  Serial.print(F("Collecting directory name file:"));
	  Serial.println(allocd);
	  *allDirNames = allocd;
	  allDirNames ++;
	}
      file.close();
    }
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


int countMusicFiles(char * directory,  SdFile file){

  char tempfilename[13];
  sd.chdir(directory, true);

  int count = 0;
  while (file.openNext(sd.vwd(),O_READ))
    {
      file.getFilename(tempfilename);    // get filename

     if (!file.isDir()) {
       if (isMusicFile(tempfilename))
	 {
	   serialPrint("Found audio file: %s\n", tempfilename);
	   count++;
	 }
       else
	 {
	   serialPrint("Found NON audio file: %s\n", tempfilename);
	 }
     }
     file.close();
    }
  sd.chdir("/", true);
  return count;
}


/*
 * Add the music files from the supplied directory, return the pointer to the next 
 * location.
 */
char ** populateAllMusicFiles(char * dir, SdFile file, char** allMusicFiles){
  char tempfilename[13];

  Serial.print(F("Collecting files..."));
  Serial.println(dir);

  sd.chdir(dir, true);
  int count = 0;
  while (file.openNext(sd.vwd(),O_READ))
    {
      file.getFilename(tempfilename);    // get filename
      if (!file.isDir() && isMusicFile(tempfilename))
	{
	  String d = dir;
	  String str  =  String(d + "/" + tempfilename);  // Concatenate to get full filename
	  //	  char *allocd = (char *) malloc(  (unsigned int)( (sizeof(char) * strlen(tempfilename)) + 1));
	  char *allocd = (char *) malloc(  (unsigned int)( (sizeof(char) * str.length()) + 1));
	  
	  str.toCharArray(allocd, str.length() + 1); // Put string into space allocated 
	  *allMusicFiles = allocd;   // Add pointer to string memory
	  allMusicFiles ++;  //Increment array pointer to next spot

	  serialPrint("Collected sound file: %s\n", allocd);
	  count++;
	}
      file.close();
    }

  serialPrint("Collected %d Files.", count);  

  // go back to root (IMPORTANT)
  sd.chdir("/", true);

  return allMusicFiles;
}


/*
 * Scan all the first level folders for audio files and add the files to 
 * a set of char pointers.
 * Songs have the directory name in them without leading slash
 *  e.g.:  music/rock_on.mp3
 * Returns TRUE if we found some files.
 */
bool collectSongs() {
  SdFile file;
  
  nDirs = countDirectories(file);
  serialPrint("Found %d directories\n", nDirs);

  allDirs = (char **)malloc(sizeof(char*) * nDirs);
  populateDirectories(file, allDirs, nDirs);

  // Now that we know all the directories, get the number of audio files
  for (int i=0; i<nDirs; i++){
    char *dirname = *(allDirs + i);
    Serial.print("Directory:");
    Serial.println(dirname);
    nMusicFiles += countMusicFiles(dirname,  file);
  }
  serialPrint("Found %d music files\n", nMusicFiles);

  // allocate list of song pointers, song strings themselves
  // are allocated elsewhere
  allMusicFiles = (char **)malloc(sizeof(char*) * nMusicFiles);
  Serial.println("Allocated memory for audio filenames.");  

  // Put all the audio files into a list of char pointers
  char ** curSong = allMusicFiles;
  for (int i=0; i<nDirs; i++){
    char *fn = *(allDirs + i);
    String dirname = fn; 
    curSong = populateAllMusicFiles(fn, file, curSong);
  }

  return (nMusicFiles != 0);
}



