#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

#define CONTROL_RATE 128

int waveCount = 0;

Oscil <2048, AUDIO_RATE> asinc(SIN2048_DATA); //asinc is the carrier wave. This is the sum of all other waves and the wave that should be outputted.
Oscil <2048, AUDIO_RATE> asin1(SIN2048_DATA); //20 - 29 hz
// Oscil <2048, AUDIO_RATE> asin2(SIN2048_DATA); //30 - 39
// Oscil <2048, AUDIO_RATE> asin3(SIN2048_DATA); //40 - 49

const int OscilCount = 2;  //The total number of waves. Modify this if more waves are added, or the program will segfault.

int gainzDivisor;

float sickGainz[OscilCount] = { 0 };  //Set between 0 and 1.
int realGainz[OscilCount] = { 0 };    //Holds translated float value to integer between 0 and 255 for actual gain settings.

long currentCarrier;
long nextCarrier;

int addSynthCarrier;
int addSyntComplete = 0;

void setup() {
  Serial.begin(115200);

  startMozzi(CONTROL_RATE);
  asinc.setFreq(16);  //16 Hz - Base shaker sub-harmonic freq.
  asin1.setFreq(30);  //Set frequencies here. (Hardcoded for now)
  // asin2.setFreq(60);
  // asin3.setFreq(80);

  sickGainz[0] = 0;
  sickGainz[1] = 0;  //Set amplitude here. The array index corresponds to the sin wave number. Use any value between 0-1 for amplitude. (Hardcoded for now)
  // sickGainz[2] = 0;
  // sickGainz[3] = 0;

  while (!Serial)
    ;
  Serial.println("Ready");
}

void loop() {
  // put your main code here, to run repeatedly:
  /* Additive Synthesis */
  Serial.print('\t');
  addSyntComplete = 0;
  while (addSyntComplete == 0) {
    audioHook();
  }
  Serial.printf("Min:%d\tMax:%d\tCarrier:%d", 0, 255, addSynthCarrier);
  Serial.println();

  char dataT = '0';
  while (Serial.available()) {
    char dataT = Serial.read();
    if (dataT == 'n') {
      if (waveCount == 0) {
        //asinc.setFreq(16);
        sickGainz[0] = 0.5;
        waveCount++;
        continue;
      }
      if (waveCount == 1) {
        sickGainz[0] = 1.0;
        waveCount++;
        continue;
      }
      if (waveCount == 2) {
        sickGainz[0] = 0;
        sickGainz[1] = 0.5;
        waveCount++;
        continue;
      }
      if (waveCount == 3) {
        sickGainz[0] = 0.5;
        sickGainz[1] = 1.0;
        waveCount++;
        continue;
      }
      if (waveCount == 4) {
        sickGainz[0] = 0;
        sickGainz[1] = 0;
        waveCount++;
        waveCount = 0;
        continue;
      }
    }
  }
}

void sickGainzTranslation(float *gainz)  //Used to translate the 0-255 amplitude scale to a 0-1 float scale.
{
  for (int i = 0; i < OscilCount; i++) {
    if (gainz[i] > 1.0) {
      Serial.println("ERROR: Gain value out of bounds");
    } else if (gainz[i] > 0.0) {
      realGainz[i] = (int)(gainz[i] * 255);
    } else {
      realGainz[i] = 0;
    }
  }
}

void updateControl() {
  //Serial.println("updateControlStart");
  //sickGainzTranslation(sickGainz);
  //Serial.println("after translation");
  //gainzControl();
  //Serial.println("after control");

  //========================================== DEBUG BELOW
  // for(int i = 0; i < OscilCount; i++)
  //{
  //  Serial.printf("Gain %d: %f\n", i, sickGainz[i]);
  // }

  //Serial.printf("currentCarrier: %ld | nextCarrier: %ld\n", currentCarrier, nextCarrier);
  addSynthCarrier = nextCarrier;
  addSyntComplete = 1;
  //Serial.println("updateControlend");
}

void gainzControl()  //Used for amplitude modulation so it does not exceed the 255 cap.
{
  int totalGainz = 0;
  gainzDivisor = 0;

  for (int i = 0; i < OscilCount; i++) {
    totalGainz += realGainz[i];
  }

  totalGainz *= 255;

  while (totalGainz > 255 && totalGainz != 0) {
    //Serial.println(totalGainz);
    totalGainz >>= 1;
    gainzDivisor++;
  }
}

int updateAudio() {
  //Serial.println("updateAudioStart");
  //currentCarrier = ((long)asinc.next() + realGainz[1] * asin1.next() + realGainz[2] * asin2.next() + realGainz[3] * asin3.next() + realGainz[4] * asin4.next() + realGainz[5] * asin5.next()); //+ realGainz[6] * asind.next());  //Additive synthesis equation
  
  currentCarrier = round(sickGainz[0] * asinc.next() + sickGainz[1] * asin1.next());

  //currentCarrier = ((long)asinc.next() + realGainz[6] * asind.next());  //Additive synthesis equation


  nextCarrier = currentCarrier >> gainzDivisor;
  //Serial.println("updateAudioEnd");
  return (int)nextCarrier;
}
