/*
  Temperature web interface

 This example shows how to serve data from an analog input
 via the Arduino Yún's built-in webserver using the Bridge library.

 The circuit:
 * TMP36 temperature sensor on analog pin A1
 * SD card attached to SD card slot of the Arduino Yún

 Prepare your SD card with an empty folder in the SD root
 named "arduino" and a subfolder of that named "www".
 This will ensure that the Yún will create a link
 to the SD to the "/mnt/sd" path.

 In this sketch folder is a basic webpage and a copy of zepto.js, a
 minimized version of jQuery.  When you upload your sketch, these files
 will be placed in the /arduino/www/TemperatureWebPanel folder on your SD card.

 You can then go to http://arduino.local/sd/TemperatureWebPanel
 to see the output of this sketch.

 You can remove the SD card while the Linux and the
 sketch are running but be careful not to remove it while
 the system is writing to it.

 created  6 July 2013
 by Tom Igoe

 This example code is in the public domain.

 http://arduino.cc/en/Tutorial/TemperatureWebPanel

 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Adafruit_NeoPixel.h>
 
#define PIN 5
uint32_t stripColor  = 0xff0000; // Start red 
int ledID = 0;
int ledBright = 20;
int randomFactor = 10;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN);

// Listen on default port 5555, the webserver on the Yún
// will forward there all the HTTP requests for us.
YunServer server;
String startString;
long hits = 0;

void setup() {
  Serial.begin(9600);

  // Bridge startup
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);

  // using A0 and A2 as vcc and gnd for the TMP36 sensor:
  pinMode(A0, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A0, HIGH);
  digitalWrite(A2, LOW);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  strip.begin();
  strip.setBrightness(60); // 1/3 brightness

  // get the time that this sketch started:
  Process startTime;
  startTime.runShellCommand("date");
  while (startTime.available()) {
    char c = startTime.read();
    startString += c;
  }
  
  strip.setPixelColor(ledID, stripColor);
  strip.show();
}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  // There is a new client?
  if (client) {
    // read the command
    String command = client.readString();
    //Serial.println(command);  // debugging
    String param = "";

    command.trim();        //kill whitespace
    Serial.println(command);

    int slashAt = command.indexOf('/');
    if (slashAt >= 0 && slashAt < command.length()){
      param = command.substring(slashAt + 1, command.length());
      Serial.println(param);
      command = command.substring(0, slashAt);
    }

    Serial.println(command);

    if (command == "bump") {
      if (param.equals("blue")){
	stripColor  = 0x0000ff; 
      } else if (param.equals("red")){
	stripColor  = 0xff0000; 
      }

      Serial.println("bumping...");
      bumpLED();
      pushStats(client);
    } else if (command == "bright"){
	Serial.print("Brightening:");
	Serial.println(param);
	int value = param.toInt();
	value = max(0, value);
	value = min(255, value);
	brightLED(value);
	pushStats(client);
    } else if (command == "color"){
	Serial.print("color:");
	stripColor = hexStringToLong(param);
	Serial.println(stripColor);
	renderLEDs();
	pushStats(client);
    } else if (command == "randomize"){
      Serial.print("Param");
      Serial.println(param);

	Serial.print("randomize:");
	randomFactor = param.toInt();
	Serial.println(randomFactor);
	renderLEDs();
	pushStats(client);
    }



    // Close connection and free resources.
    client.stop();
    hits++;
  }

  delay(50); // Poll every 50ms
}

long hexStringToLong(String string){
 
  char char_string[string.length()+1];
  string.toCharArray(char_string, string.length()+1);
 
  return strtoul(char_string, 0, 16);
}



void pushStats(YunClient yClient){

      // get the time from the server:
      Process time;
      time.runShellCommand("date");
      String timeString = "";
      while (time.available()) {
        char c = time.read();
        timeString += c;
      }

      yClient.print("Current time on the Yún: ");
      yClient.println(timeString);
      yClient.print("<br>Current Led: ");
      yClient.print(ledID);
      yClient.print("<br>Hits so far: ");
      yClient.print(hits);
}

#define RENDER_PAUSE 30
void renderLEDs(){

  int half = strip.numPixels() /2;
 for(uint16_t i=0; i<half; i++) {

   uint32_t pixelColor = stripColor;

   int mask = 0x0000FF;

   int r = (pixelColor >> 16) &  mask;
   int g = (pixelColor >> 8) & mask;
   int b =  pixelColor & mask;

   strip.setPixelColor(i, applyRandom(r), applyRandom(g), applyRandom(b)); 
   strip.setPixelColor(i + half,  applyRandom(r), applyRandom(g), applyRandom(b)); 

   strip.show();
   delay(RENDER_PAUSE);
  }

}

int applyRandom(int component){

  int amount = random(randomFactor * 2) - randomFactor;
  component += amount;
  component = max(0, component);
  component = min(255, component);
  return component;

}



void bumpLED(){

  strip.setPixelColor(ledID, 0, 0, 0);

  ledID++;
  ledID%=16;
  strip.setPixelColor(ledID, stripColor);

  strip.show();
}

void brightLED(int val){
  ledBright = val;
  strip.setPixelColor(ledID, 0, 0, ledBright);
  strip.show();
}

