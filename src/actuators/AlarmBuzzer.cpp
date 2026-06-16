#include "AlarmBuzzer.h"

AlarmBuzzer::AlarmBuzzer(uint8_t pin) : pin_(pin) {}

void AlarmBuzzer::begin() {
  pinMode(pin_, OUTPUT);
  // Le module HW-c508 semble être "Active High" (il sonne avec du 3.3V).
  // On le met donc à LOW pour le rendre silencieux par défaut.
  digitalWrite(pin_, LOW); 
}

void AlarmBuzzer::setActive(bool active) {
  if (active) {
    digitalWrite(pin_, HIGH); // HIGH = Alarme ON
  } else {
    digitalWrite(pin_, LOW);  // LOW = Alarme OFF
  }
}
