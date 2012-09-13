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
const unsigned int messageIdHeartbeat = 1;
const unsigned int messageIdRestart = 2;
const unsigned int messageIdPayload = 3;
const String phone = "+4915259723556";
const String marsServer = "";
byte icmpServer[] = { 
  8,8,4,4};
SOCKET pingSocket = 0;

//byte httpServer[] = { 173, 194, 35, 142 }; // Google
char httpServer[] = "www.google.com";
EthernetClient ether;
const unsigned long httpTimeout = 20000;

unsigned int localPort = 8888;      // local port to listen for UDP packets
IPAddress timeServer(192, 43, 244, 18); // time.nist.gov NTP server
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
EthernetUDP Udp; // A UDP instance to let us send and receive packets over UDP
const  int timeZoneOffset = +2;

// device configs
unsigned int customerId = 0;
unsigned int deviceId = 0;
//String phone = "+265884781634";
//const String message = "Hi Steve, this is a friendly test message from your Arduino board in Germany.";

// device counters
unsigned int incomingMessageCount = 0;
unsigned int outgoingMessageCount = 0;
unsigned int restartCount = 0;

// device board layout
int pinLed = 3;
int pinButton = 5;
int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;
SoftwareSerial gprs(7, 8);
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0D, 0x86, 0x4C };

// *******************************************************
// arduino methods

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("setup");

  //  setTime(23,59,59,24,12,2011); // just to begin with something

  //  gprs_powerUpOrDown();
  //  delay(500);
  //  gprs_setup();
  //  delay(500);
  //  grps_setTime();
  //  delay(500);

  ether_setup();
  setSyncProvider(ether_syncTime);

  //  printTime();

  restartCount++;
  sendRestart();
  //  Alarm.timerRepeat(60, sendHeartbeat);

  // init board layout  
  pinMode(pinButton, INPUT);
  pinMode(pinLed, OUTPUT);
}

void loop() {
  //  if (digitalRead(pinButton)==HIGH) {
  //    digitalWrite(pinLed, HIGH);
  //gprs_sendTextMessage(phone, message);
  Alarm.delay(5000);
  int ms = ether_icmpPing();
  Serial.print("ping took (ms): ");
  Serial.println(ms);

  Alarm.delay(5000);
  ms = ether_httpPing();
  Serial.print("http took (ms): ");
  Serial.println(ms);

  //  Alarm.delay(5000);
  //  sendMessage();
  //    digitalWrite(pinLed, LOW);
  //  }
  Ethernet.maintain(); // refresh DHCP IP if necessary
}

// *******************************************************
// i.a.m. logic

void sendMessage() {
  Serial.println("sendMessage");
  ether_sendMessage();
  outgoingMessageCount++;
}

void sendRestart() {
  Serial.print("sendRestart: ");
  Serial.println(restartMessage());
  outgoingMessageCount++;
}

void sendHeartbeat() {
  Serial.print("sendHeartbeat: ");
  Serial.println(messageHeartbeat());
  outgoingMessageCount++;
}

String messageHeartbeat() {
  String m = String (messageIdHeartbeat);
  m += ",";
  m += String(customerId);
  m += ",";
  m += String(deviceId);
  m += ",";
  m += String(outgoingMessageCount);
  m += ",";
  m += String(restartCount);
  m += ",";
  m += currentTime();
  return m;
}

String restartMessage() {
  String m = String(messageIdRestart);
  m += ",";
  m += String(customerId);
  m += ",";
  m += String(deviceId);
  m += ","; 
  m += String(outgoingMessageCount);
  m += ",";
  m += String(restartCount);
  m += ",";
  m += currentTime();
  return m;
}

// *******************************************************
// GSM/GPRS shield specific code

void gprs_sendTextMessage(String number, String message) {
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

void gprs_setup() {
  gprs.begin(19200); // the default GPRS baud rate   
}

// *******************************************************
// ethernet shield specific code

void ether_setup() {
  Serial.println("ether_setup");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    for(;;)
      ; // todo, useful defaulting?
  }
  Serial.println(Ethernet.localIP());
}

void ether_sendMessage() {
  Serial.println("ether_sendMessage");
  long t = millis();
  //  ether_icmpPing();
  ether_httpGet();
  int secs = (int) ((millis() - t) / 1000l);
  Serial.print("ether_sendMessage: took (s) ");
  Serial.println(secs);
}

int ether_icmpPing() {
  Serial.println("ether_icmpPing");
  ICMPPing ping(pingSocket);
  char buffer [256];
  ping(4, icmpServer, buffer);

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

long ether_httpPing() {
  Serial.println("ether_httpPing");
  long time = millis();
  if (ether.connect(httpServer, 80)) {
    ether.println("GET /search?q=arduino HTTP/1.0");
    //    ether.println("GET / HTTP/1.0");
    ether.println();
  } 
  else {
    return -1;
  }
  long timeout = millis() + httpTimeout; // stop after this period
  while (ether.connected() && (millis() < timeout)) {
    if (ether.available()) {
      char c = ether.read();
      //      Serial.print(c);
    }
  }
  ether.stop();
  return millis() - time;
}


void ether_httpGet() {
  Serial.println("ether_httpGet");
  if (ether.connect(httpServer, 80)) {
    Serial.println("connected");
    ether.println("GET /search?q=arduino HTTP/1.0");
    //    ether.println("GET / HTTP/1.0");
    ether.println();
  } 
  else {
    Serial.println("connection failed");
  }
  long timeout = millis() + httpTimeout; // stop after this period
  while (ether.connected() && (millis() < timeout)) {
    if (ether.available()) {
      char c = ether.read();
      //      Serial.print(c);
    }
  }
  ether.stop();
  Serial.println("disconnecting.");
}

// *******************************************************
// time via NTP 
unsigned long ether_syncTime() {
  Serial.println("ether_syncTime");

  Udp.begin(localPort);
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  delay(5000);
  if ( Udp.parsePacket() ) {  
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
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
  Serial.println("sendNTPpacket");
  memset(packetBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}

// *****************************************************************
// Helpers

String currentTime() {
  String t = String("");
  t += formatDigits(year());
  t += formatDigits(month());
  t += formatDigits(day());
  t += "-";
  t += formatDigits(hour());
  t += formatDigits(minute());
  t += formatDigits(second());
  return t;
}

void printTime(){
  Serial.println("printTime");
  Serial.println(currentTime());
  //  Serial.print(year());
  //  Serial.print(formatDigits(month()));
  //  Serial.print(formatDigits(day()));
  //  Serial.print("-");
  //  Serial.print(formatDigits(hour()));
  //  Serial.print(formatDigits(minute()));
  //  Serial.println(formatDigits(second()));
}

String formatDigits(int digits){
  if(digits < 10)
    return String("0" + String(digits));
  return String(digits);
}




