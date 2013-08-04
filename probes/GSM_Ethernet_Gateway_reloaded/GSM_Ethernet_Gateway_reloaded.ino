#include <EEPROM.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <SPI.h>
#include <util.h>
#include <SoftwareSerial.h>
#include <Time.h>

// base configs and vars
char messageIdHeartbeat[] = "HEARTBEAT";
char messageIdRestart[] = "RESTART";
char messageIdPayload[] = "ALARM";

// device configs
const char* customerId = "1";
//12 for Germany, 13 for Malawi
const char* deviceId = "13";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;
SoftwareSerial gprs(7, 8);

// needs to match value of _SS_MAX_RX_BUFF in SoftwareSerial.h 
// default 64 is not enough, so increase it!
// (found in somewhere like /Applications/Arduino.app/Contents/Resources/Java/libraries/SoftwareSerial)
#define INCOMING_BUFFER_SIZE 128

char incomingNumber[20];
char incomingMessage[INCOMING_BUFFER_SIZE];
char incomingTimestamp[20];

// arduino ethernet
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x6F, 0x35 };
// germany
//byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xBB, 0xE1 }; 
//byte ip[] = { 
// 172.16.1.88 for neno, 192.168.1.10 for mainz
//  172, 16, 1, 88 };
//  195, 200, 93, 246 };
//  192, 168, 1, 113 };
IPAddress ip(172,16,1,88);
IPAddress myDns(8,8,4,4); // public google DNS

// mars server
//char marsServer[] = "192.168.21.199";
//int marsPort = 3000;
char marsServer[] = "www.marsmonitoring.com";
int marsPort = 80;
char marsUrl[] = "/messages/create_from_probe";
EthernetClient marsClient;

void setup()
{
  if (false) Serial.println("ja");
  
  Serial.begin(19200); // the Serial port of Arduino baud rate.
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for Leonardo only
//  }
  Serial.println(F("setup"));
//oneTimeEepromInit();
  eepromRead();

  gprs_setup();
  ether_setup();
  delay(1000);

  restart();
}

  // simple solution to replace simpletimer
  int clock = 0;
  int timeQuantum = 30000; // 30 sec
  int heartbeatIntervall = 1440;
  
void loop() {
//  delay(5000);
//  ether_setup();
//  delay(5000);

  checkForNewSms();
  delay(timeQuantum);
  clock++;
  if (clock == heartbeatIntervall) {
    heartbeat();
    clock = 0;
  }
  delay(timeQuantum);
/*
  delay(1000);
  checkForNewSmsTimer.run();
  delay(1000);
  heartbeatTimer.run();
  delay(1000);
*/
}

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

void gprs_setup() {
  Serial.println(F("gprs_setup"));
  gprs.begin(19200); // the default GPRS baud rate   
  delay(500);
  if (gprs_alreadyOn()) {
    // switch off and on again
    gprs_powerUpOrDown();
    delay(1000);
    gprs_powerUpOrDown();
  } else {
    gprs_powerUpOrDown();
  }
  delay(5000);
  //gprs_setTime();
  delay(500);
  gprs.println("ATZ"); // go to defaults
  delay(500);
  gprs.println("ATE0"); // turn off echo mode
  delay(500);
  gprs.println("AT+CMGF=1"); // SMS in text mode
  delay(500);
}

boolean gprs_alreadyOn() {
  boolean on = false;
  gprs.println("ATE0");
  delay(500);
  while (gprs.available() > 0) { gprs.read(); }
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
  Serial.print(F("ether_setup: (might block) ... "));
  Ethernet.begin(mac, ip, dns);
  Serial.println(F(" completed"));
}

void heartbeat() {
  Serial.println(F("heartbeat"));
  char m[50];
  message(m, messageIdHeartbeat);
  ether_sendMessage(m);
  Serial.println();
}

void restart() {
  restartCount++;
  EEPROM.write(restartCountAdr, restartCount);
  char m[50];
  message(m, messageIdRestart);
  ether_sendMessage(m);
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
  itoa(incomingMessageCount, counter, 10);
  strcat(m, counter);
  strcat(m, ",");
  itoa(gprs_nextAvailableTextIndex(), counter, 10);
  strcat(m, counter);
  strcat(m, ",");
  strcat(m, ",");
  return m;
}

boolean ether_sendMessage(char * message) {
  Serial.print(F("ether_sendMessage: "));
  Serial.println(marsServer);

  if(ether_httpPost(marsServer, marsPort, marsUrl, message)) {
    outgoingMessageCount++;
    EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
    return true;
  } else {
    return false;
  }
}

