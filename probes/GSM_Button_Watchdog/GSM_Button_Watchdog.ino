/*
 LED: 
   1. connect - to GND
   2. connect + to digital pin 4
 Button: 
   1. connect C to GND 
   2. conenct NC1 (the one next to +) to digital pin 7 and +
   
 Careful: Sktech uses internal pullup resistors, don't change pins without knowing what this means!
 
*/


void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  pinMode(4, OUTPUT);    // sets the digital pin as output for LED
  pinMode(7, INPUT);    // sets the digital pin as input to read switch
  // use internal pullups
  digitalWrite(7, HIGH);
}

void loop() {
  if (digitalRead(7) == HIGH) {
    delay(500);
    if (digitalRead(7) == HIGH) {
      delay(500);
      if (digitalRead(7) == HIGH) {
        // button pressed for more than 1 second
        digitalWrite(4, HIGH);
        // sending SMS
        if (sendNotification()) {
          // ok, wait 5 seconds on switch off LED
          delay(5000);
          digitalWrite(4, LOW);
        }
      }
    }
  }
}

boolean sendNotification() {
  return false;
}
