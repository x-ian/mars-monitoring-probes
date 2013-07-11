// WiFi shield, Groove temp & humidity sensor to A0

#include <DHT.h>

#include <EEPROM.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#include <Time.h>

#include <SimpleTimer.h>

#include <EEPROM.h>

#include <SPI.h>

#include <Dhcp.h>
#include <util.h>
#include <math.h>


DHT dht(A0, DHT11);

//SimpleTimer heartbeatTimer;

// base configs and vars
char messageIdHeartbeat[] = "HEARTBEAT";
char messageIdRestart[] = "RESTART";
char messageIdPayload[] = "PAYLOAD";

// mars server
//char marsServer[] = "192.168.1.4";
//int marsPort = 3000;
char marsServer[] = "www.marsmonitoring.com";
int marsPort = 80;
char marsUrl[] = "/messages/create_from_probe";
WiFiClient marsClient;

// NTP communication
/*
unsigned int ntpLocalPort = 8888;
IPAddress ntpTimeServer(192, 43, 244, 18); // time.nist.gov NTP server
const int NTP_PACKET_SIZE= 48;
byte ntpPacketBuffer[NTP_PACKET_SIZE];
EthernetUDP ntpClient;
const int timeZoneOffset = +2;
*/
// device configs
const char* customerId = "1";
const char* deviceId = "9";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

char ssid[] = "YAKOBO";     //  your network SSID (name) 
char pass[] = "kirschsiefen";    // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

// probe values
int currentPayloadValue1 = 0;
int previousPayloadValue1 = 0;
const int payloadValue1Threshold = 15;
int currentPayloadValue2 = 0;
int previousPayloadValue2 = 0;
const int payloadValue2Threshold = 16500;
int currentPayloadValue3 = 0;
int previousPayloadValue3 = 0;
const int payloadValue3Threshold = 16500;
int currentPayloadValue4 = 0;
int previousPayloadValue4 = 0;
const int payloadValue4Threshold = 16500;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

// *******************************************************
// arduino methods

void setup() {
  Serial.begin(19200);
//  while (!Serial) { ; } // wait for serial port to connect. Needed for Leonardo only
  Serial.println("setup");

  //oneTimeEepromInit();
  eepromRead();

  setTime(23,59,59,24,12,2011); // just to begin with something
//  wifi_setup();
//  setSyncProvider(ether_syncTime);
//  setSyncInterval(43200); // get new time every 12 hours = 43200 secs

  // disable SD interface for ethernet shield (as in http://arduino.cc/forum/index.php/topic,98607.0.html)
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  
  previousPayloadValue1 = 0;
  currentPayloadValue1 = 0;
  previousPayloadValue2 = 0;
  currentPayloadValue2 = 0;
  previousPayloadValue3 = 0;
  currentPayloadValue3 = 0;
  previousPayloadValue4 = 0;
  currentPayloadValue4 = 0;

  // wait 1 minute before sending out restart message
  delay(10000);
  measure();
  restart();
}

  // simple solution to replace simpletimer
  int clock = 0;
  int timeQuantum = 5000; //30000; // 30 sec
  int heartbeatIntervall = 30;
  
void loop() {
  Serial.print("loop ");

  measure();

  Serial.print(previousPayloadValue1);
  Serial.print(" -> ");
  Serial.print(currentPayloadValue1);
  Serial.print(" | ");
  Serial.print(previousPayloadValue2);
  Serial.print(" -> ");
  Serial.print(currentPayloadValue2);
  Serial.print(" | ");
  Serial.print(previousPayloadValue3);
  Serial.print(" -> ");
  Serial.print(currentPayloadValue3);
  Serial.print(" | "); 
  Serial.print(previousPayloadValue4);
  Serial.print(" -> ");
  Serial.println(currentPayloadValue4);

  if (((currentPayloadValue1 >= payloadValue1Threshold && previousPayloadValue1 < payloadValue1Threshold) 
    || (currentPayloadValue1 < payloadValue1Threshold && previousPayloadValue1 >= payloadValue1Threshold))
    || ((currentPayloadValue2 >= payloadValue2Threshold && previousPayloadValue2 < payloadValue2Threshold) 
    || (currentPayloadValue2 < payloadValue2Threshold && previousPayloadValue2 >= payloadValue2Threshold)))
  {
    //payload();
  }  

  delay(timeQuantum);
 
  clock++;
  if (clock == heartbeatIntervall) {
    Serial.println("time for another heartbeat");
    heartbeat();
    clock = 0;
  }
  delay(timeQuantum);
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
  // temp and humidity
  delay(500);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  previousPayloadValue1 = currentPayloadValue1;
  currentPayloadValue1 = (int) lround(t * 100);
  previousPayloadValue2 = currentPayloadValue2;
  currentPayloadValue2 = (int) lround(h * 100);

  // light
  delay(500);
  analogRead(A3);
  delay(500);
  int sensorValue = analogRead(A3); 
  float Rsensor=(float)(1023-sensorValue)*10/sensorValue; // there might be a way to get Lux from Rsensor, but I guess it is too unreliable
  previousPayloadValue3 = currentPayloadValue3;
  currentPayloadValue3 = (sensorValue * 10); // bring it into the range of temp and hum

  // wind
//  delay(500);
//  previousPayloadValue4 = currentPayloadValue4;
//  analogRead(A2);
//  delay(500);
//  currentPayloadValue4 = analogRead(A2) * 10;

  // sound
/*  delay(500);
  previousPayloadValue3 = currentPayloadValue3;
  analogRead(A2);
  delay(500);
  currentPayloadValue3 = analogRead(A2);
*/    
}

void payload() {
  char m[60];
  message(m, messageIdPayload);
  //  gprs_sendTextMessage(phone, m);
  ether_sendMessage(m);
}

