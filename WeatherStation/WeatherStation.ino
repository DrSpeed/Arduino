#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <DHT.h>

#include "plotly_streaming_cc3000.h"

#define WLAN_SSID       "GNGUEST"
#define WLAN_PASS       "puffygnow"
#define WLAN_SECURITY   WLAN_SEC_WPA2

// Sign up to plotly here: https://plot.ly
// View your API key and streamtokens here: https://plot.ly/settings
#define nTraces 2
// View your tokens here: https://plot.ly/settings
// Supply as many tokens as data traces
// e.g. if you want to ploty A0 and A1 vs time, supply two tokens
char *tokens[nTraces] = {"oxhyazz265", "ww1qr8auua"};
// arguments: username, api key, streaming token, filename
plotly graph("DrSpeed", "2p39jeabln", tokens, "filename", nTraces);

// DHT Sensor Setup
#define DHTPIN 7 // We have connected the DHT to Digital Pin 2
#define DHTTYPE DHT11 // This is the type of DHT Sensor (Change it to DHT11 if you're using that model)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT object


float humids[100];
float tems[100];


/*
 *  --SETUP--
 */
void setup() {
  graph.maxpoints = 100;
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  wifi_connect();

  bool success;
  success = graph.init();
  if(!success)
    {
      while(true){}
    }

  graph.openStream();

  dht.begin();

  freeRam();

}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  int fr = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
  Serial.print("Free ram: ");
  Serial.println(fr);

}


/*
 *  --LOOP--
 */
void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  graph.plot(millis(), t, tokens[0]);
  //  graph.plot(millis(), h, tokens[1]);
}


void wifi_connect(){
  /* Initialise the module */
  Serial.println(F("\n... Initializing..."));
  if (!graph.cc3000.begin())
    {
      Serial.println(F("... Couldn't begin()! Check your wiring?"));
      while(1);
    }

  // Optional SSID scan
  // listSSIDResults();

  if (!graph.cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }

  Serial.println(F("... Connected!"));

  /* Wait for DHCP to complete */
  Serial.println(F("... Request DHCP"));
  while (!graph.cc3000.checkDHCP())
    {
      delay(100); // ToDo: Insert a DHCP timeout!
    }
  Serial.println(F("... Got DHCP, completed wifi_connect"));
}
