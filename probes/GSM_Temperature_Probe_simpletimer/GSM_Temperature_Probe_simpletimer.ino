#include <Time.h>
#include <SimpleTimer.h>
#include <EEPROM.h>
#include <SPI.h>
#include <util.h>
#include <SoftwareSerial.h>

SimpleTimer heartbeatTimer;

// base configs and vars
char* messageIdHeartbeat = "HEARTBEAT";
char* messageIdRestart = "RESTART";
char* messageIdAlarm = "ALARM";
//const char phone[] = "+4915259723556";
String phone = "+4915225839733";

// device configs
const char* customerId = "1";
const char* probeId = "2";

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
float currentValue1 = 0;
float previousValue1 = 0;
const float alarm1Threshold = 23;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

// *******************************************************
// arduino methods

void setup() {
  Serial.begin(19200);
  while (!Serial) { ; } // wait for serial port to connect. Needed for Leonardo only
  Serial.println("setup");

  //oneTimeEepromInit();
  eepromRead();
  
  setTime(23,59,59,24,12,2011); // just to begin with something
  gprs_setup();

  // wait 1 minute before sending out restart message
  delay(60000);
  measure(); // make sure the restart message already has the values
//  restart();

  heartbeatTimer.setInterval(21600000, heartbeat); // every 6 hours

  // init board layout  
  //pinMode(pinButton, INPUT);
  //pinMode(pinLed, OUTPUT); 
}

void loop() {
  Serial.print("loop ");

  Serial.print(previousValue1);
  Serial.print(" -> ");
  Serial.println(currentValue1);
  
  if (currentValue1 >= alarm1Threshold && previousValue1 < alarm1Threshold) {
    // temp was rising above threshold, send out message
    alarm();
  }  
  if (currentValue1 <= alarm1Threshold && previousValue1 > alarm1Threshold) {
    // temp was falling below threshold, send out message
    alarm();
  }  
  delay(60000); // wait for 60 sec
  measure(); // measure was first invoked in setup(), now run at the end of loop()
  heartbeatTimer.run();
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
  measureAnalogTemperatureFromGrove(value);
}

void alarm() {
  char m[50];
  message(m, messageIdAlarm);
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
  strcat(m, probeId);
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
  ftoa(value, currentValue1, 2);
  strcat(m, value);
  strcat(m, ",");
  strcat(m, ",");
  strcat(m, ",");
  return m;
}

char* measureAnalogTemperatureFromGrove(char* v) {
  char value[] = "     ";
  int B=3975; 
  int a=analogRead(pinAnalogTemp);
  float resistance=(float)(1023-a)*10000/a; 
  float temperature=1/(log(resistance/10000)/B+1/298.15)-273.15;
  previousValue1=currentValue1;
  currentValue1 = temperature;
  ftoa(value, temperature, 2);
  strcpy(v, value);
  return v;
}

// *******************************************************
// GSM/GPRS shield specific code

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
  } else {
    strcpy(number, tmp);
  }
  return number;
}

char* ftoa(char *a, float f, int precision) {
  // slightly wrong sometimes, e.g. 23.04 results in 23.4
  long p[] = {
    0,10,100,1000,10000,100000,1000000,10000000,100000000  };
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