boolean ether_httpPost(char * server, int port, char * url, char * d) {
  
  // clear unprocessed incoming stuff
  if (marsClient.available()) {
    char c = client.read();
    Serial.print(c);
  }
  
  boolean r = false;
  char counter[3];
  itoa(gprs_nextAvailableTextIndex(), counter, 10);
  String c = String(counter);
  char time[16];
  String t =  String(currentTime(time));

  String data = "{\"message\":{\"data\":\"";

/*
  data += " - ";
  data += t;
  data += " - ";
  data += c;
  data += " - ";
  */
  
  data += d;
  data += "\"}}";
  data.replace("\n", " ");
  data.replace("\r", " ");
  Serial.println("ether_httpPost: " + data);
  Serial.print(F("ether_httpPost: connecting ... "));
  marsClient.connect(server,port);
    if (!marsClient.connected()) {
      Serial.print(F("connecting ... "));
      marsClient.connect(server,port); 
        if (!marsClient.connected()) {
          Serial.print(F("connecting ... "));
          marsClient.connect(server,port); 
        }
     
    }
  if (marsClient.connected()) {
    Serial.println(F("posting HTTP"));
    marsClient.print("POST ");
  delay(10);
    marsClient.print(url);
  delay(10);
    marsClient.println(" HTTP/1.1");
  delay(10);
    marsClient.print("Host: ");
  delay(10);
    marsClient.println(server);
  delay(10);
    marsClient.println("Content-Type: application/json");
  delay(10);
    marsClient.println("Accept: application/json");
  delay(10);
    marsClient.println("Connection: close");
  delay(10);
    marsClient.print("Content-Length: ");
  delay(10);
    marsClient.println(data.length());
  delay(10);
    marsClient.println();
  delay(10);
    marsClient.print(data);
  delay(10);
    marsClient.println();
    Serial.println(marsClient.connected());
  }
  delay(1000);

  if (marsClient.connected()) {
    Serial.println(F("... connected"));
    char response[20];
    Serial.println(marsClient.connected());
    Serial.println(marsClient.available());
      delay(500);
    Serial.println(marsClient.available());
      delay(500);
    Serial.println(marsClient.available());
      delay(500);
    Serial.println(marsClient.available());
      delay(500);
    Serial.println(marsClient.available());
      delay(500);
    for (int i = 0; marsClient.connected() && marsClient.available() > 0 && i < 20; i++) {
      response[i] = (char) marsClient.read();
    }
    response[19]='\0';
    while (marsClient.available() > 0) { 
      // make sure we read the whole remaining incoming thing
      marsClient.read();
    }
    Serial.print(F("Server response: "));
    Serial.println(response);
    
    if (strcmp(response, "HTTP/1.1 201 Create") == 0) {
      // ok, assume post was successful
      r = true;
      Serial.println(F("Posting to server successful"));
    } else {
      Serial.println(F("Posting to server NOT successful"));
    }
  } else {
    Serial.println(F("... ERROR CONNECTING"));
  }
    marsClient.flush();
    marsClient.stop();
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
  Serial.println(F("checkForNewSms"));
  
  int incomingStoragePosition = gprs_nextAvailableTextIndex();
  if (incomingStoragePosition > 0) {
    gprs_readTextMessage(incomingStoragePosition);
    incomingMessageCount++;

    // check incoming message
    if (checkIncomingTextMessage()) {
      int i = payloadOfIncomingMessage();
      if (i > -1) {
        Serial.print(F("message with content ..."));
        // forward incoming message
        if (ether_sendMessage(&incomingMessage[i])) {
          Serial.println(F("message forwarded"));
          gprs_deleteTextMessage(incomingStoragePosition);
          EEPROM.write(incomingMessageCountAdr, incomingMessageCount);
        } else {
          Serial.println(F("marsmonitoring.com unavailable, keeping the message"));
        }
      } else {
        Serial.println(F("message without any content"));
        gprs_deleteTextMessage(incomingStoragePosition);
      }
    } else {
      // something seems wrong with the SMS, delete it for now
      // maybe have another dedicated notification channel for such cases?
      Serial.println(F("Unknown response from board or provider"));
      gprs_deleteTextMessage(incomingStoragePosition);
    }
  }
  Serial.println(F("checkForNewSms done"));
  Serial.println();
}

// *******************************************************
// GSM/GPRS shield specific code

int gprs_nextAvailableTextIndex() {
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
 //   Serial.print(c);
    // assume output of CMGL is always '  +CMGL: <one digit number at position 9>'
    // TODO what if number is bigger than 9? should be ok as we always take the first one
    if (i == 9) index = (int) c;
    i++;
  }
  index = index - 48; //poor man converts ASCII to int like this...
  if (index == -49) index = 0; // no msg available means 49 means 0
  Serial.print(F("gprs_nextAvailableTextIndex: "));
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
  Serial.print(F("deleteTextMessage: "));
  Serial.println(storagePosition);

  delay(500);
  gprs.print("AT+CMGD=");
  gprs.println(storagePosition);
  delay(500);
}

boolean checkIncomingTextMessage() {
  // +CMGR: "REC READ","+491784049573","","12/10/10,17:49:25+08"
  // RESTART,0,0,81,33,20111225-000106,0.0,,,

  // start with +CMGR: , assume output if always '\r\n+CMGR: '
  if (incomingMessage[2] != '+' && incomingMessage[3] != 'C' && incomingMessage[4] != 'M' && incomingMessage[5] != 'G' && incomingMessage[6] != 'R' && incomingMessage[7] != ':' && incomingMessage[8] != ' ') {
    return false;
  }

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
  if (charCounter != 4) {
    return false;
  }

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

void softReset(){
  asm volatile ("  jmp 0");
}
