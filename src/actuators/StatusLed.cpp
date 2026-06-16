#include "StatusLed.h"

StatusLed::StatusLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin)
    : redPin_(redPin), greenPin_(greenPin), bluePin_(bluePin) {}

void StatusLed::begin() {
  pinMode(redPin_, OUTPUT);
  pinMode(greenPin_, OUTPUT);
  pinMode(bluePin_, OUTPUT);
  write(false, false, false);
}

void StatusLed::show(ReactorStatus status, bool mqttConnected) {
  if (!mqttConnected && status != ReactorStatus::Critical && status != ReactorStatus::EmergencyStop) {
    // Bleu fixe = perte de connectivité MQTT
    write(false, false, true);
    return;
  }

  switch (status) {
    case ReactorStatus::Nominal:
      write(false, true, false);   // Vert = Nominal
      break;
    case ReactorStatus::Warning:
      write(true, true, false);    // Jaune (Rouge + Vert) = Attention
      break;
    case ReactorStatus::Critical:
      write(true, false, false);   // Rouge = Critique
      break;
    case ReactorStatus::EmergencyStop:
      write(true, false, true);    // Magenta (Rouge + Bleu) = Arrêt urgence
      break;
    case ReactorStatus::Offline:
    default:
      write(false, false, true);   // Bleu faible = Offline
      break;
  }
}

void StatusLed::write(bool red, bool green, bool blue) {
  digitalWrite(redPin_, red ? HIGH : LOW);
  digitalWrite(greenPin_, green ? HIGH : LOW);
  digitalWrite(bluePin_, blue ? HIGH : LOW);
}

