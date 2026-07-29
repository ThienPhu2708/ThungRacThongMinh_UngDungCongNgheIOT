
#include <Arduino.h>
#include "auto_lid.h"
#include "display_ui.h"
#include "network.h"
#include "sensor_motor.h"

void setup() {
  Serial.begin(115200);
  setupDisplayUI();
  setupAutoLid();
  setupTrashLevelSensor();
  setupNetwork();
}

void loop(){
  runNetworkLoop();
  updateTrashLevelSensor();
  const int trashPercent = getTrashPercent();

  // updateDisplayUI() dong thoi doc va chong doi nut nhan.
  updateDisplayUI(trashPercent, isLidOpen());
  updateAutoLid(isManualLidHoldRequested() || isBlynkHoldRequested());
  updateDisplayUI(trashPercent, isLidOpen());

  // Nhan ngan chi xac nhan da do rac khi cam bien da do duoc thung gan rong.
  if (consumeResetRequest()) {
    if (isTrashBinEmpty()) {
      Serial.println("Thung rong: da xac nhan reset.");
    } else {
      Serial.println("Khong reset: cam bien van phat hien rac.");
    }
  }

  syncDataToCloud(trashPercent);
}

