#include <EEPROM.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TimeAlarms.h>
#include <Time.h>

char* messageIdHeartbeat = "HEARTBEAT";
char* messageIdRestart = "RESTART";
char* messageIdAlarm = "ALARM";

const char* customerId = "3";
//const char* probeId = "16";
const char* probeId = "17";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

// device board layout
const int pinAnalogVoltage = 2;
const int pinVoltage = 5;
const int pinGprsRx = 7;
const int pinGrpsTx = 8;
const int pinGprsPower = 9;
SoftwareSerial GPRS(pinGprsRx, pinGrpsTx);

// probe values
float currentValue1 = 0;
float previousValue1 = 0;
const float value1Threshold = 500;

int currentValue2 = 0;
int previousValue2 = 0;
const int value2Threshold = HIGH;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

// *******************************************************
// arduino methods

void setup() {
  GPRS.begin(9600); 
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("setup");

  oneTimeEepromInit();
  //eepromRead();

  setTime(23,59,59,24,12,2011); // just to begin with something
  gprs_setup();

  previousValue1 = 0;
  currentValue1 = 0;

  previousValue2 = LOW;
  currentValue2 = LOW;

  // wait 1 minute before sending out restart message
  delay(60000);
  measure();
  restart();

  Alarm.timerRepeat(60, heartbeat); // 14400 sec = 4 h, 86400 = 24 h

  // init board layout
  pinMode(pinAnalogVoltage, INPUT);
  pinMode(pinVoltage, INPUT);
}

void loop() {
  Serial.print(F("loop "));
  measure();
  Serial.print(previousValue1);
  Serial.print(" -> ");
  Serial.println(currentValue1);
  if (currentValue1 >= value1Threshold && previousValue1 < value1Threshold) {
    alarm();
  }  
  if (currentValue1 <= value1Threshold && previousValue1 > value1Threshold) {
    alarm();
  }  
  Serial.print(previousValue2);
  Serial.print(" -> ");
  Serial.println(currentValue2);
  if (currentValue2 >= value2Threshold && previousValue2 < value2Threshold) {
    alarm();
  }  
  if (currentValue2 <= value2Threshold && previousValue2 > value2Threshold) {
    alarm();
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
  payloadValue2();
}

char* payloadValue1(char* v) {
  Serial.print(F("payloadValue1 "));
  // analog temperature from grove
  char value[] = "     ";
  
  // read analog value X times and calc average of it
  const int maxReadings = 20;
  int readings[maxReadings];
  int total = 0;
  for (int i = 0; i < maxReadings; i++) {
    total = total + analogRead(pinAnalogVoltage); 
    delay(100);
  }
  int average = total / maxReadings;
  
  int B=3975;
  float resistance=(float)(1023-average)*10000/average;
  float temperature=1/(log(resistance/10000)/B+1/298.15)-273.15;
  previousValue1=currentValue1;
  currentValue1 = temperature;
  ftoa(value, temperature, 2);
  strcpy(v, value);
  return v;
}

char* payloadValue2() {
  Serial.print(F("payloadValue2 "));
  previousValue2=currentValue2;
  currentValue2 = digitalRead(pinVoltage);
}


void alarm() {
  char m[50];
  message(m, messageIdAlarm);
  gprs_sendMessage(m);
  outgoingMessageCount++;
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void restart() {
  char m[50];
  message(m, messageIdRestart);
  gprs_sendMessage(m);
  restartCount++;
  outgoingMessageCount++;
  EEPROM.write(restartCountAdr, restartCount);
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void heartbeat() {
  char m[50];
  message(m, messageIdHeartbeat);
  gprs_sendMessage(m);
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
  itoa(currentValue2, counter, 10);
  strcat(m, counter);
  strcat(m, ",");
  strcat(m, ",");
  return m;
}


// *******************************************************
// GPRS shield specific code

void gprs_setup() {
  switchGprsOn();
  
  delay(1000);
  printATCommand("ATZ");
  delay(3000);
  printATCommand("AT+SAPBR=0,1");
  printATCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  printATCommand("AT+SAPBR=3,1,\"APN\",\"internet\"");
  printATCommand("AT+SAPBR=1,1");
  delay(1000);
}

void switchGprsOn() {
  Serial.println("Powering Up SIM900");
  pinMode(9, OUTPUT);
  digitalWrite(pinGprsPower,LOW);
  delay(100);
  digitalWrite(pinGprsPower,HIGH);
  delay(500);
  digitalWrite(pinGprsPower,LOW);
  delay(100);
  Serial.println("SIM900 Powered Up");
}

void switchGprsOff() {
  Serial.println("Powering Down SIM900");
  // on or off
  pinMode(pinGprsPower, OUTPUT); 
  digitalWrite(pinGprsPower,LOW);
  delay(1000);
  digitalWrite(pinGprsPower,HIGH);
  delay(2000);
  digitalWrite(pinGprsPower,LOW);
  delay(300); //3000
  Serial.println("SIM900 Powered Down");
}

void gprs_sendMessage(char *message) {
  Serial.print(F("gprs_sendMessage: "));
  gprs_httpPost("", 80, "", message);
}

void gprs_httpPost(char *server, int port, char *url, char* d) {
  
  printATCommand("AT+HTTPINIT");
  printATCommand("AT+HTTPPARA=\"CID\",1");
  printATCommand("AT+HTTPPARA=\"URL\",\"http://www.marsmonitoring.com/messages/create_from_probe\"");

  char data[80] = "";
  strcpy(data, "message%5Bdata%5D="); // json not possible with SIM 900, so use old style 
  strcat(data, d);

  String ss = "";
  ss += strlen(data);
  char ssid[ss.length() + 1]; 
  ss.toCharArray(ssid, ss.length() + 1);
  
  char httpdata[25] = "";
  strcpy(httpdata, "AT+HTTPDATA=");
  strcat(httpdata, ssid);
  strcat(httpdata, ",10000");
  printATCommand(httpdata);
  delay(1000);
  printATCommand(data);
  delay(2000);

  printATCommand("AT+HTTPACTION=1");
  delay(10000);
  printATCommand("AT+HTTPTERM");
}

char buffer[64]; // buffer array for data recieve over serial port
int count=0;     // counter for buffer array 

void printATCommand(char* command) {
  Serial.println(command);
  GPRS.println(command);
  delay(500);
  if (GPRS.available())              // if date is comming from softwareserial port ==> data is comming from gprs shield
  {
    while(GPRS.available())          // reading data into char array 
    {
      buffer[count++]=GPRS.read();     // writing data into array
      if(count == 64) break;
    }
    Serial.print(buffer);            // if no data transmission ends, write buffer to hardware serial port
    clearBufferArray();              // call clearBufferArray function to clear the storaged data from the array
    count = 0;                       // set counter of while loop to zero
  }
}

void clearBufferArray()              // function to clear buffer array
{
  for (int i=0; i<count; i++)
  { 
    buffer[i]=NULL;
  }                  // clear all index of array with command NULL
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

