#include "RadiationSensor.h"

RadiationSensor::RadiationSensor(uint8_t pin) : pin_(pin), oneWire_(pin), dallas_(&oneWire_) {}

void RadiationSensor::begin() {
  // Ne PAS appeler pinMode() ici — la bibliothèque OneWire gère le pin elle-même.
  // Le pull-up externe 4.7kΩ entre VCC et DATA est obligatoire.
  dallas_.begin();

  // Log le nombre de capteurs détectés pour le diagnostic
  uint8_t deviceCount = dallas_.getDeviceCount();
  Serial.printf("[ds18b20] Devices found on OneWire bus: %u\n", deviceCount);

  if (deviceCount == 0) {
    Serial.println("[ds18b20] WARNING: No DS18B20 sensor detected! Check wiring and 4.7k pull-up.");
  }

  dallas_.setResolution(11);
  dallas_.setWaitForConversion(false);
  dallas_.requestTemperatures();
}

float RadiationSensor::readCoreTemperatureC() {
  const float value = dallas_.getTempCByIndex(0);
  dallas_.requestTemperatures();
  // Rejet des valeurs aberrantes
  if (value == DEVICE_DISCONNECTED_C || value < -55.0F || value > 125.0F) {
    return filteredTemp_;
  }

  // Filtrage EMA
  if (isnan(filteredTemp_)) {
    filteredTemp_ = value;
  } else {
    filteredTemp_ = alpha_ * value + (1.0f - alpha_) * filteredTemp_;
  }

  return filteredTemp_;
}

