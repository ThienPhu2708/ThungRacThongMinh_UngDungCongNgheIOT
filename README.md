# 🗑️ Đồ Án: Thùng Rác Thông Minh IoT (ESP32)

Dự án "Thùng rác thông minh" ứng dụng vi điều khiển ESP32, cho phép tự động đóng/mở nắp, giám sát lượng rác tự động, hiển thị thông tin trực quan và theo dõi, cảnh báo từ xa thông qua nền tảng đám mây Blynk.

## 🌟 Các chức năng chính (Đã tích hợp cơ chế chống nhiễu)

1. **Mở/đóng nắp tự động (Cảm biến hiện diện):** 
   * Nhận diện người dùng tiến vào vùng khoảng cách **< 20cm**.
   * Cần nhận diện thành công ít nhất **2 mẫu đo** liên tiếp để loại bỏ nhiễu sóng âm, tránh mở nắp nhầm.
   * Khi kích hoạt, Servo SG90 mở nắp vuông góc 90 độ và duy trì trong đúng **3 giây** kể từ lần cuối phát hiện người.
   * Cơ chế Hysteresis: Hệ thống chỉ được tái kích hoạt (rearm) khi người dùng đã bước ra xa khỏi ngưỡng **> 30cm** (giúp nắp không bị đóng/mở liên tục khi người dùng đứng yên ở ranh giới).

2. **Giám sát mức rác (Cảm biến đo chiều sâu):** 
   * Đo khoảng cách từ nắp đến bề mặt rác (chiều cao thùng mặc định cấu hình là **100cm**).
   * Áp dụng thuật toán lọc trung vị (Median Filter) từ **5 mẫu đo** liên tiếp để loại bỏ các điểm mù (dưới 2cm) và làm mịn dữ liệu.

3. **Hiển thị thông số LCD 16x2:** 
   * Cập nhật thời gian thực không giật lag mỗi **200ms**.
   * Hiển thị rõ phần trăm rác hiện tại và trạng thái nắp. Khi rác đầy, LCD sẽ chớp nháy luân phiên dòng chữ "THUNG DA DAY" mỗi **500ms**.

4. **Quản lý nút nhấn đa năng (Chống nhiễu phần mềm):** 
   * **Nhấn giữ (>= 800ms):** Bắt buộc duy trì trạng thái mở nắp vĩnh viễn (thuận tiện khi thay túi rác).
   * **Nhấn nhả (Reset):** Xác nhận đã dọn rác. Tính năng này được bảo vệ kép, chỉ thực thi thành công nếu cảm biến bên trong xác nhận thùng thực sự đã rỗng (**mức rác <= 5%**).

5. **Giám sát đám mây (Blynk Dashboard):** 
   * Đồng bộ % rác và phân loại hiển thị theo 3 dải màu: Rỗng (Xanh lá), Gần đầy (>= 70% - Màu cam), Cần đổ rác (>= 95% - Màu đỏ).

6. **Cảnh báo Push Notification thông minh:** 
   * Gửi thông báo đẩy ngay lập tức khi rác đạt ngưỡng báo động **>= 95%**.
   * Tích hợp ngưỡng hạ nhiệt: Ngừng gửi cảnh báo lặp lại cho đến khi mức rác thực sự tụt xuống **<= 90%** (tránh spam điện thoại khi rác dao động quanh mức 95%).

---

## 📁 Cấu trúc thư mục

Dự án được phân chia thành các module (namespace) độc lập để tối ưu hóa việc quản lý luồng dữ liệu và chống nhiễu:

```text
📦 ThungRacThongMinh_IoT
 ┣ 📂 src
 │ ┣ 📜 main.cpp               # Tệp chạy chính, điều phối đa luồng và các module
 │ ┣ 📜 auto_lid.h             # Xử lý cảm biến hiện diện và logic mở/đóng Servo
 │ ┣ 📜 display_ui.h           # Điều khiển LCD I2C và quản lý trạng thái nút nhấn
 │ ┣ 📜 network.h              # Kết nối Wi-Fi và đồng bộ dữ liệu với máy chủ Blynk
 │ ┣ 📜 sensor_motor.h         # Đo lường % rác và áp dụng thuật toán lọc nhiễu trung vị
 │ ┗ 📜 ultrasonic_scheduler.h # Lịch trình thời gian, chống xung đột sóng âm 2 cảm biến
 ┣ 📜 diagram.json             # Cấu hình dây nối phần cứng dành cho Wokwi Simulator
 ┣ 📜 libraries.txt            # Danh sách thư viện phụ thuộc (Blynk, LiquidCrystal I2C...)
 ┗ 📜 README.md                # Tài liệu hướng dẫn dự án


 ---
 