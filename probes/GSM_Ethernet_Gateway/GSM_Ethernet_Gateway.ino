#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <SPI.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#include <SimpleTimer.h>
#include <SoftwareSerial.h>

int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;
SoftwareSerial gprs(7, 8);

SimpleTimer checkForNewSmsTimer;

// needs to match value of _SS_MAX_RX_BUFF in SoftwareSerial.h 
// default 64 is not enough, so increase it!
// (found in somewhere like /Applications/Arduino.app/Contents/Resources/Java/libraries/SoftwareSerial)
#define INCOMING_BUFFER_SIZE 128

char incomingNumber[20];
char incomingMessage[INCOMING_BUFFER_SIZE];
char incomingTimestamp[20];

// ethernet shield
// byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x86, 0x4C };
// arduino ethernet
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x6F, 0x35 };
byte ip[] = { 
  192, 168, 1, 10 };
EthernetServer server(80);
EthernetClient client ;

// mars server
//char marsServer[] = "192.168.1.4";
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

  gprs_setup();
  ether_setup();
  delay(1000);

  checkForNewSmsTimer.setInterval(60000, checkForNewSms); // check every 60 secs for new incoming SMS
}

void loop() {
  // might be that web client request and ocal web server doesn't like each other too much...
  // maybe some additional info?
  // - http://stackoverflow.com/questions/7432309/arduino-uno-ethernet-client-connection-fails-after-many-client-prints
  // - http://forum.freetronics.com/viewtopic.php?f=4&t=176
//  handleHttpRequests();
  checkForNewSmsTimer.run();
}

// *******************************************************
// Process incoming HTTP requests

void handleHttpRequests() {
  boolean smsSent = false;
  char line[INCOMING_BUFFER_SIZE];

  client = server.available();
  if (client.connected() && client.available()) {
    // assume 1st line is http method
    nextHttpLine(line);
    char * method = strtok(line, " \r\n");
    if (strcmp(method, "POST") == 0) {
      char * uri = strtok(NULL, " \r\n");
      if (strcmp(uri, "/sendsms") == 0) {
        // send out sms message
        while (client.connected() && client.available()) {
          nextHttpLine(line);
          // check for a line "number=+123456789&message=hallo"
          if (strstr(line, "number") != NULL && strstr(line, "message") != NULL) {
            // most likely you want some form of URL decoding as in http://rosettacode.org/wiki/URL_decoding#C
            char * param1 = strtok(line, "&\r\n\0");
            char * param2 = strtok(NULL, "&\r\n\0");
            strtok(param1, "=");
            char * number = strtok(NULL, "=\r\n\0");
            strtok(param2, "=");
            char * message = strtok(NULL, "=\r\n\0");
            gprs_sendTextMessage(number, message);
            smsSent = true;
          }          
        }
      }
    }
    if(smsSent) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println();
      client.println("<h2>OK - SMS sent.</h2>");
    } 
    else {
      client.println("HTTP/1.1 500 Internal Server Error");
      client.println("Content-Type: text/html");
      client.println();
      client.println("<h2>Something went wrong somewhen and somewhere.</h2>");
    }
  }

  // give the web browser time to receive the data
  delay(100);
  client.stop();
}

void nextHttpLine(char* line) {
  int i = 0;  
  while (client.connected()) {
    if (client.available())  {
      line[i] = client.read();
      if (i == INCOMING_BUFFER_SIZE - 2) {
        line[i+1] = '\0';
        break;
      } 
      else if (i > 0 && line[i-1] == '\r' && line[i] == '\n') {
        // stop here with null terminated string
        // simplification as HTTP spec requires always <CR><LF>
        line[i-1] = '\0';
        break;
      }
    } 
    else {
      line[i] = '\0';
      break;
    }
    i++;
  } 
}

// *******************************************************
// Process incoming text messages

void checkForNewSms() {
  char incomingStoragePosition = gprs_nextAvailableTextIndex();
  Serial.print("found incoming text message at position ");
  Serial.println(incomingStoragePosition);
  gprs_readTextMessage(incomingStoragePosition);
  // check incoming message
  if (checkIncomingTextMessage()) {
    Serial.println("incoming text message ok");
    int i = payloadOfIncomingMessage();
    if (i > -1) {
      Serial.println("incoming text message contains payload");
      // forward incoming message
      if (ether_sendMessage(&incomingMessage[i])) {
        Serial.println("incoming text message forwarded via HTTP");
        // gprs delete old message
        gprs_deleteTextMessage(incomingStoragePosition);
      }
    }
  } else {
    // something seems wrong with the SMS, delete it for now
    // maybe have another dedicated notification channel for such cases?
    gprs_deleteTextMessage(incomingStoragePosition);
  }
}

// *******************************************************
// GSM/GPRS shield specific code

char gprs_nextAvailableTextIndex() {
  // make sure nothing unexpected is coming in
  delay(500);
  while (gprs.available() > 0) {
    gprs.read();
  }

  gprs.println("AT+CMGL=\"ALL\"");
  delay(500);
  char c = ' ';
  for (int i = 0; i < 10; i++) {
    c = gprs.read();
    // assume output of CMGL is always '  +CMGL: <one digit number at position 9>'
    // TODO what if number is bigger than 9?
    if (i == 9) return c;
  }
  return '?';
}

void gprs_readTextMessage(char storagePosition) {
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

void gprs_deleteTextMessage(char storagePosition) {
  Serial.print("deleteTextMessage: ");
  Serial.println(storagePosition);

  delay(500);
  gprs.print("AT+CMGD=");
  gprs.println(storagePosition);
  delay(500);
}

void gprs_setup() {
  gprs_powerUpOrDown();
  delay(500);
  gprs.begin(19200); // the default GPRS baud rate   
  delay(500);
  //gprs_setTime();
  delay(500);
  gprs.println("ATE0"); // SMS in text mode
  delay(100);
  gprs.println("AT+CMGF=1"); // SMS in text mode
  delay(100);
}

void gprs_sendTextMessage(String number, char * message) {
  Serial.print("sendTextMessage: ");
  Serial.print(number);
  Serial.println(message);
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
    Serial.print(incomingMessage[i]);
    if (incomingMessage[i] == '\r' && incomingMessage[i+1] == '\n') {
      // text after 1st CRLF should be the content of the text message, the 'payload' for mars
      Serial.println("payload starts at " + i);
      return i+2;
    }
  }
  return -1;
}

// *******************************************************
// ethernet shield specific code

void ether_setup() {
  Ethernet.begin(mac, ip);
//  server.begin();
}

boolean ether_sendMessage(char * message) {
  Serial.print("ether_sendMessage: ");
  Serial.print(marsServer);
  Serial.println(message);

  return ether_httpPost(marsServer, marsPort, marsUrl, message);
}

boolean ether_httpPost(char * server, int port, char * url, char * d) {
  boolean r = false;

  String data = "{\"message\":{\"data\":\"";
  data += d;
  data += "\"}}";
  if (marsClient.connect(server,port)) {
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
  }
  return r;
}

