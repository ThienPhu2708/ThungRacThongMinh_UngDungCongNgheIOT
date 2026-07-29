#ifndef ULTRASONIC_SCHEDULER_H
#define ULTRASONIC_SCHEDULER_H

#include <Arduino.h>

// Hai HC-SR04 cung tan so 40 kHz nen khong duoc phat xung gan nhau.
namespace UltrasonicScheduler {
constexpr uint32_t MIN_GAP_BETWEEN_MEASUREMENTS_MS = 50;

inline uint32_t &lastMeasurementTime() {
  static uint32_t time = 0;
  return time;
}

inline bool canMeasure() {
  return millis() - lastMeasurementTime() >=
         MIN_GAP_BETWEEN_MEASUREMENTS_MS;
}
  
inline void markMeasurementStarted() {
  lastMeasurementTime() = millis();
}
}  // namespace UltrasonicScheduler

#endif  // ULTRASONIC_SCHEDULER_H
