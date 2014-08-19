/*
 * Sketch for Arduino Yun
 * WebService controller NeoPixel 
 * 
 *  Example url: 
 *     /arduino/bright/25
 * 
 * api:
 *  bright 0..100     // Scale the brightness of the lamp  0 off, 10 very dim,  100 full brightness
 *  color rrggbb      // The the color of the strip, hexidecimal. E.g.:  /arduino/color/FCBEBE
 *  randomize  0..100 // How much to randomize each pixel
 *  skip 0..n         // Skip pixels to make it darker, 2 means every third pixel is illuminated
 *  animate  0..n     // How often to render a random pixel. Value in mSec, 100 being very often 1000 is less often
 *  toggleon          // Toggle the on/off state of the lamp
 *  sleep 1..n        // Sleep timer, seconds to wait until turning all pixels off
 * 
 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Adafruit_NeoPixel.h>
 
#define PIN 5
#define MASK 0x0000FF
#define N_LEDS 16 


// Listen on default port 5555, the webserver on the Yún
// will forward there all the HTTP requests for us.
YunServer server;

// The NeoPixel Strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN);

// Strip control variable and startup values
uint32_t stripColor   = 0x0000ff; // Start color
int      ledBright    = 20;       // 0..100 %
int      randomFactor = 0;
int      skip         = 0;

// Animate settings
long renderInterval = 0;  // 0 means don't animate, any other value is mSeconds
long renderTime     = 0;  // 0 means don't animate, any other value is mSeconds
bool lampOn = true;

// Sleep timer
unsigned long offTime = 0;  // 0 means never, otherwise the mSec time to turn the lamp off

long hits = 0;


void setup() {
  Serial.begin(9600);
  Serial.println("Initializing Yun");

  // Bridge startup
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  strip.begin();
  strip.setBrightness(60); // 1/3 brightness

  PrintTime();

  renderLEDs();
  strip.show();
  Serial.println("Done initializing Yun");

}

// get the time 
void PrintTime(){

  String tString;
  Process tTime;
  tTime.runShellCommand("date");
  while (tTime.available()) {
    char c = tTime.read();
    tString += c;
  }
  Serial.println(tString);
}


void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  unsigned long now = millis();
  if (offTime > 0 && now >= offTime){
    Serial.print("Sleeping Lamp NOW!");
    lampOn = false;
    renderLEDs();
    offTime = 0;  // reset
  }

  // If we're animating, do it every loop
  HandleAnimation(now);

  // There is a new client?
  if (client) {
    // read the command/url 
    String command = client.readString();
    String param = "";

    command.trim();        //kill whitespace
    Serial.println(command);

    // get command's parameter from input string
    int slashAt = command.indexOf('/');
    if (slashAt >= 0 && slashAt < command.length()){
      param = command.substring(slashAt + 1, command.length());  // Get parameter from input string/url
      Serial.println(param);
      command = command.substring(0, slashAt);  // Get command from string/url
    }

    if (command == "bright"){   // Relative brightness
      Serial.print("Bright:");
      Serial.println(param);
      int value = param.toInt();
      value = max(0, value);
      value = min(100, value);
      ledBright = value;
      lampOn = true;
      renderLEDs();
      pushStats(client);
    } else if (command == "color"){   // Color
      Serial.print("color:");
      stripColor = hexStringToLong(param);
      Serial.println(stripColor);
      lampOn = true;
      renderLEDs();
      pushStats(client);
    } else if (command == "randomize"){  // Randomize
      Serial.print("Param");
      Serial.println(param);
      Serial.print("randomize:");
      randomFactor = param.toInt();
      Serial.println(randomFactor);
      lampOn = true;
      renderLEDs();
      pushStats(client);
    } else if (command == "skip"){  // Pixel skip
      Serial.print("Param");
      Serial.println(param);
      Serial.print("skip:");
      skip = param.toInt();
      Serial.println(skip);
      lampOn = false;
      renderLEDs();
      lampOn = true;
      renderLEDs();
      pushStats(client);
    } else if (command == "animate"){ // animation
      Serial.print("Param");
      Serial.println(param);
      Serial.print("animate:");
      renderInterval = param.toInt();
      Serial.println(renderInterval);
      lampOn = true;
      renderLEDs();
      if (renderInterval == 0){
	renderTime = 0;
      } else {
	renderTime = millis(); // now
      }
      pushStats(client);
    } else if (command == "toggleon"){  // on/off toggle
      Serial.print("Toggling Lamp");
      if (lampOn == true){
	lampOn = false;
	renderLEDs();
      } else { // turn on
	lampOn = true;
	renderLEDs();
      }
      pushStats(client);
    } else if (command == "sleep"){ // input in minutes
      Serial.print("Sleeping Lamp: ");
      Serial.print(param.toInt());
      Serial.println(" Minutes.");
      offTime = millis() + (param.toInt() * 1000 * 60); // now + minutes to mSec
      Serial.print("Offtime: ");
      Serial.println(offTime);
      Serial.print("Millis: ");
      Serial.println(millis());
      pushStats(client);
    } else if (command == "status"){  // status
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


void HandleAnimation(unsigned long now){

  if (renderTime != 0 && renderTime <= now){
    animateLED();
    renderTime = now + renderInterval;
  } 
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
      yClient.print("<br>Current Color: ");
      yClient.print(stripColor, HEX);
      yClient.print("<br>Brightness: ");
      yClient.print(ledBright);

      yClient.print("<br>Random: ");
      yClient.print(randomFactor);
      yClient.print("<br>Pixel Skip: ");
      yClient.print(skip);

      if (lampOn){
	yClient.print("<br>Lamp is turned on.");
      } else {
	yClient.print("<br>Lamp is turned off.");
      }

      yClient.print("<br>Hits so far: ");
      yClient.print(hits);


      // If the sleep timer is set, display it
      if (offTime > 0){
	unsigned long offInMinutes = offTime - millis();
	offInMinutes = offInMinutes / 1000; // msec to minutes
	yClient.print("<br>Turning off in: ");
	if (offInMinutes < 60){
	  yClient.print(offInMinutes);
	  yClient.print(" Seconds.");
	} else {
	  offInMinutes /= 60;
	  yClient.print(offInMinutes);
	  yClient.print(" Minutes.");
	  
	}
      }
}


// Change 1 random led on string, using global settings
void animateLED(){

  if (lampOn == false){
    return;
  }

  // Colors here are RRGGBB
  int r = (stripColor >> 16) & MASK;
  int g = (stripColor >> 8)  & MASK;
  int b =  stripColor        & MASK;

  // Adjust brightness (the simple way)
  r = factorColor(r);
  g = factorColor(g);
  b = factorColor(b);

  uint32_t pixel;

  if (skip == 0){
    pixel = random(strip.numPixels());  // get a random pixel
  } else {
    pixel = random(0, strip.numPixels()/skip +1);
    pixel *= (skip + 1);
  }

  strip.setPixelColor(pixel, applyRandom(r), applyRandom(g), applyRandom(b)); 
  strip.show();
}



#define RENDER_PAUSE 40
void renderLEDs(){

  int bump = (lampOn == false) ? 1: (1 + skip);

  int half = strip.numPixels() /2;
  for(uint16_t i=0; i<strip.numPixels(); i+=bump)  {
    
    if (lampOn == true){
      uint32_t pixelColor = stripColor;

      int mask = 0x0000FF;

      int r = (pixelColor >> 16) &  mask;
      int g = (pixelColor >> 8) & mask;
      int b =  pixelColor & mask;

      // Adjust brightness (the simple way)
      r = factorColor(r);
      g = factorColor(g);
      b = factorColor(b);

      strip.setPixelColor(i, applyRandom(r), applyRandom(g), applyRandom(b)); 
    } else { // lamp off
      strip.setPixelColor(i,         0, 0, 0);
    }
    strip.show();
    delay(RENDER_PAUSE);
  }

}

int factorColor(int colorIn){
    double brFactor = ledBright / 100.0;
    
    return round(colorIn * brFactor);


}


int applyRandom(int component){

  int r = factorColor(randomFactor);
  int amount = random(r * 2) - r;
  component += amount;
  component = max(0, component);
  component = min(255, component);
  return component;

}



