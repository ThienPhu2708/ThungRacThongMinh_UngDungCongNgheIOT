#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

/*
 * LCD + nut nhan (nhiem vu 3, 4)
 *
 * Phan cung theo diagram.json:
 *   LCD I2C: SDA GPIO 21, SCL GPIO 22, dia chi 0x27
 *   Nut nhan: GPIO 2, noi GND, dung INPUT_PULLUP
 *
 * API de main.cpp su dung:
 *   setupDisplayUI();
 *   updateDisplayUI(percent, lidOpen);       // goi lien tuc trong loop()
 *   consumeResetRequest();                   // true mot lan khi nhan ngan
 *   isManualLidHoldRequested();              // true khi giu nut >= 800 ms
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#ifndef DISPLAY_UI_SDA_PIN
#define DISPLAY_UI_SDA_PIN 21
#endif

#ifndef DISPLAY_UI_SCL_PIN
#define DISPLAY_UI_SCL_PIN 22
#endif

#ifndef DISPLAY_UI_BUTTON_PIN
#define DISPLAY_UI_BUTTON_PIN 2
#endif

#ifndef DISPLAY_UI_LCD_ADDRESS
#define DISPLAY_UI_LCD_ADDRESS 0x27
#endif

namespace TrashDisplayUI {
constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;
constexpr uint16_t DEBOUNCE_MS = 40;
constexpr uint16_t HOLD_MS = 800;
constexpr uint16_t LCD_REFRESH_MS = 200;
constexpr uint8_t FULL_THRESHOLD = 95;
constexpr uint16_t FULL_ALERT_BLINK_MS = 500;

inline LiquidCrystal_I2C &lcd() {
  static LiquidCrystal_I2C instance(DISPLAY_UI_LCD_ADDRESS, LCD_COLUMNS,
                                    LCD_ROWS);
  return instance;
}

inline bool &rawButtonState() {
  static bool state = HIGH;
  return state;
}

inline bool &stableButtonState() {
  static bool state = HIGH;
  return state;
}

inline uint32_t &lastRawChangeTime() {
  static uint32_t time = 0;
  return time;
}

inline uint32_t &buttonPressedTime() {
  static uint32_t time = 0;
  return time;
}

inline bool &longPressDetected() {
  static bool detected = false;
  return detected;
}

inline bool &resetRequested() {
  static bool requested = false;
  return requested;
}

inline int &lastDisplayedPercent() {
  static int percent = -1;
  return percent;
}

inline int &lastDisplayedLidState() {
  static int state = -1;
  return state;
}

inline uint32_t &lastDisplayUpdateTime() {
  static uint32_t time = 0;
  return time;
}

inline int &lastFullAlertPhase() {
  static int phase = -1;
  return phase;
}

inline void printRow(uint8_t row, const char *text) {
  char padded[LCD_COLUMNS + 1];
  snprintf(padded, sizeof(padded), "%-16.16s", text);
  lcd().setCursor(0, row);
  lcd().print(padded);
}

inline void updateButton() {
  const uint32_t now = millis();
  const bool rawState = digitalRead(DISPLAY_UI_BUTTON_PIN);

  if (rawState != rawButtonState()) {
    rawButtonState() = rawState;
    lastRawChangeTime() = now;
  }

  if (now - lastRawChangeTime() >= DEBOUNCE_MS &&
      rawState != stableButtonState()) {
    stableButtonState() = rawState;

    if (stableButtonState() == LOW) {
      buttonPressedTime() = now;
      longPressDetected() = false;
    } else {
      // Nhan ngan chi duoc xac nhan khi tha nut truoc nguong nhan giu.
      if (!longPressDetected()) {
        resetRequested() = true;
      }
    }
  }

  if (stableButtonState() == LOW && !longPressDetected() &&
      now - buttonPressedTime() >= HOLD_MS) {
    longPressDetected() = true;
  }
}
}  // namespace TrashDisplayUI

inline void setupDisplayUI() {
  Wire.begin(DISPLAY_UI_SDA_PIN, DISPLAY_UI_SCL_PIN);
  TrashDisplayUI::lcd().init();
  TrashDisplayUI::lcd().backlight();
  pinMode(DISPLAY_UI_BUTTON_PIN, INPUT_PULLUP);

  TrashDisplayUI::printRow(0, "Thung rac IoT");
  TrashDisplayUI::printRow(1, "Dang khoi tao...");
}

// Goi lien tuc trong loop(). Khi rac >= 95%, LCD luan phien canh bao moi 500 ms.
inline void updateDisplayUI(int trashPercent, bool lidOpen) {
  TrashDisplayUI::updateButton();
  const int percent = constrain(trashPercent, 0, 100);
  const uint32_t now = millis();
  const bool isFull = percent >= TrashDisplayUI::FULL_THRESHOLD;
  const int fullAlertPhase =
      isFull ? static_cast<int>((now / TrashDisplayUI::FULL_ALERT_BLINK_MS) % 2)
             : 0;
  const bool contentChanged =
      percent != TrashDisplayUI::lastDisplayedPercent() ||
      static_cast<int>(lidOpen) != TrashDisplayUI::lastDisplayedLidState();
  const bool alertPhaseChanged =
      fullAlertPhase != TrashDisplayUI::lastFullAlertPhase();

  if (!contentChanged && !alertPhaseChanged &&
      now - TrashDisplayUI::lastDisplayUpdateTime() <
          TrashDisplayUI::LCD_REFRESH_MS) {
    return;
  }

  if (isFull && fullAlertPhase == 1) {
    char fullRow[TrashDisplayUI::LCD_COLUMNS + 1];
    snprintf(fullRow, sizeof(fullRow), "RAC: %3d%%", percent);
    TrashDisplayUI::printRow(0, "THUNG DA DAY");
    TrashDisplayUI::printRow(1, fullRow);
  } else {
    char firstRow[TrashDisplayUI::LCD_COLUMNS + 1];
    snprintf(firstRow, sizeof(firstRow), "Muc rac: %3d%%", percent);
    TrashDisplayUI::printRow(0, firstRow);
    TrashDisplayUI::printRow(1, lidOpen ? "Nap: MO" : "Nap: DONG");
  }
  TrashDisplayUI::lastDisplayedPercent() = percent;
  TrashDisplayUI::lastDisplayedLidState() = lidOpen;
  TrashDisplayUI::lastDisplayUpdateTime() = now;
  TrashDisplayUI::lastFullAlertPhase() = fullAlertPhase;
}

// Tra ve true dung mot lan sau thao tac nhan ngan va tha nut.
inline bool consumeResetRequest() {
  const bool requested = TrashDisplayUI::resetRequested();
  TrashDisplayUI::resetRequested() = false;
  return requested;
}

// Tra ve true sau khi giu nut >= 800 ms va con dang giu nut.
inline bool isManualLidHoldRequested() {
  return TrashDisplayUI::stableButtonState() == LOW &&
         TrashDisplayUI::longPressDetected();
}

#endif  // DISPLAY_UI_H
