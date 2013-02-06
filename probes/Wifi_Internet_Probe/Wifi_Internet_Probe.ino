#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#include <Time.h>

#include <SimpleTimer.h>

#include <EEPROM.h>

#include <SPI.h>

#include <Dhcp.h>
//#include <Dns.h>
//#include <Ethernet.h>
//#include <EthernetClient.h>
//#include <EthernetServer.h>
//#include <EthernetUdp.h>
#include <util.h>
#include <ICMPPing.h>

//#include <SoftwareSerial.h>

SimpleTimer heartbeatTimer;

// base configs and vars
char messageIdHeartbeat[] = "HEARTBEAT";
char messageIdRestart[] = "RESTART";
char messageIdPayload[] = "PAYLOAD";

// mars phones
//const char phone[] = "+4915259723556";
//const char phone[] = "+265884781634";
//String phone = "+491784049573";

// mars server
//char marsServer[] = "192.168.1.4";
//int marsPort = 3000;
char marsServer[] = "www.marsmonitoring.com";
int marsPort = 80;
char marsUrl[] = "/messages/create_from_probe";
WiFiClient marsClient;

// ICMP ping communication
byte icmpServer[] = { 
  8,8,4,4};
SOCKET icmpSocket = 2; // critical setting, only 4 avail, unknown which is used by which lib

// HTTP communication
char httpServer[] = "www.google.com";
const unsigned int httpTimeout = 20000;
WiFiClient httpClient;

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

// device board layout
//int pinAnalogTemp = 0;
//int pinLed = 3;
//int pinButton = 5;
//int pinGprsRx = 7;
//int pinGrpsTx = 8;
//int pinGprsPower = 9;
//SoftwareSerial gprs(7, 8);

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
  wifi_setup();
//  setSyncProvider(ether_syncTime);
//  setSyncInterval(43200); // get new time every 12 hours = 43200 secs

  // disable SD interface for ethernet shield (as in http://arduino.cc/forum/index.php/topic,98607.0.html)
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  previousPayloadValue1 = 0;
  currentPayloadValue1 = 0;
  previousPayloadValue2 = 0;
  currentPayloadValue2 = 0;

  // wait 1 minute before sending out restart message
  delay(10000);
  restart();

  heartbeatTimer.setInterval(60000, heartbeat);

  // init board layout  
  //pinMode(pinButton, INPUT);
  //pinMode(pinLed, OUTPUT);
}

void loop() {
  Serial.print("loop ");
  delay(10000);
  measure();

  Serial.print(previousPayloadValue1);
  Serial.print(" -> ");
  Serial.print(currentPayloadValue1);
  Serial.print(" | ");
  Serial.print(previousPayloadValue2);
  Serial.print(" -> ");
  Serial.println(currentPayloadValue2);

  if (((currentPayloadValue1 >= payloadValue1Threshold && previousPayloadValue1 < payloadValue1Threshold) 
    || (currentPayloadValue1 < payloadValue1Threshold && previousPayloadValue1 >= payloadValue1Threshold))
    || ((currentPayloadValue2 >= payloadValue2Threshold && previousPayloadValue2 < payloadValue2Threshold) 
    || (currentPayloadValue2 < payloadValue2Threshold && previousPayloadValue2 >= payloadValue2Threshold)))
  {
    //payload();
  }  

  heartbeatTimer.run();
//  Ethernet.maintain(); // refresh DHCP IP if necessary
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
//  payloadValue1();
  payloadValue2();
}

void payload() {
  char m[50];
  message(m, messageIdPayload);
  //  gprs_sendTextMessage(phone, m);
  ether_sendMessage(m);
  outgoingMessageCount++;
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void restart() {
  char m[50];
  message(m, messageIdRestart);
  //  gprs_sendTextMessage(phone, m);
  ether_sendMessage(m);
  restartCount++;
  outgoingMessageCount++;
  EEPROM.write(restartCountAdr, restartCount);
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void heartbeat() {
  char m[50];
  message(m, messageIdHeartbeat);
  //  gprs_sendTextMessage(phone, m);
  ether_sendMessage(m);
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
  itoa(currentPayloadValue1, value, 10);
  strcat(m, value);
  strcat(m, ",");
  itoa(currentPayloadValue2, value, 10);
  strcat(m, value);
  strcat(m, ",");
  strcat(m, ",");
  return m;
}

int payloadValue1() {
  previousPayloadValue1 = currentPayloadValue1;
  currentPayloadValue1 = ether_icmpPing(icmpServer, icmpSocket);
  return currentPayloadValue1;
}

int payloadValue2() {
  previousPayloadValue2 = currentPayloadValue2;
  currentPayloadValue2 = ether_httpPing(httpServer, 80, "GET /search?q=arduino HTTP/1.0");
  return currentPayloadValue2;
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
    Serial.print("You're connected to the network");

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
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

void ether_sendMessage(char *message) {
  Serial.print("ether_sendMessage: ");
  Serial.print(marsServer);
  Serial.println(message);

  //char number[message.length];
  // strcpy(time, formatNumber(number, year()));

  //char data[] ="{\"message\":{\"data\":\"" + "\"}}";

  ether_httpPost(marsServer, marsPort, marsUrl, message);
}

int ether_icmpPing(byte *server, SOCKET socket) {
  ICMPPing ping(icmpSocket);
  char buffer [256];
  ping(4, server, buffer);

  // parse response, e.g. Reply[1] from: 8.8.4.4: bytes=32 time=12ms TTL=128
  char delims[]=" ";
  char *token = NULL;
  token = strtok(buffer, delims);
  String t = token;
  if (t.startsWith("Request")) {
    return -1;
  } 
  else {
    while(token != NULL){
      String t = token;
      if (t.startsWith("time=")) {
        t = t.substring(5, t.indexOf("ms"));
        t.toCharArray(token, t.length() + 1);
        int ms = atoi(token);
        return ms;
      }
      token = strtok(NULL, delims); 
    }
  }
}

long ether_httpPing(char *server, int port, char *url) {
  //Serial.println("ether_httpPing");
  long time = millis();
  if (httpClient.connect(server, port)) {
    httpClient.println(url);
    // ether.println("GET /search?q=arduino HTTP/1.0");
    httpClient.println();
  } 
  else {
    return -1;
  }
  long timeout = millis() + httpTimeout; // stop after this period
  while (httpClient.connected() && (millis() < timeout)) {
    if (httpClient.available()) {
      char c = httpClient.read();
      //      Serial.print(c);
    }
  }
  httpClient.stop();
  return millis() - time;
}

void ether_httpPost(char *server, int port, char *url, char* d) {
  String data = "{\"message\":{\"data\":\"";
  data += d;
  data += "\"}}";
  //  Serial.println("connecting...");
  if (marsClient.connect(server,port)) {
    //    Serial.println("connected");
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
    //   Serial.println("disconnecting.");
    marsClient.stop();
  }

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





