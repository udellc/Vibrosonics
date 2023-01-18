#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include "arduinoFFT.h"
#include "esp32FFTtoParabolicBins.h"

#define CONTROL_RATE 128

Oscil <2048, AUDIO_RATE> asinc(SIN2048_DATA); //asinc is the carrier wave. This is the sum of all other waves and the wave that should be outputted.
Oscil <2048, AUDIO_RATE> asin1(SIN2048_DATA); //20 - 29 hz
Oscil <2048, AUDIO_RATE> asin2(SIN2048_DATA); //30 - 39
Oscil <2048, AUDIO_RATE> asin3(SIN2048_DATA); //40 - 49
Oscil <2048, AUDIO_RATE> asin4(SIN2048_DATA); //50 - 59
Oscil <2048, AUDIO_RATE> asin5(SIN2048_DATA); //60 - 69
Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //70 - 80


const int OscilCount = 8; //The total number of waves. Modify this if more waves are added, or the program will segfault.

int gainzDivisor;

double sickGainz[OscilCount] = { 0 }; //Set between 0 and 1.
int realGainz[OscilCount] = { 0 };  //Holds translated double value to integer between 0 and 255 for actual gain settings.

long currentCarrier;
long nextCarrier;

double avgAmps[8];
double highestAmp;

void setup()
{
  Serial.begin(115200);
  startMozzi(CONTROL_RATE);

  asinc.setFreq(16); //16 Hz - Base shaker sub-harmonic freq.
  asin1.setFreq(30); //Set frequencies here. (Hardcoded for now)
  asin2.setFreq(40);
  asin3.setFreq(50);

  FFTtoParaSetup();
}

void loop()
{
  audioHook();
}

void updateSickGainz(double gainz[])
{
	
	for(int i = 0; i < 8; i++)
	{
		if(highestAmp > 0)
		{
			sickGainz[i] = avgAmps[i] / highestAmp;
		}
		else
		{
			sickGainz[i] = 0;
		}

    //Serial.println(sickGainz[i]);

	}
}

void sickGainzTranslation(double gainz[]) //Used to translate the 0-1 amplitude scale to a 0-255 integer scale.
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

    //Serial.println(realGainz[i]);
  }
}


void updateControl()
{
  FFTtoParaLoop();
  updateSickGainz(sickGainz);
	sickGainzTranslation(sickGainz);
  gainzControl();

  //========================================== DEBUG BELOW
 //for(int i = 0; i < OscilCount; i++)
 //{
   //Serial.printf("Gain %d: %f\n", i, sickGainz[i]);
 //}
  
  //Serial.printf("currentCarrier: %ld | nextCarrier: %ld\n", currentCarrier, nextCarrier);
  Serial.printf("currentCarrier: %ld\n", nextCarrier);
  //Serial.printf("asinNext: %ld\n", asin1.next());
  //delay(500);
  //Serial.println("updateControlend");
  //sickGainz[0] = 0;
  //sickGainz[1] = 0;
}

 //Used for amplitude modulation so it does not exceed the 255 cap.
 void gainzControl()
{
  int totalGainz = 0;
  gainzDivisor = 0;

  for(int i = 0; i < OscilCount; i++)
  {
    totalGainz += realGainz[i];
  }

  totalGainz = 255 * totalGainz;
  
  while(totalGainz > 255 && totalGainz != 0)
  {
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
                    realGainz[3] * asin3.next() ); //Additive synthesis equation

  nextCarrier = currentCarrier >> gainzDivisor;
  //Serial.println("updateAudioEnd");
	return (int)nextCarrier;
}