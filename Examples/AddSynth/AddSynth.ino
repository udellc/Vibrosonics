#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

#define CONTROL_RATE 128

Oscil <2048, AUDIO_RATE> asinc(SIN2048_DATA);
Oscil <2048, AUDIO_RATE> asin1(SIN2048_DATA);
Oscil <2048, AUDIO_RATE> asin2(SIN2048_DATA);
Oscil <2048, AUDIO_RATE> asin3(SIN2048_DATA);

const int OscilCount = 4;

int gainzDivisor;

float sickGainz[OscilCount] = { 0 }; //Set between 0 and 1.
int realGainz[OscilCount] = { 0 };  //Holds translated float value to integer between 0 and 255 for actual gain settings.

long currentCarrier;
long nextCarrier;

void setup() 
{
  //Serial.println("InSetup");
  Serial.begin(115200);

  startMozzi(CONTROL_RATE);
	asinc.setFreq(16); //Base shaker sub-harmonic freq.
  asin1.setFreq(30);
  asin2.setFreq(40);
  asin3.setFreq(50);

  //sickGainz[0] = 0;
  sickGainz[1] = 1;
  //Serial.println("endSetup");
}

void loop() 
{

 audioHook();

}

void sickGainzTranslation(float gainz[])
{
  for(int i = 0; i < OscilCount; i++)
  {
    if(gainz[i] > 1 || gainz[i] < 0)
    {
      Serial.println("ERROR: Gain value out of bounds");
    }
    else if (gainz[i] > 0)
    {
      realGainz[i] = (int)(gainz[i] * 255);
    }
    else
    {
      realGainz[i] = 0;
    }
  }
}


void updateControl()
{
  //Serial.println("updateControlStart");
	sickGainzTranslation(sickGainz);
  //Serial.println("after translation");
  gainzControl();
  //Serial.println("after control");

  //========================================== DEBUG BELOW
 // for(int i = 0; i < OscilCount; i++)
  //{
  //  Serial.printf("Gain %d: %f\n", i, sickGainz[i]);
 // }
  
  //Serial.printf("currentCarrier: %ld | nextCarrier: %ld\n", currentCarrier, nextCarrier);
  Serial.printf("currentCarrier: %ld\n", currentCarrier);
  delay(500);
  //Serial.println("updateControlend");
  sickGainz[0] = 0;
  sickGainz[1] = 0;
}

void gainzControl()
{
  int totalGainz = 0;
  gainzDivisor = 0;

  for(int i = 0; i < OscilCount; i++)
  {
    totalGainz += realGainz[i];
  }

  while(totalGainz > 255 && totalGainz != 0)
  {
    //Serial.println(totalGainz);
    totalGainz >>= 1;
    gainzDivisor++;
  }

}

int updateAudio()
{
  //Serial.println("updateAudioStart");
  currentCarrier = ((long)asinc.next() +
                    realGainz[1] * asin1.next() +
                    realGainz[2] * asin2.next() +
                    realGainz[3] * asin3.next() );

  nextCarrier = currentCarrier >> gainzDivisor;
  //Serial.println("updateAudioEnd");
	return (int)nextCarrier;
}