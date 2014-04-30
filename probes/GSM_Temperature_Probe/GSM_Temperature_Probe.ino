#include <EEPROM.h>

#include <MemoryFree.h>

#include <SPI.h>

#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#include <ICMPPing.h>

#include <TimeAlarms.h>

#include <Time.h>

#include <SoftwareSerial.h>

// base configs and vars
char* messageIdHeartbeat = "HEARTBEAT";
char* messageIdRestart = "RESTART";
char* messageIdPayload = "PAYLOAD";
//const char phone[] = "+4915259723556";
//const char phone[] = "+265884781634";
//const char phone[] = "+265888288976";
const char phone[] = "+265881007201";
//String phone = "+491784049573";


// device configs
//water configs deviceID="3"
//Fridge configs deviceID="14"
// Ward cofigs deviceID=

const char* customerId = "2";
const char* deviceId = "3";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

// device board layout
int pinAnalogTemp = 0;
int pinLed = 3;
int pinButton = 5;
int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;
SoftwareSerial gprs(7, 8);

// probe values
float currentPayloadValue1 = 0;
float previousPayloadValue1 = 0;
const float payloadValue1Threshold = 12;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

// *******************************************************
// arduino methods

void setup() {
  Serial.begin(19200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("setup");

  //oneTimeEepromInit();
  eepromRead();

  setTime(23,59,59,24,12,2011); // just to begin with something
  gprs_setup();

  previousPayloadValue1 = 0;
  currentPayloadValue1 = 0;

  // wait 1 minute before sending out restart message
  delay(10000);
  restart();

  Alarm.timerRepeat(14400, heartbeat); // 14400 sec = 4 h

  // init board layout
  //pinMode(pinButton, INPUT);
  //pinMode(pinLed, OUTPUT);
}

void loop() {
  Serial.print("loop ");
  measure();

  Serial.print(previousPayloadValue1);
  Serial.print(" -> ");
  Serial.println(currentPayloadValue1);

  if (currentPayloadValue1 >= payloadValue1Threshold &&
    previousPayloadValue1 < payloadValue1Threshold) {
    // temp was rising above threshold, send out message
    payload();
  }
  if (currentPayloadValue1 <= payloadValue1Threshold &&
    previousPayloadValue1 > payloadValue1Threshold) {
    // temp was falling below threshold, send out message
    payload();
  }
  Alarm.delay(10000); // wait for 60 sec
}

// *******************************************************
// i.a.m. logic

void oneTimeEepromInit() {
  // make sure this is really only invoked once!
  EEPROM.write(incomingMessageCountAdr, 0);
  EEPROM.write(outgoingMessageCountAdr, 0);
  EEPROM.write(restartCountAdr, 0);
}

void eepromRead() {
  incomingMessageCount = EEPROM.read(incomingMessageCountAdr);
  outgoingMessageCount = EEPROM.read(outgoingMessageCountAdr);
  restartCount = EEPROM.read(restartCountAdr);
}

void measure() {
  char value[6];
  payloadValue1(value);
}

void payload() {
  char m[50];
  message(m, messageIdPayload);
  gprs_sendTextMessage(phone, m);
  outgoingMessageCount++;
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void restart() {
  char m[50];
  message(m, messageIdRestart);
  gprs_sendTextMessage(phone, m);
  restartCount++;
  outgoingMessageCount++;
  EEPROM.write(restartCountAdr, restartCount);
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void heartbeat() {
  char m[50];
  message(m, messageIdHeartbeat);
  gprs_sendTextMessage(phone, m);
  outgoingMessageCount++;
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

char* message(char* m, char* messageId) {
  char counter[3];
  strcpy(m, messageId);
  strcat(m, ",");
  strcat(m, customerId);
  strcat(m, ",");
  strcat(m, deviceId);
  strcat(m, ",");
  itoa(outgoingMessageCount, counter, 10);
  strcat(m, counter);
  strcat(m, ",");
  itoa(restartCount, counter, 10);
  strcat(m, counter);
  strcat(m, ",");
  char time[16];
  strcat(m, currentTime(time));
  strcat(m, ",");
  char value[6];
  ftoa(value, currentPayloadValue1, 2);
  strcat(m, value);
  strcat(m, ",");
  strcat(m, ",");
  strcat(m, ",");
  return m;
}

char* payloadValue1(char* v) {
  Serial.print("payloadValue1 ");
  // analog temperature from grove
  char value[] = "     ";
  int B=3975;
  int a=analogRead(pinAnalogTemp);
  float resistance=(float)(1023-a)*10000/a;
  float temperature=1/(log(resistance/10000)/B+1/298.15)-273.15;
  previousPayloadValue1=currentPayloadValue1;
  currentPayloadValue1 = temperature;
  ftoa(value, temperature, 2);
  strcpy(v, value);
  return v;
}

// *******************************************************
// GSM/GPRS shield specific code

void gprs_setup() {
  //gprs_powerUpOrDown();
  delay(500);
  if (gprs_alreadyOn()) {
    // switch off and on again
    gprs_powerUpOrDown();
    delay(1000);
    gprs_powerUpOrDown();
  } 
  else {
    gprs_powerUpOrDown();
  }

  delay(500);
  gprs.begin(19200); // the default GPRS baud rate
  delay(500);
  //gprs_setTime();
  delay(500);
}

boolean gprs_alreadyOn() {
  boolean on = false;
  gprs.println("ATE0");
  delay(500);
  while (gprs.available() > 0) { 
    gprs.read(); 
  }
  delay(500);
  gprs.println("ATZ");
  delay(500);
  char c = ' ';
  int i= 0;
  while (gprs.available() > 0) {
    c = gprs.read();
    if (i == 0) on = true;
    if (i == 2 && c != 'O') on = false;
    if (i == 3 && c != 'K') on = false;
    i++;
  }
  return on;
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
  delay(1000);
  digitalWrite(pinGprsPower,LOW);
  delay(1000);
}

// *****************************************************************
// Helpers

char* currentTime(char* time) {
  char number[5];
  strcpy(time, formatNumber(number, year()));
  strcat(time, formatNumber(number, month()));
  strcat(time, formatNumber(number, day()));
  strcat(time, "-");
  strcat(time, formatNumber(number, hour()));
  strcat(time, formatNumber(number, minute()));
  strcat(time, formatNumber(number, second()));
  return time;
}

char* formatNumber(char* number, int digits){
  char tmp[5];
  itoa(digits, tmp, 10);
  if(digits < 10) {
    strcpy(number, "0");
    strcat(number, tmp);
  } 
  else {
    strcpy(number, tmp);
  }
  return number;
}

char* ftoa(char *a, float f, int precision)
{
  // slightly wrong sometimes, e.g. 23.04 results in 23.4
  long p[] = {
    0,10,100,1000,10000,100000,1000000,10000000,100000000    };
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

