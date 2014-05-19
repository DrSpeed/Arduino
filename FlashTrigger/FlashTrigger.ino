/*
 * swdba's board:  Duemilanova w.ATMega 328
 */

#define MIC_PIN  A5
#define KNOB_PIN A4
#define LED_PIN  13
#define OPTO_PIN 6


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);   
  pinMode(OPTO_PIN, OUTPUT);   
  digitalWrite(LED_PIN, LOW);   
}


// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:

  int sensorValue = analogRead(MIC_PIN);
  int knobValue = analogRead(KNOB_PIN);

  sensorValue = sensorValue / 10;
  knobValue   = knobValue / 10;

/*  Some basic debugging
    Serial.print(sensorValue);
    Serial.print(" ");
    Serial.println(knobValue);
    delay(500);
*/
  if (sensorValue < knobValue){  // A sound was heard
    Serial.print(sensorValue);
    Serial.print(" ");
    Serial.println(knobValue);

    // LED ON--------------
    digitalWrite(LED_PIN, HIGH);  // LED is a visual indicator of what is going on

    // Flash Start
    digitalWrite(OPTO_PIN, HIGH);   

    delay(500);  // Wait for flash to finish

    digitalWrite(OPTO_PIN, LOW);   

    // Wait to avoid double bounce
    delay(500);
    // LED OFF-------------------
    digitalWrite(LED_PIN, LOW);   

  }
}

