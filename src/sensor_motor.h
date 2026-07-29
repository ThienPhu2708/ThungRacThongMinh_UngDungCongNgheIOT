#ifndef SENSOR_MOTOR_H
#define SENSOR_MOTOR_H

/*
 * Cam bien HC-SR04 so 2: giam sat muc rac (nhiem vu 2).
 *
 * Theo diagram.json: TRIG = GPIO 19, ECHO = GPIO 23.
 * BIN_HEIGHT_CM phai bang khoang cach thuc te tu cam bien den day thung khi
 * thung rong. Wokwi dang dung 100 cm, nen giu gia tri 100 de de kiem thu.
 */

#include <Arduino.h>
#include "ultrasonic_scheduler.h"

#ifndef TRASH_SENSOR_TRIG_PIN
#define TRASH_SENSOR_TRIG_PIN 19
#endif

#ifndef TRASH_SENSOR_ECHO_PIN
#define TRASH_SENSOR_ECHO_PIN 23
#endif

#ifndef BIN_HEIGHT_CM
#define BIN_HEIGHT_CM 100.0F
#endif

namespace TrashLevelSensor {
constexpr uint8_t SAMPLE_COUNT = 5;
constexpr uint32_t SAMPLE_INTERVAL_MS = 100;
constexpr uint32_t ECHO_TIMEOUT_US = 30000;
constexpr float MIN_VALID_DISTANCE_CM = 2.0F;
constexpr float EMPTY_TOLERANCE_CM = 3.0F;

inline float (&samples())[SAMPLE_COUNT] {
  static float values[SAMPLE_COUNT] = {};
  return values;
}

inline uint8_t &sampleCount() {
  static uint8_t count = 0;
  return count;
}

inline uint8_t &sampleIndex() {
  static uint8_t index = 0;
  return index;
}

inline uint32_t &lastSampleTime() {
  static uint32_t time = 0;
  return time;
}

inline float &filteredDistanceCm() {
  static float distance = BIN_HEIGHT_CM;
  return distance;
}

inline int &trashPercent() {
  static int percent = 0;
  return percent;
}

inline bool &hasValidReading() {
  static bool valid = false;
  return valid;
}

inline float readDistanceCm() {
  digitalWrite(TRASH_SENSOR_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRASH_SENSOR_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRASH_SENSOR_TRIG_PIN, LOW);

  const uint32_t duration =
      pulseIn(TRASH_SENSOR_ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1.0F;
  return (duration * 0.0343F) / 2.0F;
}

inline float medianDistance() {
  float sorted[SAMPLE_COUNT];
  const uint8_t count = sampleCount();
  for (uint8_t i = 0; i < count; ++i) sorted[i] = samples()[i];

  for (uint8_t i = 1; i < count; ++i) {
    const float value = sorted[i];
    int8_t j = static_cast<int8_t>(i) - 1;
    while (j >= 0 && sorted[j] > value) {
      sorted[j + 1] = sorted[j];
      --j;
    }
    sorted[j + 1] = value;
  }
  return sorted[count / 2];
}

inline void updatePercentFromSamples() {
  filteredDistanceCm() = medianDistance();
  const float fill =
      (BIN_HEIGHT_CM - filteredDistanceCm()) * 100.0F / BIN_HEIGHT_CM;
  trashPercent() = constrain(static_cast<int>(lroundf(fill)), 0, 100);
  hasValidReading() = true;
}
}  // namespace TrashLevelSensor

inline void setupTrashLevelSensor() {
  pinMode(TRASH_SENSOR_TRIG_PIN, OUTPUT);
  pinMode(TRASH_SENSOR_ECHO_PIN, INPUT);
  digitalWrite(TRASH_SENSOR_TRIG_PIN, LOW);
}

// Goi lien tuc trong loop(). Moi 100 ms lay mot mau va dung median 5 mau.
inline void updateTrashLevelSensor() {
  const uint32_t now = millis();
  if (now - TrashLevelSensor::lastSampleTime() <
      TrashLevelSensor::SAMPLE_INTERVAL_MS) {
    return;
  }
  if (!UltrasonicScheduler::canMeasure()) return;
  TrashLevelSensor::lastSampleTime() = now;
  UltrasonicScheduler::markMeasurementStarted();

  const float distance = TrashLevelSensor::readDistanceCm();
  // Loai bo timeout, diem mu <2 cm va gia tri vuot qua chieu cao thung.
  if (distance < TrashLevelSensor::MIN_VALID_DISTANCE_CM ||
      distance > BIN_HEIGHT_CM + TrashLevelSensor::EMPTY_TOLERANCE_CM) {
    return;
  }

  TrashLevelSensor::samples()[TrashLevelSensor::sampleIndex()] = distance;
  TrashLevelSensor::sampleIndex() =
      (TrashLevelSensor::sampleIndex() + 1) % TrashLevelSensor::SAMPLE_COUNT;
  if (TrashLevelSensor::sampleCount() < TrashLevelSensor::SAMPLE_COUNT) {
    ++TrashLevelSensor::sampleCount();
  }

  // Can it nhat 3 mau de tranh % nhay khi khoi dong.
  if (TrashLevelSensor::sampleCount() >= 3) {
    TrashLevelSensor::updatePercentFromSamples();
  }
}

inline int getTrashPercent() {
  return TrashLevelSensor::trashPercent();
}

inline float getTrashDistanceCm() {
  return TrashLevelSensor::filteredDistanceCm();
}

inline bool hasTrashLevelReading() {
  return TrashLevelSensor::hasValidReading();
}

// Dung khi xu ly nut nhan: chi cho phep xac nhan reset khi cam bien gan rong.
inline bool isTrashBinEmpty() {
  return hasTrashLevelReading() && getTrashPercent() <= 5;
}

#endif  // SENSOR_MOTOR_H
