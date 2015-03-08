// Send AT commands to the GPRS modem from a terminal screen 

// shamelessly copied from http://www.seeedstudio.com/wiki/GPRS_Shield
// as of now it requires an Arduino board (at least I had issues with an Leonardo)
// make sure the jumper of the GRPS shield are in SoftSerial mode
// use a serial terminal program to enter commands, like gnu-screen
// e.g. on MacOS via screen /dev/tty.usbmodem411 19200 (close connection with "control-a control-\")
// to send a text message use these commands
// AT+IPR=19200
// AT+CMGF=1
// AT+CMGS="+4917840495723"
// <type in text>, finish with a control-z (hex 1A)
// use power on/off button of GRPS shield in case it doesn't come up automatically

//Serial Relay - Arduino will patch a 
//serial link between the computer and the GPRS Shield
//at 19200 bps 8-N-1
//Computer is connected to Hardware UART
//GPRS Shield is connected to the Software UART 

// check http://www.developershome.com/sms/atCommandsIntro.asp for a basic intro

#include <SoftwareSerial.h>

SoftwareSerial GPRS(2, 3); // for Gboard
//SoftwareSerial GPRS(7, 8); // for GRPS shield with Arduino
char buffer[64]; // buffer array for data recieve over serial port
int count=0;     // counter for buffer array 
void setup()
{
  GPRS.begin(9600);               // the GPRS baud rate   
  Serial.begin(9600);             // the Serial port of Arduino baud rate.
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("setup");
  Serial.println("make sure auto echo from modem is on. if not, uncomment the Serial.print in loop()");
}

void loop()
{
  if (GPRS.available())              // if date is comming from softwareserial port ==> data is comming from gprs shield
  {
    while(GPRS.available())          // reading data into char array 
    {
      buffer[count++]=GPRS.read();     // writing data into array
      if(count == 64) break;
    }
    Serial.print(buffer);            // if no data transmission ends, write buffer to hardware serial port
    clearBufferArray();              // call clearBufferArray function to clear the storaged data from the array
    count = 0;                       // set counter of while loop to zero
  }
  if (Serial.available()) {
    char c = Serial.read(); // somehow the echo mode sometimes seems deactivated
    Serial.print(c);        // this line ensure the chars are printed
    GPRS.write(c);
    // if data is available on hardwareserial port ==> data is comming from PC or notebook
  }
   // GPRS.write(Serial.read());       // write it to the GPRS shield
   //
}

void clearBufferArray()              // function to clear buffer array
{
  for (int i=0; i<count; i++)
  { 
    buffer[i]=NULL;
  }                  // clear all index of array with command NULL
}

