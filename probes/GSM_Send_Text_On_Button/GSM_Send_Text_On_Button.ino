/*
  Sends out a simple GSM text message whenever the button is pressed.
  
  Requires: 
   - GSM Shield
   - Seeedstudio Grove Shield (http://www.seeedstudio.com/wiki/GROVE_System)
     - LED connected to pin 3
     - Button connected to pin 5
*/

#include <SoftwareSerial.h>

// inits
int pinLed = 3;
int pinButton = 5;

int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;
SoftwareSerial gprs(7, 8);

// configs
String phone = "+26588828876";
//String phone = "+491784049573";
String message = "This is a friendly test message from your Arduino board.";

void setup()
{
  gprs_powerUpOrDown();
  gprs.begin(19200); // the default GPRS baud rate   
  Serial.begin(19200);    // the GPRS baud rate 
  delay(500);

  pinMode(pinButton, INPUT);
  pinMode(pinLed, OUTPUT);
}

void gprs_powerUpOrDown()
{
  pinMode(pinGprsPower, OUTPUT); 
  digitalWrite(pinGprsPower,LOW);
  delay(1000);
  digitalWrite(pinGprsPower,HIGH);
  delay(2000);
  digitalWrite(pinGprsPower,LOW);
  delay(3000);
}

void loop()
{
  if (digitalRead(pinButton)==HIGH)
  {
    digitalWrite(pinLed, HIGH);
    gprs_sendTextMessage(phone, message);
    delay(1000);
    digitalWrite(pinLed, LOW);
  }
}

void gprs_sendTextMessage(String number, String message)
{
  gprs.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
  delay(100);
  gprs.println("AT + CMGS = \"" + number + "\"");//send sms message, be careful need to add a country code before the cellphone number
  delay(100);
  gprs.println(message);//the content of the message
  delay(100);
  gprs.println((char)26);//the ASCII code of the ctrl+z is 26
  delay(100);
  gprs.println();
}

