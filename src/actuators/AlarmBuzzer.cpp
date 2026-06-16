#include "AlarmBuzzer.h"

// Canal LEDC dédié au buzzer
static constexpr uint8_t BuzzerChannel = 0;
static constexpr uint8_t BuzzerResolution = 8;
static constexpr uint32_t BuzzerFrequency = 2000; // Hz

AlarmBuzzer::AlarmBuzzer(uint8_t pin) : pin_(pin) {}

void AlarmBuzzer::begin() {
  ledcSetup(BuzzerChannel, BuzzerFrequency, BuzzerResolution);
  ledcAttachPin(pin_, BuzzerChannel);
  ledcWriteTone(BuzzerChannel, 0); // Silence au démarrage
}

void AlarmBuzzer::setActive(bool active) {
  if (active) {
    ledcWriteTone(BuzzerChannel, BuzzerFrequency);
  } else {
    ledcWriteTone(BuzzerChannel, 0);
  }
}

