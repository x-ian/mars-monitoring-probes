#include <DHT.h>

DHT dht(14, DHT11);

void setup() {
  Serial.begin(19200);
  Serial.println(F("setup"));
    pinMode(14, INPUT);
/*
delay(1000);
Serial.println("1");
delay(1000);
Serial.println("1");
delay(1000);
Serial.println("1");
delay(1000);
Serial.println("1");
delay(1000);
Serial.println("1");
delay(1000);
Serial.println("1");
delay(1000);
Serial.println("1");*/
 // float h = dht.readHumidity();
  float t = dht.readTemperature();
Serial.println(t);
Serial.println(digitalRead(14));
delay(2000);

}

void loop() {
delay(2000);
  float t = dht.readTemperature();
Serial.println(t);
}
