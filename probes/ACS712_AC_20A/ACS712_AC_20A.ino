/******************************************************
Precise AC Current Measurement with Arduino & ACS712
tested with Arduinos based on ATmega 328P

Stefan Thesen, 09/2015

This code uses direct programming of ADC registers
to perform high speed sampling of the ACS712 analog
signal connected to A0 pin. 
The code samples >160 times per AC period (@50Hz)
and calculated the effective current/apparent power.
This sampling is run for 100ms so that we effectively
improve the ADC resolution beyond 10Bit by 
oversampling.
For oversampling background see:
http://www.atmel.com/Images/doc8003.pdf
Exact accuracy to be determined - should be better
than 1 percent.

Code is set-up for the 20A ACS712 version.
                             
Copyright: public domain -> do what you want  
For details visit http://blog.thesen.eu 

Code from https://blog.thesen.eu/genaue-strommessung-mit-dem-arduino-und-dem-acs712-hall-sensor-mittels-oversampling/

******************************************************/

float gfLineVoltage = 235.0f;               // typical effective Voltage in Germany
float gfACS712_Factor = 27.03f;              // use 50.0f for 20A version, 75.76f for 30A version; 27.03f for 5A version
unsigned long gulSamplePeriod_us = 100000;  // 100ms is 5 cycles at 50Hz and 6 cycles at 60Hz
int giADCOffset = 512;                      // initial digital zero of the arduino input from ACS712

void setup()
{
 Serial.begin(19200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
 Serial.println("Precise AC Current Measurement with ACS712 - Stefan Thesen 09/2016");
}

void loop()
{
 long lNoSamples=0;
 long lCurrentSumSQ = 0;
 long lCurrentSum=0;

  // set-up ADC
  ADCSRA = 0x87;  // turn on adc, adc-freq = 1/128 of CPU ; keep in min: adc converseion takes 13 ADC clock cycles
  ADMUX = 0x40;   // internal 5V reference

  // 1st sample is slower due to datasheet - so we spoil it
  ADCSRA |= (1 << ADSC);
  while (!(ADCSRA & 0x10));
  
  // sample loop - with inital parameters, we will get approx 800 values in 100ms
  unsigned long ulEndMicros = micros()+gulSamplePeriod_us;
  while(micros()<ulEndMicros)
  {
    // start sampling and wait for result
    ADCSRA |= (1 << ADSC);
    while (!(ADCSRA & 0x10));
    
    // make sure that we read ADCL 1st
    long lValue = ADCL; 
    lValue += (ADCH << 8);
    lValue -= giADCOffset;

    lCurrentSum += lValue;
    lCurrentSumSQ += lValue*lValue;
    lNoSamples++;
  }
  
  // stop sampling
  ADCSRA = 0x00;

  if (lNoSamples>0)  // if no samples, micros did run over
  {  
    // correct quadradic current sum for offset: Sum((i(t)+o)^2) = Sum(i(t)^2) + 2*o*Sum(i(t)) + o^2*NoSamples
    // sum should be zero as we have sampled 5 cycles at 50Hz (or 6 at 60Hz)
    float fOffset = (float)lCurrentSum/lNoSamples;
    lCurrentSumSQ -= 2*fOffset*lCurrentSum + fOffset*fOffset*lNoSamples;
    if (lCurrentSumSQ<0) {lCurrentSumSQ=0;} // avoid NaN due to round-off effects
    
    float fCurrentRMS = sqrt((float)lCurrentSumSQ/(float)lNoSamples) * gfACS712_Factor * gfLineVoltage / 1024;
    Serial.println(fCurrentRMS);
  
    // correct offset for next round
    giADCOffset=(int)(giADCOffset+fOffset+0.5f);
  }
}

