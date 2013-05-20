#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <SPI.h>
#include <util.h>
#include <SimpleTimer.h>
#include <SoftwareSerial.h>
#include <Time.h>

// base configs and vars
char messageIdHeartbeat[] = "HEARTBEAT";
char messageIdRestart[] = "RESTART";
char messageIdPayload[] = "ALARM";

// device configs
const char* customerId = "1";
// 12 for Germany, 13 for Malawi
const char* deviceId = "13";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

// probe values
int currentPayloadValue1 = 0;
int previousPayloadValue1 = 0;
const int payloadValue1Threshold = 17;
int currentPayloadValue2 = 0;
int previousPayloadValue2 = 0;
const int payloadValue2Threshold = 18000;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;
SoftwareSerial gprs(7, 8);

SimpleTimer checkForNewSmsTimer;
SimpleTimer heartbeatTimer;

// needs to match value of _SS_MAX_RX_BUFF in SoftwareSerial.h 
// default 64 is not enough, so increase it!
// (found in somewhere like /Applications/Arduino.app/Contents/Resources/Java/libraries/SoftwareSerial)
#define INCOMING_BUFFER_SIZE 128

char incomingNumber[20];
char incomingMessage[INCOMING_BUFFER_SIZE];
char incomingTimestamp[20];

// arduino ethernet
//byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x6F, 0x35 };
// germany
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xBB, 0xE1 }; 
byte ip[] = { 
// 172.16.1.88 for neno, 192.168.1.10 for mainz
  172, 16, 1, 88 };
//  192, 168, 1, 113 };

// mars server
//char marsServer[] = "192.168.1.5";
//int marsPort = 3000;
char marsServer[] = "www.marsmonitoring.com";
int marsPort = 80;
char marsUrl[] = "/messages/create_from_probe";
EthernetClient marsClient;

void setup()
{
  Serial.begin(19200); // the Serial port of Arduino baud rate.
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("setup");

  //oneTimeEepromInit();
//  eepromRead();

  gprs_setup();
  ether_setup();
  delay(1000);

  checkForNewSmsTimer.setInterval(60000, checkForNewSms); // check every 60 secs for new incoming SMS
//  heartbeatTimer.setInterval(90000, heartbeat); 
  delay(5000);
  restart();
}

void loop() {
  checkForNewSmsTimer.run();
//  heartbeatTimer.run();
}

