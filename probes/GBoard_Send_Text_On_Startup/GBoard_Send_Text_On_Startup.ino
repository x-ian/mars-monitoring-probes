/*
gboard programming (http://imall.iteadstudio.com/im120411004.html)

FTDI connection through seeeduino stalker board
GND
DTR
TX - > Rx
RX -> Tx

download gsm_shield lib (https://github.com/jgarland79/GSM_Shield)
remove newsoftserial from lib folder
include '#include <SoftwareSerial.h>' in GSM_Shield.cpp

maybe get a serial programmer link (http://imall.iteadstudio.com/im120525005.html)

board - arduino pro or pro mini atmega 328, 5V, 16 MHz
programmer adruino as ISP

make sure jumpers are in ST-D2, SR-D3 ('Software UART   to SIM900,  Hardware UART   to Specific')
*/
#include <GSM_Shield.h>
#include <SoftwareSerial.h>

GSM gsm;

void setup() {
  Serial.begin(9600);
  Serial.println("system startup"); 
  gsm.TurnOn(9600);              //module power on
  gsm.InitParam(PARAM_SET_1);//configure the module  
  gsm.Echo(1);               //enable AT echo 
  Serial.println("system startup done"); 
  delay(10000);
  gboard_sendTextMessage("+491784049573", "Test message during startup");
//  gboard_sendTextMessage("+265884767251", "Test message during startup");
}

void loop()
{  
  delay(60000);
//  gboard_sendTextMessage("+491784049573", "Test message in loop");
//  gboard_sendTextMessage("+265884767251", "Test message in loop");
}

void gboard_sendTextMessage(char* number, char* message) {
  Serial.print("Send SMS to ");
  Serial.println(number);
  int error=gsm.SendSMS(number, message);  
  if (error==0) {
    Serial.println("SMS ERROR \n");
  } else {
    Serial.println("SMS OK \n");             
  }
}  
