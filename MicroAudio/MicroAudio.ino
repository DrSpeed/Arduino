#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>


#define CLK  9 // SPI Clock, shared with SD card
#define MISO 11 // Input data, from VS1053/SD card
#define MOSI 10 // Output data, to VS1053/SD card

#define RESET 13 // VS1053 reset pin (output)
#define CS    10 // VS1053 chip select pin (output)
#define DCS   8 // VS1053 Data/command select pin (output)
#define DREQ   0 // VS1053 Data request pin (into Arduino)
#define CARDCS 6 // Card chip select pin


Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(RESET, CS, DCS, DREQ, CARDCS);

bool foundIt = false;

void setup() {
  Serial.begin(9600);

  if (!musicPlayer.begin()) {
    Serial.println("VS1053 not found");
    
  } else {
    Serial.println("VS1053 WAS found!");
    foundIt = true;
    SD.begin(CARDCS); // initialise the SD card
    musicPlayer.setVolume(10,10);

    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
    delay(5000);
    Serial.println("Done initializing");
  }
}


int count = 1;
int maxFiles = 10;

String getFile(){
  
  count = (++count)%maxFiles;  // 0...(max-1)

  String s = String(count+1);
  //  s += ".mp3";
  s += ".wav";
  
  return "10.wav";


}


void loop(){

  if (foundIt){
    Serial.println("Trying to play");

    if (musicPlayer.stopped())
      {
	String str = getFile();
	int str_len = str.length() + 1; 

	// Prepare the character array (the buffer) 
	char char_array[str_len];

	// Copy it over 
	str.toCharArray(char_array, str_len);


	Serial.print("Starting ");
	Serial.println(str);
	musicPlayer.startPlayingFile(char_array);

	//  musicPlayer.sineTest(0x44, 500); // Make a tone to indicate VS1053 is working


      } else {
      Serial.println("Not stopped");
    }
  } else {
    Serial.println("Did not find it");

  }

  delay(1000);  
}
