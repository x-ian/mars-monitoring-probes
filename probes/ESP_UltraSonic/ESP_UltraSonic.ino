#include <EEPROM.h>
#include <SPI.h>
#include <TimeAlarms.h>
#include <Time.h>

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid     = "YAKOBO";
const char* password = "kirschsiefen";

// base configs and vars
const char* messageIdHeartbeat = "HEARTBEAT";
const char* messageIdRestart = "RESTART";
const char* messageIdAlarm = "ALARM";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

// probe values
float currentValue1 = 0;
float previousValue1 = 0;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

// *******************************************************
// CHANGE ME
// device configs
const char* customerId = "1";
const char* probeId = "50";
// device board layout
const int trigPin = 5;
const int echoPin = 2;

const float alarm1Threshold = 1;
const int heartbeatIntervall = 300;  // 14400 sec = 4 h

// *******************************************************
// arduino methods

void setup() {
  Serial.begin(115200);
  delay(5000);
  Serial.println(F("setup"));

  //oneTimeEepromInit();
  eepromRead();
  
  setTime(23,59,59,24,12,2016); // just to begin with something
  if (!esp_initWifi()) {
    Serial.println(F("Error during Wifi initialization"));
  }

  // wait 1 minute before sending out restart message
//  delay(60000);
  measure(); // make sure the restart message already has the values
  restart();

  Alarm.timerRepeat(heartbeatIntervall, heartbeat);

  // CHANGE ME: 
  // init board layout  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
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
  Alarm.delay(60000); // wait for 60 sec
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

void alarm() {
  char m[50];
  message(m, messageIdAlarm);
  esp_sendMessage(m);
  outgoingMessageCount++;
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void restart() {
  char m[50];
  message(m, messageIdRestart);
  esp_sendMessage(m);
  restartCount++;
  outgoingMessageCount++;
  EEPROM.write(restartCountAdr, restartCount);
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

void heartbeat() {
  char m[50];
  message(m, messageIdHeartbeat);
  esp_sendMessage(m);
  outgoingMessageCount++;
  EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
}

char* message(char* m, const char* messageId) {
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

// *******************************************************
// ESP32 specific code

boolean esp_initWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println("   (restart if it fails to connect)");

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.setAutoConnect(true);
  delay(100);
  WiFi.begin(ssid, password);
  delay(100);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");Serial.print(WiFi.status());
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

void esp_sendMessage(char *message) {
  Serial.print(F("esp_sendMessage: "));
  esp_httpPost("", 80, "", message);
}

void esp_httpPost(char *server_nix, int port_nix, char *url_nix, char* d) {
  const int httpPort = 80;
  const char* host = "www.marsmonitoring.com";

  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
  }

  HTTPClient http;
  http.begin("http://www.marsmonitoring.com/messages/create_from_probe"); 
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  String data = "{\"message\":{\"data\":\"";
  data += d;
  data += "\"}}";

  int httpCode = http.POST(data);
  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);
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

    // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  //Reads the echoPin, returns the sound wave travel time in microseconds
  int duration = pulseIn(echoPin, HIGH);

  // Convert the time into a distance
  float distance = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343
  Serial.print(distance);
  Serial.println("cm");

  previousValue1=currentValue1;
  currentValue1 = distance;
  ftoa(value, distance, 2);
  strcpy(v, value);
  return v;
}
