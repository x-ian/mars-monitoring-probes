/*
  Connect metal push button:
 LED +: A2 - Signal
 LED GND: A2 - GND
 C1: A0 - Signal
 N?1: A0 - GND
 */

#include <EEPROM.h>

#include <GSM_Shield.h>
#include <SoftwareSerial.h>

#include <Time.h>

GSM gsm;

// Gboard A0 starts at digital 14, A2 is 16, ...
int button = 14;
int led = 16;

// max number of messages per hour
int maxMessagesPerHour = 1;

// base configs and vars
char messageIdHeartbeat[] = "HEARTBEAT";
char messageIdRestart[] = "RESTART";
char messageIdPayload[] = "ALARM";

//char phone[] = "+491784049573";
char phone[] = "+265888288976";

// device configs
const char* customerId = "0";
const char* deviceId = "0";

// device counters
byte incomingMessageCount = 0;
byte outgoingMessageCount = 0;
byte restartCount = 0;

const int restartCountAdr = 0;
const int outgoingMessageCountAdr = 1;
const int incomingMessageCountAdr = 2;

// sms contingent
const int smsContingentPerDay = 1; // max. number of messages within 24 hours
int messageCounterPerDay = 0;
int uptimeInDays = 0;

// *******************************************************
// arduino methods

void setup() {      
  Serial.begin(9600);
  Serial.println("setup");
  if (!initGsm()) {
    // error during GSM setup, stop working and only blink
    while (true) {
      morseError();
      delay(5000);
    }
  }

  pinMode(led, OUTPUT);     
  pinMode(button, INPUT_PULLUP);

  //oneTimeEepromInit();
  eepromRead();

  restart();

  Serial.println("setup done"); 
}

void loop() {
  if (digitalRead(button) == HIGH) {
    delay(500);
    if (digitalRead(button) == HIGH) {
      delay(500);
      if (digitalRead(button) == HIGH) {
        // button pressed for more than 1 second
        digitalWrite(led, HIGH);
        delay(5000);
        // sending SMS
        if (sendNotification()) {
          // ok, wait 5 seconds on switch off LED
          delay(5000);
          digitalWrite(led, LOW);
        } 
        else {
          // error sending text message
          morseError();
          delay(2000);
          morseError();
          delay(2000);
          morseError();
          delay(2000);
          digitalWrite(led, HIGH);
        }
      }
    }
  }
  
  // check sms contingent
//  int daysRunning = (millis() / 1000 / 60); // minutes
  int daysRunning = (millis() / 1000 / 60 / 60 / 24); // days
  if (uptimeInDays < daysRunning) {
    // reset contingent
    uptimeInDays = daysRunning;
    messageCounterPerDay = 0;
  }
  delay(500);
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

boolean smsContingentDepleted() {
  if (messageCounterPerDay < smsContingentPerDay) {
    return false;
  } else {
    return true;
  }
}

boolean sendNotification() {
  if (smsContingentDepleted()) {
    // already send too many messages. don't do it again yet
    Serial.println("sms Contingent depleted");
    return false;
  } else {
    // still sms available
    
    char m[50];
    char counter[3];
    m[0] = '\0';
    strcat(m, "Button was pressed (");
    itoa(outgoingMessageCount, counter, 10);
    strcat(m, counter);
    strcat(m, ",");
    itoa(restartCount, counter, 10);
    strcat(m, counter);
    strcat(m, ")\0");
 
    messageCounterPerDay++;
    outgoingMessageCount++;
    EEPROM.write(outgoingMessageCountAdr, outgoingMessageCount);
    return gboard_sendTextMessage(phone, m);
  }
}

void restart() {
  char m[50];
  message(m, messageIdRestart);
//  gboard_sendTextMessage(phone, m);
  restartCount++;
  outgoingMessageCount++;
  EEPROM.write(restartCountAdr, restartCount);
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
  //  char value[6];
  //  ftoa(value, currentPayloadValue1, 2);
  //  strcat(m, value);
  strcat(m, ",");
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

void morseError() {
  dot(); 
  dot(); 
  dot();
  dash(); 
  dash(); 
  dash();
  dot(); 
  dot(); 
  dot();
}

int morsePeriod = 250;
void dot() {
  digitalWrite(led, LOW);
  delay(morsePeriod);
  digitalWrite(led, HIGH);
  delay(morsePeriod);
  digitalWrite(led, LOW);
  delay(morsePeriod);
}

void dash() {
  digitalWrite(led, LOW);
  delay(morsePeriod);
  digitalWrite(led, HIGH);
  delay(morsePeriod * 2);
  digitalWrite(led, LOW);
  delay(morsePeriod * 2);
}

boolean gboard_sendTextMessage(char* number, char* message) {
  Serial.print("Send SMS to ");
  Serial.println(number);
  Serial.println(message);
  int error = gsm.SendSMS(number, message);  
  if (error == 0) {
    Serial.println("SMS ERROR");
    return false;
  } 
  else {
    Serial.println("SMS OK");
    return true;
  }
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

char* ftoa(char *a, float f, int precision)
{
  // slightly wrong sometimes, e.g. 23.04 results in 23.4
  long p[] = {
    0,10,100,1000,10000,100000,1000000,10000000,100000000    };
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}


