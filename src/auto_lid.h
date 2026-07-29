#ifndef AUTO_LID_H
#define AUTO_LID_H

/*
 * HC-SR04 so 1 + Servo SG90 (nhiem vu 1).
 * TRIG = GPIO 5, ECHO = GPIO 18, Servo PWM = GPIO 13 theo diagram.json.
 *
 * Nguong mo 20 cm va nguong roi 30 cm tao hysteresis: mot nguoi dung chi
 * kich hoat mot lan, khong lam nap mo lai lien tuc khi dung canh thung.
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include "ultrasonic_scheduler.h"

#ifndef PRESENCE_SENSOR_TRIG_PIN
#define PRESENCE_SENSOR_TRIG_PIN 5
#endif

#ifndef PRESENCE_SENSOR_ECHO_PIN
#define PRESENCE_SENSOR_ECHO_PIN 18
#endif

#ifndef LID_SERVO_PIN
#define LID_SERVO_PIN 13
#endif

namespace AutoLid {
constexpr float OPEN_DISTANCE_CM = 20.0F;  //khoảng cách nhận diện cảm biến  
constexpr float REARM_DISTANCE_CM = 30.0F; // khoảng cách đi xa để hệ thống nhận diện lại
constexpr uint32_t LID_OPEN_DURATION_MS = 3000;  //tg đóng mở nắp
constexpr uint32_t SAMPLE_INTERVAL_MS = 150;
constexpr uint32_t ECHO_TIMEOUT_US = 30000;
constexpr uint8_t REQUIRED_NEAR_SAMPLES = 2;

inline Servo &servo() {
  static Servo instance;
  return instance;
}

inline bool &lidOpen() {
  static bool open = false;
  return open;
}

inline bool &armed() {
  static bool canOpen = true;
  return canOpen;
}

inline uint32_t &lidOpenedAt() {
  static uint32_t time = 0;
  return time;
}

inline uint32_t &lastSampleTime() {
  static uint32_t time = 0;
  return time;
}

inline float &lastDistanceCm() {
  static float distance = -1.0F;
  return distance;
}

inline uint8_t &nearSampleCount() {
  static uint8_t count = 0;
  return count;
}

inline float readDistanceCm() {
  digitalWrite(PRESENCE_SENSOR_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(PRESENCE_SENSOR_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(PRESENCE_SENSOR_TRIG_PIN, LOW);

  const uint32_t duration =
      pulseIn(PRESENCE_SENSOR_ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1.0F;
  return (duration * 0.0343F) / 2.0F;
}

inline void setLidOpen(bool open) {
  if (open == lidOpen()) return;
  servo().write(open ? 90 : 0);
  lidOpen() = open;
  if (open) lidOpenedAt() = millis();
}
}  // namespace AutoLid

inline void setupAutoLid() {
  pinMode(PRESENCE_SENSOR_TRIG_PIN, OUTPUT);
  pinMode(PRESENCE_SENSOR_ECHO_PIN, INPUT);
  digitalWrite(PRESENCE_SENSOR_TRIG_PIN, LOW);

  AutoLid::servo().setPeriodHertz(50);
  AutoLid::servo().attach(LID_SERVO_PIN, 500, 2400);
  AutoLid::servo().write(0);
}

// Goi lien tuc trong loop(). forceOpen = true khi nguoi dung nhan giu nut.
inline void updateAutoLid(bool forceOpen) {
  const uint32_t now = millis();

  if (forceOpen) {
    AutoLid::setLidOpen(true);
    return;
  }

  // Tu dong dong sau 3 giay, ke ca khi nguoi dung van dung gan thung.
  if (AutoLid::lidOpen() &&
      now - AutoLid::lidOpenedAt() >= AutoLid::LID_OPEN_DURATION_MS) {
    AutoLid::setLidOpen(false);
  }

  if (now - AutoLid::lastSampleTime() < AutoLid::SAMPLE_INTERVAL_MS ||
      !UltrasonicScheduler::canMeasure()) {
    return;
  }
  AutoLid::lastSampleTime() = now;
  UltrasonicScheduler::markMeasurementStarted();

  const float distance = AutoLid::readDistanceCm();
  if (distance < 2.0F || distance > 400.0F) {
    AutoLid::nearSampleCount() = 0;
    return;
  }
  AutoLid::lastDistanceCm() = distance;

  if (distance < AutoLid::OPEN_DISTANCE_CM) {
    if (AutoLid::nearSampleCount() < AutoLid::REQUIRED_NEAR_SAMPLES) {
      ++AutoLid::nearSampleCount();
    }
  } else {
    AutoLid::nearSampleCount() = 0;
  }

  // Chi tai kich hoat sau khi nap da dong va nguoi dung da roi khoi >30 cm.
  if (!AutoLid::lidOpen() && !AutoLid::armed() &&
      distance > AutoLid::REARM_DISTANCE_CM) {
    AutoLid::armed() = true;
  }

  if (!AutoLid::lidOpen() && AutoLid::armed() &&
      AutoLid::nearSampleCount() >= AutoLid::REQUIRED_NEAR_SAMPLES) {
    AutoLid::armed() = false;
    AutoLid::nearSampleCount() = 0;
    AutoLid::setLidOpen(true);
  }
}

inline bool isLidOpen() {
  return AutoLid::lidOpen();
}

inline float getPresenceDistanceCm() {
  return AutoLid::lastDistanceCm();
}

#endif  // AUTO_LID_H