void gprs_setup() {
  Serial.println("gprs_setup");
  gprs_powerUpOrDown();
  delay(500);
  gprs.begin(19200); // the default GPRS baud rate   
  delay(500);
  //gprs_setTime();
  delay(500);
  gprs.println("ATE0"); // turn off echo mode
  delay(100);
  gprs.println("AT+CMGF=1"); // SMS in text mode
  delay(100);
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

void ether_setup() {
  Serial.print("ether_setup: (might block) ... ");
  Ethernet.begin(mac, ip);
  Serial.println(" completed");
}

void heartbeat() {
  Serial.println("heartbeat");
  char m[50];
  message(m, messageIdHeartbeat);
  ether_sendMessage(m);
  Serial.println();
}

void restart() {
  char m[50];
  message(m, messageIdRestart);
  ether_sendMessage(m);
  restartCount++;
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
//  itoa(currentPayloadValue1, value, 10);
//  strcat(m, value);
//  strcat(m, "3");
  strcat(m, ",");
//  strcat(m, ",");
//  itoa(currentPayloadValue2, value, 10);
//  strcat(m, value);
//  strcat(m, "4");
  strcat(m, ",");
//  strcat(m, "5");
  strcat(m, ",");
//  strcat(m, "6");
  strcat(m, ",");
  return m;
}

boolean ether_sendMessage(char * message) {
  Serial.print("ether_sendMessage: ");
  Serial.println(marsServer);

  if(ether_httpPost(marsServer, marsPort, marsUrl, message)) {
    outgoingMessageCount++;
//    EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
    return true;
  } else {
    return false;
  }
}

boolean ether_httpPost(char * server, int port, char * url, char * d) {
  boolean r = false;

  String data = "{\"message\":{\"data\":\"";
  data += d;
  data += "\"}}";
  Serial.println("ether_httpPost: " + data);
  if (marsClient.connect(server,port)) {
    Serial.print("ether_httpPost: trying to connect ... ");
    marsClient.print("POST ");
    marsClient.print(url);
    marsClient.println(" HTTP/1.1");
    marsClient.print("Host: ");
    marsClient.println(server);
    marsClient.println("Content-Type: application/json");
    marsClient.println("Accept: application/json");
    marsClient.println("Connection: close");
    marsClient.print("Content-Length: ");
    marsClient.println(data.length());
    marsClient.println();
    marsClient.print(data);
    marsClient.println();
  }
  delay(5000);

  if (marsClient.connected()) {
    // check for "HTTP/1.1 201 Created"
    Serial.println("... connected");
    char response[20];
    for (int i = 0; marsClient.connected() && marsClient.available() && i < 20; i++) {
      response[i] = (char) marsClient.read();
    }
    response[19]='\0';
    if (strcmp(response, "HTTP/1.1 201 Create") == 0) {
      // ok, assume post was successful
      r = true;
    }
    marsClient.stop();
  } else {
    Serial.println("... ERROR CONNECTING");
  }
  return r;
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

void checkForNewSms() {
  Serial.println("checkForNewSms");
  
  int incomingStoragePosition = gprs_nextAvailableTextIndex();
  if (incomingStoragePosition > -1) {
    gprs_readTextMessage(incomingStoragePosition);
    // check incoming message
    if (checkIncomingTextMessage()) {
      Serial.println("valid message layout");
      int i = payloadOfIncomingMessage();
      if (i > -1) {
        Serial.println("message with content");
        // forward incoming message
        if (ether_sendMessage(&incomingMessage[i])) {
          Serial.println("message forwarded");
          gprs_deleteTextMessage(incomingStoragePosition);
        } else {
          Serial.println("marsmonitoring.com unavailable, keeping the message");
        }
      } else {
        Serial.println("message without any content");
        gprs_deleteTextMessage(incomingStoragePosition);
      }
    } else {
      // something seems wrong with the SMS, delete it for now
      // maybe have another dedicated notification channel for such cases?
      Serial.println("Unknown response from board");
      gprs_deleteTextMessage(incomingStoragePosition);
    }
  }
  Serial.println("checkForNewSms done");
  Serial.println();
}

// *******************************************************
// GSM/GPRS shield specific code

int gprs_nextAvailableTextIndex() {
//  Serial.println("gprs_nextAvailableTextIndex");
  // make sure nothing unexpected is coming in
  delay(500);
  while (gprs.available() > 0) {
    gprs.read();
  }

  gprs.println("AT+CMGL=\"ALL\"");
  delay(500);
  int index = -1;
  int i = 0;
  char c = ' ';
  
  while (gprs.available() > 0) {
    c = gprs.read();
    Serial.print(c);
    // assume output of CMGL is always '  +CMGL: <one digit number at position 9>'
    // TODO what if number is bigger than 9?
    if (i == 9) index = (int) c;
    i++;
  }
  index = index - 48; //poor man converts ASCII to int like this...
  Serial.println();
  Serial.print("gprs_nextAvailableTextIndex: ");
  Serial.println(index);
  return index;
}

void gprs_readTextMessage(int storagePosition) {
  // make sure nothing unexpected is coming in
  delay(500);
  while (gprs.available() > 0) {
    gprs.read();
  }

  gprs.print("AT+CMGR=");
  gprs.println(storagePosition);
  delay(500);
  int count = 0;
  int newlineCount = 0;
  char c = ' ';
  while (gprs.available() > 0 && newlineCount < 3 && count < INCOMING_BUFFER_SIZE) {
    c = gprs.read();
    if (c == '\r') newlineCount++;
    incomingMessage[count++] = c;
  }
  incomingMessage[count-1]='\0';

  // make sure nothing unprocessed is still waiting
  while (gprs.available()) {
    gprs.read();
  }
}

void gprs_deleteTextMessage(int storagePosition) {
  Serial.println("deleteTextMessage: " + storagePosition);

  delay(500);
  gprs.print("AT+CMGD=");
  gprs.println(storagePosition);
  delay(500);
}

boolean checkIncomingTextMessage() {
  // +CMGR: "REC READ","+491784049573","","12/10/10,17:49:25+08"
  // RESTART,0,0,81,33,20111225-000106,0.0,,,

  // start with +CMGR: , assume output if always '\r\n+CMGR: '
  if (incomingMessage[2] != '+' && incomingMessage[3] != 'C' && incomingMessage[4] != 'M' && incomingMessage[5] != 'G' && incomingMessage[6] != 'R' && incomingMessage[7] != ':' && incomingMessage[8] != ' ') return false;

  // 8 "'s before newline
  int charCounter=0;
  for (int i = 2; i < strlen(incomingMessage) - 1; i++) {
    if (incomingMessage[i] == '\r' || incomingMessage[i+1] == '\n') break;
    if (incomingMessage[i] == '"') charCounter++;
  }
  if (charCounter != 8) return false;

  // 4 ,'s before newline
  charCounter = 0;
  for (int i = 2; i < strlen(incomingMessage) - 1; i++) {
    if (incomingMessage[i] == '\r' || incomingMessage[i] == '\n') break;
    if (incomingMessage[i] == ',') charCounter++;
  }
  if (charCounter != 4) return false;

  // at least 2 newlines (one right at beginning and one between header and content of sms
  charCounter = 0;
  for (int i = 0; i < strlen(incomingMessage) - 1; i++) {
    if (incomingMessage[i] == '\r' && incomingMessage[i+1] == '\n') charCounter++;
  }
  if (charCounter < 2) return false;

  return true;
}

int payloadOfIncomingMessage() {
  for (int i = 2; i < strlen(incomingMessage) - 2; i++) {
 //   Serial.print(incomingMessage[i]);
    if (incomingMessage[i] == '\r' && incomingMessage[i+1] == '\n') {
      // text after 1st CRLF should be the content of the text message, the 'payload' for mars
   //   Serial.println("payload starts at " + i);
      return i+2;
    }
  }
  return -1;
}

