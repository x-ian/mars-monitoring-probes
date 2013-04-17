/*

sample output:

ssetup
-------------------------
Reset default configuration
ATZ

OK
-------------------------
Display product identification information
ATI

SIM900 R11.0

OK
-------------------------
Request complete TA capabilities list
AT+GCAP

+GCAP:+FCLASS,+CGSM

OK
-------------------------
Request manufacturer identification
AT+GMI

SIMCOM_Ltd

OK
-------------------------
Request TA model identification
AT+GMM

SIMCOM_SIM900

OK
-------------------------
Request TA revision identification of software
AT+GMR

Revision:1137B11SIM900M64_ST

OK
-------------------------
Request global object id
AT+GOI

SIM900

OK
-------------------------
Request TA serial number id
AT+GSN

012896006348517

OK
-------------------------
Operator selection
AT+COPS?

+COPS: 0

OK
-------------------------
Signal quality report
AT+CSQ

+CSQ: 10,0

OK
-------------------------
Request internatin mobile subscriber id
AT+CIMI

262026046912239

OK
-------------------------
Request product serial number id
AT+CGSN

012896006348517

OK
-------------------------
Clock
AT+CCLK

ERROR
-------------------------
Battery charge
AT+CBC

+CBC: 0,91,4080

OK
-------------------------
SIM inserted status report
AT+CSMINS?

+CSMINS: 0,1

OK
-------------------------
Network registration
AT+CREG?

+CREG: 0,1

OK
-------------------------
Radio link protocol parameters
AT+CRLP?

+CRLP: 61,61,48,6,0,7

OK

*/

#include <SoftwareSerial.h>
SoftwareSerial GPRS(7, 8);

int pinGprsRx = 7;
int pinGrpsTx = 8;
int pinGprsPower = 9;

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("setup");

  gprs_setup();
  delay(60000);
  gprs_sendAtCommand("ATZ", "Reset default configuration");
  gprs_sendAtCommand("ATI", "Display product identification information");
  gprs_sendAtCommand("AT+GCAP", "Request complete TA capabilities list");
 // gprs_sendAtCommand("AT+GMI", "Request manufacturer identification");
 // gprs_sendAtCommand("AT+GMM", "Request TA model identification");
  gprs_sendAtCommand("AT+GMR", "Request TA revision identification of software");
  gprs_sendAtCommand("AT+GOI", "Request global object id");
  gprs_sendAtCommand("AT+GSN", "Request TA serial number id");
  gprs_sendAtCommand("AT+COPS?", "Operator selection");
  gprs_sendAtCommand("AT+CSQ", "Signal quality report");
  gprs_sendAtCommand("AT+CIMI", "Request international mobile subscriber id");
  gprs_sendAtCommand("AT+CGSN", "Request product serial number id");
  gprs_sendAtCommand("AT+CCLK?", "Clock");
  gprs_sendAtCommand("AT+CBC", "Battery charge");
  gprs_sendAtCommand("AT+CSMINS?", "SIM inserted status report");
  gprs_sendAtCommand("AT+CREG?", "Network registration");
  gprs_sendAtCommand("AT+CRLP?", "Radio link protocol parameters");
  gprs_sendAtCommand("AT+CMGL=\"ALL\"", "List all messages");
  gprs_sendAtCommand("AT+CMGR=0", "Read message #0");
  gprs_sendAtCommand("AT+CMGR=1", "Read message #1");
  gprs_sendAtCommand("AT+CMGR=2", "Read message #2");
  Serial.println("done");
}

void loop()
{
  // intentionally empty
}

void gprs_setup() {
  gprs_powerUpOrDown();
  delay(500);
  GPRS.begin(19200); // the default GPRS baud rate   
  delay(500);
  //gprs_setTime();
  delay(500);
}

void gprs_sendAtCommand(String command, String desc) {
  Serial.println("-------------------------");
  Serial.println(desc);
  GPRS.println(command);
  delay(100);
  while(GPRS.available()) {
    Serial.print((char) GPRS.read());
  }
  delay(100);
  while(GPRS.available()) {
    Serial.print((char) GPRS.read());
  }
  delay(5000);
  Serial.println("-------------------------");
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

