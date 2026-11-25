#include <SPI.h>
#include <RF24.h>

// Shared VSPI pins on ESP32 DevKit: SCK=18, MISO=19, MOSI=23
#define RADIO1_CE   4
#define RADIO1_CSN  5

#define RADIO2_CE   16
#define RADIO2_CSN  17

#define BUTTON_PIN  27

RF24 radio1(RADIO1_CE, RADIO1_CSN);
RF24 radio2(RADIO2_CE, RADIO2_CSN);

const uint8_t CHANNELS = 126; // 2.400 GHz to 2.525 GHz
uint8_t energyCounts[CHANNELS];

volatile bool modeToggleRequested = false;
bool dualSweepMode = false;
unsigned long lastDebounceMs = 0;

void IRAM_ATTR onButtonPress() {
  unsigned long now = millis();
  if (now - lastDebounceMs > 200) {
    modeToggleRequested = true;
    lastDebounceMs = now;
  }
}

void configureRadio(RF24 &r, const char *name) {
  if (!r.begin()) {
    Serial.printf("[%s] Failed to initialize. Check 3.3V power, capacitor, and SPI/CE/CSN wiring.\n", name);
    return;
  }
  r.setAutoAck(false);
  r.setPALevel(RF24_PA_LOW); // Use RF24_PA_MAX only with a rock-solid 3.3V supply
  r.setDataRate(RF24_1MBPS);
  r.startListening();
  r.stopListening();
  Serial.printf("[%s] Ready.\n", name);
}

void scanEnergy(RF24 &r, uint8_t samplesPerChannel = 64) {
  memset(energyCounts, 0, sizeof(energyCounts));
  for (uint8_t i = 0; i < samplesPerChannel; i++) {
    for (uint8_t ch = 0; ch < CHANNELS; ch++) {
      r.setChannel(ch);
      r.startListening();
      delayMicroseconds(130);
      r.stopListening();
      if (r.testRPD() || r.testCarrier()) {
        energyCounts[ch]++;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);

  SPI.begin(18, 19, 23, -1);

  configureRadio(radio1, "Radio 1");
  configureRadio(radio2, "Radio 2");
}

void loop() {
  if (modeToggleRequested) {
    modeToggleRequested = false;
    dualSweepMode = !dualSweepMode;
    Serial.printf("Mode switched -> %s\n", dualSweepMode ? "Dual Radio Scan" : "Radio 1 Channel Scan");
  }

  scanEnergy(radio1, 64);

  uint8_t peakCh = 0;
  uint8_t peakVal = 0;
  for (uint8_t ch = 0; ch < CHANNELS; ch++) {
    if (energyCounts[ch] > peakVal) {
      peakVal = energyCounts[ch];
      peakCh = ch;
    }
  }

  if (dualSweepMode) {
    scanEnergy(radio2, 64);
    uint8_t peakCh2 = 0;
    uint8_t peakVal2 = 0;
    for (uint8_t ch = 0; ch < CHANNELS; ch++) {
      if (energyCounts[ch] > peakVal2) {
        peakVal2 = energyCounts[ch];
        peakCh2 = ch;
      }
    }
    Serial.printf("R1 Peak: ch %u (%u) | R2 Peak: ch %u (%u)\n", peakCh, peakVal, peakCh2, peakVal2);
  } else {
    Serial.printf("R1 Peak Channel: %u (2.%03u GHz), Activity: %u/64\n",
                  peakCh, 400 + peakCh, peakVal);
  }

  delay(300);
}
