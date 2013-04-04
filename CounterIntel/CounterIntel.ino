//
//  Counter intel
//
//  Arduino Nano w/ATmega328
//

#include "SmoothAnalogInput.h"

// Analog Pins----------------------------------------------------
#define SENSOR_PIN  A3      // select the input pin for the Rangefinder
#define KNOB_PIN    A0      // input pin for the range knob
//----------------------------------------------------------------

// Digital Pins---------------------------------------------------
#define  GUN_PIN       5       
#define  SWITCH_PIN   10
#define  LED_PIN      13
//---------------------------------------------------------------

// Input smoothing, from Arduino IDE sample--------------------------
const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average
//------------------------------------------------------------------

#define KNOB_MIN  5    
#define KNOB_MAX  1015


SmoothAnalogInput aiSensor;
SmoothAnalogInput aiKnob;

// Settings:
// COM3,ATMega/nano328
//


void setup()
{
  // Configure IO
  Serial.begin(9600);           // LCD
  pinMode(SWITCH_PIN,  INPUT);  // ARM SWITCH

  pinMode(GUN_PIN,    OUTPUT);  // GUN OPTO-ISOLATOR
  pinMode(LED_PIN,    OUTPUT);  // BOARD MOUNTED SIGNAL LED

  aiKnob.attach(KNOB_PIN);
  aiSensor.attach(SENSOR_PIN);

  backlightOn();

  // Splash Screen
  selectLineOne();
  Serial.print("Counter Intel.");
  selectLineTwo();
  Serial.print("Cats beware!");

  // Sensor smoothing, initialize all the readings to 0: 
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;      

  delay(3000);
  clearLCD();
}

// Startup values
bool armed       = false;
int  lastTrigger = -1;
int  lastSensor  = -1;
bool triggered   = false;

long lastCheck = -1;
long CHECK_INTERVAL = 500;

void loop()
{  
  bool armed   = digitalRead(SWITCH_PIN);
  int  trigger = aiKnob.read();   // analogRead(KNOB_PIN);
  int  sensor  = aiSensor.read();

  long now = millis();

  if (now < (lastCheck + CHECK_INTERVAL))
    return; // too soon
  
  lastCheck = now;

  if (sensor != lastSensor){
    selectLineOne();
    Serial.print("Sensor:" +  buildFixedString(6, sensor));
    lastSensor = sensor;
  }

  if (trigger != lastTrigger){
    selectLineTwo();
    String armString = armed?"Armed:  ": "Unarmed:";
    Serial.print(armString + buildFixedString(5, trigger));
    lastTrigger = trigger;
  }

  // Manage alarm state
  if (sensor <= trigger){
    if(!triggered){
      digitalWrite(LED_PIN, LOW);  
      triggered = true;
    }
  } else {
    if (triggered){
      digitalWrite(LED_PIN, HIGH);  
      triggered = false;
    }
  }


  //  delay(200);


}


String buildFixedString(int nChars, int value){
  
  String num = String(value);
  String retString;
  int padding = nChars - num.length();
  for (int i=0; i<padding; i++){

    retString += " ";
  }
  return retString + num;
}


void selectLineOne(){  //puts the cursor at line 0 char 0.
  Serial.write(0xFE);   //command flag
  Serial.write(128);    //position
  delay(10);
}
void selectLineTwo(){  //puts the cursor at line 0 char 0.
  Serial.write(0xFE);   //command flag
  Serial.write(192);    //position
  delay(10);
}
void goTo(int position) { //position = line 1: 0-15, line 2: 16-31, 31+ defaults back to 0
  if (position<16){ Serial.write(0xFE);   //command flag
    Serial.write((position+128));    //position
  }else if (position<32){Serial.write(0xFE);   //command flag
    Serial.write((position+48+128));    //position 
  } else { goTo(0); }
  delay(10);
}

void clearLCD(){
  Serial.write(0xFE);   //command flag
  Serial.write(0x01);   //clear command.
  delay(10);
}
void backlightOn(){  //turns on the backlight
  Serial.write(0x7C);   //command flag for backlight stuff
  Serial.write(157);    //light level.
  delay(10);
}
void backlightOff(){  //turns off the backlight
  Serial.write(0x7C);   //command flag for backlight stuff
  Serial.write(128);     //light level for off.
  delay(10);
}
void serCommand(){   //a general function to call the command flag for issuing all other commands   
  Serial.write(0xFE);
}


int readRangefinder(){
 // subtract the last reading:
  total= total - readings[index];         
  // read from the sensor:  
  readings[index] = analogRead(SENSOR_PIN); 
  // add the reading to the total:
  total= total + readings[index];       
  // advance to the next position in the array:  
  index = index + 1;                    

  // if we're at the end of the array...
  if (index >= numReadings)              
    // ...wrap around to the beginning: 
    index = 0;                           

  // calculate the average:
  return (total / numReadings);         
}
