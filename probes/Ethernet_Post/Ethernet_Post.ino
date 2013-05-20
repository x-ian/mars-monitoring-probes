#include <Ethernet.h>
#include <SPI.h>

byte mac[] = { 
  0x90,0xA2,0xDA,0x0D,0x86,0x4C};  //Replace with your Ethernet shield MAC
EthernetClient client;

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("setup");
  Ethernet.begin(mac);
  delay(1000);
}

void loop(){
  Serial.println("loop");
  String data;
  data=" {\"message\":{\"data\":\"PAYLOAD,1,38,3,4,20121225-135502,11,22,33,44\"}}";

  Serial.println("connecting...");
  if (client.connect("192.168.1.4",3000)) {
    Serial.println("connected");
    client.println("POST /messages/create_from_probe HTTP/1.1");
    client.println("Host: 192.168.1.4");
    client.println("Content-Type: application/json");
    client.println("Accept: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(data.length());
    client.println();
    client.print(data);
    client.println();
  }
  delay(5000);

  if (client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

}

