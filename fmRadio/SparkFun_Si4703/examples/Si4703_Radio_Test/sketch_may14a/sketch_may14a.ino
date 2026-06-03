#include <Wire.h>
#include <SparkFunSi4703.h>

#define RESET_PIN 27
#define SDA_PIN 23
#define SCL_PIN 22
#define STC_PIN 4

Si4703_Breakout radio(RESET_PIN, SDA_PIN, SCL_PIN, STC_PIN);
void scanI2C() {
  Serial.println("[I2C] scanning...");

  bool found = false;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("[I2C] device found at 0x");
      Serial.println(addr, HEX);
      found = true;
    }
  }

  if (!found) {
    Serial.println("[ERROR] No I2C devices found!");
  } else {
    Serial.println("[OK] I2C device detected");
  }
}

void setup() {

  Serial.begin(115200);

  // SI4703 RESET SEQUENCE
  pinMode(SDA_PIN, OUTPUT);
  digitalWrite(SDA_PIN, LOW);

  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(50);

  // I2C START
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  Serial.println("\n=== STARTUP CHECK ===");

  scanI2C();

  // RADIO START
  Serial.println("[INFO] Powering radio...");
  radio.powerOn();
  delay(500);

  radio.setVolume(15);
  radio.setChannel(887); //KVBR

  Serial.println("[OK] Radio Ready");
}

void loop() {

}