/*
  curl http://arduino.local:5555/arduino/color/050500 
  http://arduino.local:5555/arduino/color/050511

 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

#define PIN 8
#define NLEDS 120
#define MASK 0x0000FF

#define MIDI_PIN  11
#define NO_OP_PIN  10

SoftwareSerial midiSerial(NO_OP_PIN, MIDI_PIN);


// entire transition time
#define TRANS_TIME 1000
// time granule to change color within a whole transition
#define TRANS_INCR 100

#define KNOB_PIN A0    // select the input pin for the potentiometer

int brightValue = 0; // 0..100, convert to float at runtime

// Color blending state variables
uint32_t startColor;
uint32_t curColor;
uint32_t endColor;
long     startTime;
long     endTime;
long     nextColorTime = -1;  // make constant

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NLEDS, PIN, NEO_GRB + NEO_KHZ800);

// Listen on default port 5555, the webserver on the Yún
// will forward there all the HTTP requests for us.
YunServer server;

void setup() {

  Serial.begin(115200);
  Serial.println("Initializing Yun");


  // Bridge startup
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);

  server.noListenOnLocalhost();  // port 5555
  server.begin();

  Serial.println("Done initializing Neopixels");

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  midiSerial.begin(31250);

 Serial.println("Done initializing Yun");

}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  // There is a new client?
  if (client) {
    // Process request
    Serial.println("Got client");
    process(client);

    // Close connection and free resources.
    client.stop();
  }

  manageBrightness();


  if (nextColorTime != -1 && millis() >= nextColorTime){
    nextColorTime = millis() + TRANS_INCR;
    if (nextColorTime >= endTime){
      Serial.println(millis());
      colorWipe(endColor, 0);
      nextColorTime = -1;  // skip
      Serial.print("done changing color ");
    } else {
      Serial.println(millis());
      uint32_t color = CalculateColor(startColor, endColor, startTime, endTime, millis());
      colorWipe(color, 0);
      curColor = color;
      Serial.print("changing color ");
    }
  }

  //  delay(50); // Poll every 50ms
}

/*
 * Handle brightness knob
 *   Update variable
 *   Re-render led strip
 *
 */
void manageBrightness() {

  int sensorValue = map(analogRead(KNOB_PIN), 0, 1023, 0, 25);
  sensorValue *= 4;  // make granules of brightness bigger to avoid knob jitter.

  if (brightValue != sensorValue){

    Serial.print("Changing color: ");
    Serial.println(sensorValue);

    colorWipe(curColor, 0);
    brightValue = sensorValue;


  }
}


void process(YunClient client) {

  String command = client.readString();
  command.trim(); //kill whitespace
  //Serial.println(command);

  //Serial.print("Processing Client: ");
  //Serial.println(command);

  int start = command.indexOf('/');
  command = command.substring(start+1);
  int end = command.indexOf(' ');
  command = command.substring(0, end);

  start = command.indexOf('/');
  command = command.substring(start+1);


  start = command.indexOf('/');
  String param = command.substring(start+1);
  end = command.indexOf('/');
  command = command.substring(0, end);

  Serial.println(command);

  Serial.println(param);


  if (command == "color"){
    long color = hexStringToLong(param);

    // setup transition
    startColor = curColor;
    startTime = millis();
    nextColorTime = startTime+TRANS_INCR;
    endColor = color;
    endTime = startTime + TRANS_TIME;
    Serial.print("nextTime: ");
    Serial.println(nextColorTime);

    color = CalculateColor(startColor, endColor, startTime, endTime, millis());
    colorWipe(color, 0);
    curColor = color;
    return;
  }


  if (command == "midi"){
    long patch = intStringToLong(param);
    patch ++;  // MIDI is zero indexed, patches are 1..
    Serial.print("MIDI ");
    Serial.println(patch);
    midiSerial.write(0xC0);
    midiSerial.write(patch);
    return;
  }


  // is "digital" command?
  if (command == "midi") {
    Serial.println("Digital");
    // Add MIDI command
  }

  // is "mode" command?
  if (command == "mode") {
    modeCommand(client);
  }


}


uint32_t CalculateColor(uint32_t startColor, uint32_t endColor, long startTime, long endTime, long now){

  // time, relative to start of transition
  now = now - startTime;
  
  // how far we are into the transition
  double factor = (float)now / (endTime - startTime); 

  int sr = (startColor >> 16) & MASK;
  int sg = (startColor >> 8) & MASK;
  int sb = startColor & MASK;

  int er = (endColor >> 16) & MASK;
  int eg = (endColor >> 8) & MASK;
  int eb = endColor & MASK;


  int nr = round(sr + ((er - sr) * factor)); 
  int ng = round(sg + ((eg - sg) * factor)); 
  int nb = round(sb + ((eb - sb) * factor)); 

  return strip.Color(nr, ng, nb);

}



void analogCommand(YunClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (client.read() == '/') {
    // Read value and execute command
    value = client.parseInt();
    analogWrite(pin, value);

    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" set to analog "));
    client.println(value);

    // Update datastore key with the current pin value
    String key = "D";
    key += pin;
    Bridge.put(key, String(value));
  }
  else {
    // Read analog pin
    value = analogRead(pin);

    // Send feedback to client
    client.print(F("Pin A"));
    client.print(pin);
    client.print(F(" reads analog "));
    client.println(value);

    // Update datastore key with the current pin value
    String key = "A";
    key += pin;
    Bridge.put(key, String(value));
  }
}

void modeCommand(YunClient client) {
  int pin;

  // Read pin number
  pin = client.parseInt();

  // If the next character is not a '/' we have a malformed URL
  if (client.read() != '/') {
    client.println(F("error"));
    return;
  }

  String mode = client.readStringUntil('\r');

  if (mode == "input") {
    pinMode(pin, INPUT);
    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" configured as INPUT!"));
    return;
  }

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" configured as OUTPUT!"));
    return;
  }

  client.print(F("error: invalid mode "));
  client.print(mode);
}


void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  int r = (c >> 16) & MASK;
  int g = (c >> 8) & MASK;
  int b = c & MASK;

  double factor = (float)brightValue/100.0;

  if (factor < .05){
    r = g = b = 0;
  } else {
    r = round(r * factor);
    g = round(g * factor);
    b = round(b * factor);
  }

  for (i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
}

long hexStringToLong(String string){
  char char_string[string.length()+1];
  string.toCharArray(char_string, string.length()+1);
return strtoul(char_string, 0, 16);
}

long intStringToLong(String string){
  char char_string[string.length()+1];
  string.toCharArray(char_string, string.length()+1);
  return strtoul(char_string, 0, 10);
}


