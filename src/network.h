#ifndef NETWORK_H
#define NETWORK_H

#ifndef BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_ID "TMPL6I93vXUve"
#endif

#ifndef BLYNK_TEMPLATE_NAME
#define BLYNK_TEMPLATE_NAME "ThungRacThongMinh"
#endif

#ifndef BLYNK_AUTH_TOKEN
#define BLYNK_AUTH_TOKEN "nUrYVnv9kH9SdR5hGQJeEYOefluR35SG"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Wi-Fi mac dinh cua Wokwi. Thay khi nap vao ESP32 that, hoac dung build_flags.
#ifndef WIFI_SSID
#define WIFI_SSID "Wokwi-GUEST"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

namespace TrashNetwork {
constexpr uint8_t FULL_THRESHOLD = 95;
constexpr uint8_t ALERT_RESET_THRESHOLD = 90;
constexpr uint32_t RECONNECT_INTERVAL_MS = 10000;
constexpr uint32_t PUBLISH_INTERVAL_MS = 2000;

inline bool &fullAlertSent() {
  static bool sent = false;
  return sent;
}

inline uint32_t &lastReconnectAttempt() {
  static uint32_t lastAttempt = 0;
  return lastAttempt;
}

inline int &lastPublishedPercent() {
  static int percent = -1;
  return percent;
}

inline uint32_t &lastPublishTime() {
  static uint32_t lastTime = 0;
  return lastTime;
}

inline int &lastPublishedStatusBand() {
  static int band = -1;
  return band;
}

inline uint8_t statusBandFor(uint8_t trashPercent) {
  if (trashPercent >= FULL_THRESHOLD) return 2;
  if (trashPercent >= 70) return 1;
  return 0; 
}

inline const char *statusFor(uint8_t trashPercent) {
  if (trashPercent >= FULL_THRESHOLD) return "ĐẦY - CẦN ĐỔ RÁC";
  if (trashPercent >= 70) return "GẦN ĐẦY";
  return "RỖNG";
}

inline const char *colorFor(uint8_t trashPercent) {
  if (trashPercent >= FULL_THRESHOLD) return "#D3435C";  // do
  if (trashPercent >= 70) return "#ED9D00";               // cam
  return "#23C48E";                                       // xanh
}
}  // namespace TrashNetwork

// Goi mot lan trong setup(), sau Serial.begin(...).
inline void setupNetwork() {
  Serial.println("Dang khoi tao Wi-Fi va Blynk...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Blynk.config(BLYNK_AUTH_TOKEN);
}

// Dong bo % rac va trang thai len Dashboard. Goi khi co so do moi.
inline void syncDataToCloud(int trashPercent) {
  const uint8_t percent = constrain(trashPercent, 0, 100);

  // Khong gui lien tuc neu ham nay duoc goi trong moi vong loop().
  // Van gui ngay khi % thay doi; khi khong doi, cap nhat lai moi 2 giay.
  const bool valueChanged = percent != TrashNetwork::lastPublishedPercent();
  const bool publishDue = millis() - TrashNetwork::lastPublishTime() >=
                          TrashNetwork::PUBLISH_INTERVAL_MS;
  if (Blynk.connected() && (valueChanged || publishDue)) {
    Blynk.virtualWrite(V0, percent);
    Blynk.virtualWrite(V1, TrashNetwork::statusFor(percent));
    const uint8_t statusBand = TrashNetwork::statusBandFor(percent);
    if (statusBand != TrashNetwork::lastPublishedStatusBand()) {
      // Gan V0 cho Gauge/Level va V1 cho Value Display/Label tren Dashboard.
      // Chi doi mau khi trang thai doi de tranh gui qua nhieu lenh Blynk.
      Blynk.setProperty(V0, "color", TrashNetwork::colorFor(percent));
      Blynk.setProperty(V1, "color", TrashNetwork::colorFor(percent));
      TrashNetwork::lastPublishedStatusBand() = statusBand;
    }
    TrashNetwork::lastPublishedPercent() = percent;
    TrashNetwork::lastPublishTime() = millis();
  }

  // Moi dot rac day chi gui mot thong bao. Chi cho phep gui lai sau khi
  // muc rac da giam xuong <= 90%, tranh spam khi cam bien dao dong quanh 95%.
  if (percent >= TrashNetwork::FULL_THRESHOLD &&
      !TrashNetwork::fullAlertSent()) {
    if (Blynk.connected()) {
      Blynk.logEvent("trash_full", "Thùng rác đã đầy 95%, vui lòng đi đổ rác ngay!");
      TrashNetwork::fullAlertSent() = true;
      Serial.println("Đã gửi cảnh báo thùng rác đầy.");
    } else {
      Serial.println("Chưa gửi cảnh báo: Blynk đang mất kết nối.");
    }
  } else if (percent <= TrashNetwork::ALERT_RESET_THRESHOLD) {
    TrashNetwork::fullAlertSent() = false;
  }
}

// Goi lien tuc trong loop() de xu ly Blynk va tu ket noi lai khi bi mat mang.
inline void runNetworkLoop() {
  Blynk.run();

  const uint32_t now = millis();
  if (Blynk.connected() ||
      now - TrashNetwork::lastReconnectAttempt() <
          TrashNetwork::RECONNECT_INTERVAL_MS) {
    return;
  }

  TrashNetwork::lastReconnectAttempt() = now;
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Dang ket noi lai Wi-Fi...");
  } else {
    Blynk.connect(1000);
    Serial.println("Dang ket noi lai Blynk...");
  }
}

// --- THÊM PHẦN LẮNG NGHE NÚT NHẤN BLYNK ---
namespace TrashNetwork {
  inline bool &blynkLidHoldRequested() {
    static bool requested = false;
    return requested;
  }
}

// Hàm này kích hoạt liên tục khi bạn chạm hoặc buông ngón tay khỏi app
BLYNK_WRITE(V2) {
  TrashNetwork::blynkLidHoldRequested() = (param.asInt() == 1);
  
  if (param.asInt() == 1) {
    Serial.println("Blynk: Đang NHẤN GIỮ nút trên điện thoại.");
  } else {
    Serial.println("Blynk: Đã BUÔNG ngón tay.");
  }
}

// Hàm xuất trạng thái ra bên ngoài
inline bool isBlynkHoldRequested() {
  return TrashNetwork::blynkLidHoldRequested();
}

#endif