void restart() {
  char m[60];
  message(m, messageIdRestart);
  //  gprs_sendTextMessage(phone, m);
  ether_sendMessage(m);
  restartCount++;
  EEPROM.write(restartCountAdr, restartCount);
}

void heartbeat() {
  char m[60];
  message(m, messageIdHeartbeat);
  //  gprs_sendTextMessage(phone, m);
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
  char value[6];
  itoa(currentPayloadValue1, value, 10);
  strcat(m, value);
  strcat(m, ",");
  itoa(currentPayloadValue2, value, 10);
  strcat(m, value);
  strcat(m, ",");
  itoa(currentPayloadValue3, value, 10);
  strcat(m, value);
  strcat(m, ",");
  itoa(currentPayloadValue4, value, 10);
  strcat(m, value);
  return m;
}

// *******************************************************
// ethernet shield specific code

void wifi_setup() {
  Serial.print("Attempting to connect to WPA network: ");
  Serial.println(ssid);
  status = WiFi.begin(ssid, pass);

  // if you're not connected, stop here:
  if ( status != WL_CONNECTED) { 
    Serial.println("Couldn't get a wifi connection");
    while(true);
  } 
  // if you are connected :
  else {
    Serial.println("You're connected to the network");

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print your MAC address:
    byte mac[6];  
    WiFi.macAddress(mac);
    Serial.print("MAC address: ");
    Serial.print(mac[5],HEX);
    Serial.print(":");
    Serial.print(mac[4],HEX);
    Serial.print(":");
    Serial.print(mac[3],HEX);
    Serial.print(":");
    Serial.print(mac[2],HEX);
    Serial.print(":");
    Serial.print(mac[1],HEX);
    Serial.print(":");
    Serial.println(mac[0],HEX);

    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print the MAC address of the router you're attached to:
    byte bssid[6];
    WiFi.BSSID(bssid);    
    Serial.print("BSSID: ");
    Serial.print(bssid[5],HEX);
    Serial.print(":");
    Serial.print(bssid[4],HEX);
    Serial.print(":");
    Serial.print(bssid[3],HEX);
    Serial.print(":");
    Serial.print(bssid[2],HEX);
    Serial.print(":");
    Serial.print(bssid[1],HEX);
    Serial.print(":");
    Serial.println(bssid[0],HEX);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(rssi);

    // print the encryption type:
    byte encryption = WiFi.encryptionType();
    Serial.print("Encryption Type:");
    Serial.println(encryption,HEX);
    Serial.println();
  }
}

boolean ether_sendMessage(char * message) {
  serialPrintlnFromFlash(PSTR("ether_sendMessage: "));
  Serial.println(marsServer);

  if(ether_httpPost_wifi(marsServer, marsPort, marsUrl, message)) {
    outgoingMessageCount++;
    EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
    return true;
  } else {
    return false;
  }
}

boolean ether_httpPost_wifi(char * server, int port, char * url, char * d) {
  // unlike with HttpClient .connected() doesn't work with WiFiClient
  boolean r = false;

  String data = "{\"message\":{\"data\":\"";
  data += d;
  data += "\"}}";
  Serial.println("ether_httpPost: " + data);
  if (marsClient.connect(server,port)) {
    serialPrintFromFlash(PSTR("ether_httpPost: trying to connect ... "));
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

    delay(5000);

    serialPrintlnFromFlash(PSTR("... connected"));
    char response[20];
    char c = ' ';
    int i = 0;
    while (marsClient.available()) {
      c = marsClient.read();
      if (i < 20) response[i] = c;
      i++;
    }
    response[19]='\0';
//    Serial.println(response);
    
    if (strcmp(response, "HTTP/1.1 201 Create") == 0) {
      // ok, assume post was successful
      r = true;
    } 
  } else {
    serialPrintlnFromFlash(PSTR("... ERROR CONNECTING"));
  }
  marsClient.stop();
  return r;
}

// *******************************************************
// time via NTP 
/*
unsigned long ether_syncTime() {
  Serial.println("ether_syncTime");
  ntpClient.begin(ntpLocalPort);
  sendNTPpacket(ntpTimeServer); // send an NTP packet to a time server
  delay(5000);
  if ( ntpClient.parsePacket() ) {  
    // We've received a packet, read the data from it
    ntpClient.read(ntpPacketBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(ntpPacketBuffer[40], ntpPacketBuffer[41]);
    unsigned long lowWord = word(ntpPacketBuffer[42], ntpPacketBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord; 
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years and add the time zone:
    unsigned long epoch = secsSince1900 - seventyYears + (timeZoneOffset * 3600L);
    return epoch;
  }
  return 0;
}

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  ntpPacketBuffer[0] = 0b11100011;   // LI, Version, Mode
  ntpPacketBuffer[1] = 0;     // Stratum, or type of clock
  ntpPacketBuffer[2] = 6;     // Polling Interval
  ntpPacketBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  ntpPacketBuffer[12]  = 49; 
  ntpPacketBuffer[13]  = 0x4E;
  ntpPacketBuffer[14]  = 49;
  ntpPacketBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  ntpClient.beginPacket(address, 123); //NTP requests are to port 123
  ntpClient.write(ntpPacketBuffer,NTP_PACKET_SIZE);
  ntpClient.endPacket(); 
}
*/
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
    0,10,100,1000,10000,100000,1000000,10000000,100000000          };
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

void serialPrintlnFromFlash (PGM_P s) {
        char c;
        while ((c = pgm_read_byte(s++)) != 0)
            Serial.print(c);
        Serial.println();
}

void serialPrintFromFlash (PGM_P s) {
        char c;
        while ((c = pgm_read_byte(s++)) != 0)
            Serial.print(c);
}



