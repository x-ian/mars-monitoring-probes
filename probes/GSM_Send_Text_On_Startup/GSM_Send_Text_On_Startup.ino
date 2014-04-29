#include <SoftwareSerial.h>
SoftwareSerial gprs(7, 8);

//String number = "+491784049573";
String number = "+265888288976";

int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;

void setup()
{
  Serial.begin(9600);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for Leonardo only
//  }
  Serial.println("setup");

  gprs_setup();
  delay(60000);
  gprs_sendTextMessage(number, "Test message upon restart in setup");
}

void loop()
{
  delay(60000);
  gprs_sendTextMessage(number, "Test message in loop");
}

void gprs_setup() {
  gprs_powerUpOrDown();
  delay(500);
  gprs.begin(19200); // the default GPRS baud rate   
  delay(500);
  //gprs_setTime();
  delay(500);
}

void gprs_sendTextMessage(String number, char* message) {
  Serial.print("sendTextMessage: ");
  Serial.println(message);
  gprs.print("AT+CMGF=1\r"); // SMS in text mode
  delay(100);
  gprs.println("AT + CMGS = \"" + number + "\"");
  delay(100);
  gprs.println(message);
  delay(100);
  gprs.println((char)26); // ASCII code of the ctrl+z is 26
  delay(100);
  gprs.println();
}

void gprs_powerUpOrDown() {
  pinMode(pinGprsPower, OUTPUT); 
  digitalWrite(pinGprsPower,LOW);
  delay(1000);
  digitalWrite(pinGprsPower,HIGH);
  delay(2000);
  digitalWrite(pinGprsPower,LOW);
  delay(3000);
}

