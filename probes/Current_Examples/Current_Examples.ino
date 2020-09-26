const int sensorIn = A2;

void setup(){ 
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.println(F("setup"));
  pinMode(sensorIn, INPUT);
}

void loop(){

  Serial.println(readInternalVcc());
  
  //float amps = acs712AcAmps(66, sensorIn);
  // 5110mV is voltage of my MBP with Leonardo USB, 513mV (or 528?) for no load 
  //int mAmps = pololuAcs715DcMilliAmps(A0, 5110, 513 /*528*/ );
  //int mAmps = acs712DcMilliAmps(A0, 66, 5110, 512 /* 510, 511 */ );
  //float amps = pololuAcs709DcAmps();
  int mAmps = pololuAcs711DcMilliAmps(A2, 5110, 516 );

  Serial.print(" ");
  Serial.println(mAmps);
  Serial.println();
  
//  Serial.print(amps);
//  Serial.println(" Amps ");
  delay(1500);
}

int sampleAnalogRead(int analogInputPin, int sampleSize, int delayPeriod) {
  long rawValue = 0;
  int in = 0;
  for (int i = 0; i < sampleSize; i++) {
    in = analogRead(analogInputPin);
    //Serial.print(" ");
    //Serial.print(in);
    rawValue += in;
    delay(delayPeriod);
  }
  int rawValueAvg = (long) rawValue / sampleSize;
  Serial.print(": ");
  Serial.print(rawValueAvg);
  return rawValueAvg;
}

// use 185 for 5A, 100 for 20A Module and 66 for 30A Module
// ACS712 - Allegro Microsystems
// use 185 for 5A, 100 for 20A Module and 66 for 30A Module
// 'borrowed' from http://circuits4you.com/2016/05/13/arduino-asc712-current/
int acs712DcMilliAmps(int analogInputPin, int mVPerAmp, int mVReference, int zeroLoadOffset)
{
  int rawValue = sampleAnalogRead(analogInputPin, 40, 10);
  //Serial.println(rawValue);
  float voltage = ((rawValue - zeroLoadOffset) / 1024.0) * mVReference * 1000;
  return voltage / mVPerAmp;
}

// 5 A wasnt too precise, 30A with a AC load of 1.45 A pretty good
// however both show a load even if nothing is connected
// use 185 for 5A, 100 for 20A Module and 66 for 30A Module
// 'borrowed' from http://circuits4you.com/2016/05/13/ac-current-measurement-acs712/
float acs712AcAmps(int mVPerAmp, int analogInputPin)
{
  float voltage = getVPP(analogInputPin);
  Serial.println(voltage);
  float vrms = (voltage/2.0) *0.707;  //root 2 is 0.707
  return (vrms * 1000)/mVPerAmp;
}

float getVPP(int analogInputPin)
{
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here

  uint32_t start_time = millis();
  while((millis()-start_time) < 1000) //sample for 1 Sec
  {
    readValue = analogRead(analogInputPin);
    // see if you have a new maxValue
    if (readValue > maxValue) 
    {
      /*record the maximum sensor value*/
      maxValue = readValue;
    }
    if (readValue < minValue) 
    {
      /*record the minimum sensor value*/
      minValue = readValue;
    }
  }

  // Subtract min from max
  result = ((maxValue - minValue) * 5.0)/1024.0;

  return result;
}

// https://www.pololu.com/product/1186
int pololuAcs715DcMilliAmps(int analogInputPin, int mVReference, int zeroLoadOffset) {

  // reference voltage, should be 5 V, but can vary a bit
  // with my MacBookPro and Leonardo via USB it is 5110
  //const int mVReference = 5110;

  // 133 mV / Amp, depending on mVRef?
  const int mVperAmp = 133;

  // ~103 analogread without load
  // offset 500 when Vin is 5V, unclear but according to datasheet it might be mVRef * 0.1
  // measure Vout of sensor against GND
  // 528 on my Arduino USB with MBP
  //const int zeroLoadOffset = 528;

  int sensorValue = sampleAnalogRead(analogInputPin, 40, 10);
  int milliAmps = (((long)sensorValue * mVReference / 1024) - zeroLoadOffset ) * 1000 / mVperAmp;
  return milliAmps;
}

float pololuAcs709DcAmps() {
  
  float aRef = 3.28; //4.98; // AREF voltage measured with accurate volt meter
  float amps;

  Serial.print("analogRead: ");
   Serial.println(analogRead(A0));
  
  amps = (analogRead(A0) * aRef / 1023.0 - (aRef / 2)) / 0.028;
  return amps;
}

// https://www.pololu.com/product/2452
int pololuAcs711DcMilliAmps(int analogInputPin, int mVReference, int zeroLoadOffset) {

  // reference voltage, should be 5 V, but can vary a bit
  // with my MacBookPro and Leonardo via USB it is 5110
  //const int mVReference = 5110;

  // 36.7 * (voltage / mVReference) - 18.3

  // 136 mV / Amp for +/- 15 A version, depending on mVRef
  // 68 mV / Amp for +/- 31 A version, depending on mVRef
  const int mVperAmp = 68 ;
  
  int rawValue = sampleAnalogRead(analogInputPin, 40, 10);
  float voltage = ((rawValue - zeroLoadOffset) / 1023.0) * mVReference;
  float mamps = voltage / mVperAmp;
  return (int) (mamps * 1000);
}

long readInternalVcc() {

  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);                                                    // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                                         // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result;                                  // Back-calculate AVcc in mV
  return result;
}
