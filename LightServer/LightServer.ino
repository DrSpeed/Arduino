/*
  curl http://arduino.local:5555/arduino/color/050500 
  http://arduino.local:5555/arduino/color/050511

 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Adafruit_NeoPixel.h>

#define PIN 8
#define NLEDS 16

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

  //  delay(50); // Poll every 50ms
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
    colorWipe(color, 0);
    client.print("Done!");
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

void digitalCommand(YunClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
  }
  else {
    value = digitalRead(pin);
  }

  // Send feedback to client
  client.print(F("Pin D"));
  client.print(pin);
  client.print(F(" set to "));
  client.println(value);

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
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

  for (i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

long hexStringToLong(String string){
  char char_string[string.length()+1];
  string.toCharArray(char_string, string.length()+1);
return strtoul(char_string, 0, 16);
}


