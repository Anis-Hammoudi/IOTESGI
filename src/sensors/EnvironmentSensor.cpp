#include "EnvironmentSensor.h"

EnvironmentSensor::EnvironmentSensor(uint8_t pin) : dht_(pin, DHT22) {}

bool EnvironmentSensor::begin() {
  dht_.begin();
  // Le DHT22 nécessite ~2s de stabilisation après mise sous tension
  delay(2000);
  available_ = true;
  Serial.println("[env] DHT22 initialised");
  return available_;
}

EnvironmentReadings EnvironmentSensor::read() {
  EnvironmentReadings readings;
  if (!available_) {
    Serial.println("[env] DHT22 not available");
    return readings;
  }

  const float rawTemp = dht_.readTemperature();
  const float rawHum = dht_.readHumidity();

  // Rejet des valeurs aberrantes
  if (isnan(rawTemp) || isnan(rawHum) || 
      rawTemp < -40.0f || rawTemp > 80.0f || 
      rawHum < 0.0f || rawHum > 100.0f) {
    if (readFailCount_++ % 10 == 0) {
      Serial.printf("[env] DHT22 read failed (count=%u) rawT=%.1f rawH=%.1f\n",
                    readFailCount_, rawTemp, rawHum);
    }
    readings.temperatureC = filteredTemp_;
    readings.humidityPercent = filteredHum_;
    readings.valid = !isnan(filteredTemp_) && !isnan(filteredHum_);
    return readings;
  }

  readFailCount_ = 0;

  // Filtrage EMA
  if (isnan(filteredTemp_)) {
    filteredTemp_ = rawTemp;
    filteredHum_ = rawHum;
  } else {
    filteredTemp_ = alpha_ * rawTemp + (1.0f - alpha_) * filteredTemp_;
    filteredHum_ = alpha_ * rawHum + (1.0f - alpha_) * filteredHum_;
  }

  readings.temperatureC = filteredTemp_;
  readings.humidityPercent = filteredHum_;
  readings.valid = true;
  return readings;
}
