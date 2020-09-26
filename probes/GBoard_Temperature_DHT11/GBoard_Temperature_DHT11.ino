#include <GSM_Shield.h>

#include <EEPROM.h>
#include <SPI.h>
//#include <util.h>
#include <TimeAlarms.h>
#include <Time.h>
#include <SoftwareSerial.h>
#include <DHT.h>

GSM gsm;

// base configs and vars
char* messageIdHeartbeat = "HEARTBEAT";
char* messageIdRestart = "RESTART";
char* messageIdAlarm = "ALARM";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

SoftwareSerial gprs(2, 3);

// probe values
float currentValue1 = 0;
float previousValue1 = 0;
float currentValue2 = 0;
float previousValue2 = 0;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

// *******************************************************
// CHANGE ME
// device configs
const char* customerId = "1";
const char* probeId = "2";
// device board layout
int pinAnalogTemp = 14;
const float alarm1Threshold = 23;
const int heartbeatIntervall = 3600;  // 14400 sec = 4 h
DHT dht(pinAnalogTemp, DHT11);

// *******************************************************
// arduino methods

void setup() {
  Serial.begin(19200);
  delay(5000);
  Serial.println(F("setup"));

  //oneTimeEepromInit();
  eepromRead();
  
  setTime(23,59,59,24,12,2016); // just to begin with something
  if (!initGsm()) {
    Serial.println(F("Error during GSM initialization"));
  }

  // wait 1 minute before sending out restart message
  delay(60000);
  //measure(); // make sure the restart message already has the values
  restart();

  Alarm.timerRepeat(heartbeatIntervall, heartbeat);

  // CHANGE ME: 
  // init board layout  
  pinMode(pinAnalogTemp, INPUT);
}

void loop() {
  Serial.print(F("loop "));
  measure();
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
  
  Serial.print(previousValue2);
  Serial.print(" -> ");
  Serial.println(currentValue2);

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
  payloadValue2(value);
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
  ftoa(value, currentValue2, 2);
  strcat(m, value);
  strcat(m, ",");
  strcat(m, ",");
  return m;
}

// *******************************************************
// Gboard specific code

boolean initGsm() {
  gsm.TurnOn(9600);              //module power on
  gsm.InitParam(PARAM_SET_1);//configure the module  
  gsm.Echo(1);               //enable AT echo 
  return true;
}

void gprs_sendMessage(char *message) {
  Serial.print(F("gprs_sendMessage: "));
  gprs_disconnect();
  delay(10000);
  gprs_connect();
  delay(10000);
  gprs_httpPost("", 80, "", message);
}

void gprs_connect() {
  Serial.println(F("init GPRS data"));
  delay(30000); // give it time to connect
  gboard_printATCommand("AT+SAPBR=3,2,\"CONTYPE\",\"GPRS\"");
  gboard_printATCommand("AT+SAPBR=3,2,\"APN\",\"internet\"");
  gboard_printATCommand("AT+SAPBR=1,2");
  Serial.println(F("init GPRS data done"));
}

void gprs_disconnect() {
  Serial.println(F("disconnect GPRS data"));
  gboard_printATCommand("AT+SAPBR=0,2");
}

void gprs_httpPost(char *server, int port, char *url, char* d) {
  
  gboard_printATCommand("AT+HTTPINIT");
  gboard_printATCommand("AT+HTTPPARA=\"CID\",2");
  gboard_printATCommand("AT+HTTPPARA=\"URL\",\"http://www.marsmonitoring.com/messages/create_from_probe\"");

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
  gboard_printATCommand(httpdata);
  delay(1000);
  gboard_printATCommand(data);
  delay(2000);

  gboard_printATCommand("AT+HTTPACTION=1");
  delay(10000);
  gboard_printATCommand("AT+HTTPTERM");
}

void gboard_printATCommand(char* command) {
  Serial.println(command);
  gsm.SendATCmdWaitResp(command, 1000, 50, "OK", 1);
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

// *****************************************************************
// Collect values from sensors
// CHANGE ME

char* payloadValue1(char* v) {
  Serial.print(F("payloadValue1 "));
  // analog temperature from grove
  char value[] = "     ";
  
  float temperature=dht.readTemperature();
  previousValue1=currentValue1;
  currentValue1 = temperature;
  ftoa(value, temperature, 2);
  strcpy(v, value);
  return v;
}

char* payloadValue2(char* v) {
  Serial.print(F("payloadValue2 "));
  // analog readHumidity from grove
  char value[] = "     ";
  
  float temperature=dht.readHumidity();
  previousValue2=currentValue2;
  currentValue2 = temperature;
  ftoa(value, temperature, 2);
  strcpy(v, value);
  return v;
}


