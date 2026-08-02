# read_jkbms

Thư viện Arduino đọc dữ liệu từ **JK BMS** (JK-B2A24S20P, JK-BD4A20S4P...) qua **BLE** trên ESP32.

Được phát triển và verify trên bộ pin LiFePO4 24S (78V) với JK-B2A24S20P.

## Tính năng

- Đọc **24 cell voltages** (offset 6-53, 2 byte LE × 0.001V)
- V tổng = tổng 24 cell
- Current (offset 126, int32 LE × 0.001A)
- MOS temperature (offset 144, int16 LE × 0.1°C)
- Temperature sensor T1/T2 (offset 162/164, int16 LE × 0.1°C)
- SOC (offset 150, uint8 %)
- Raw data buffer đầy đủ cho ai muốn tự parse

## Cài đặt

Copy thư mục `read_jkbms` vào `~/Arduino/libraries/` (hoặc dùng symlink).

## Cách dùng tối thiểu

```cpp
#include <read_jkbms.h>

void setup() {
  Serial.begin(115200);
  jkBMS.begin("c8:47:8c:10:1a:5b");  // MAC của JK BMS
}

void loop() {
  if (jkBMS.read()) {
    Serial.printf("V = %.2f V\n", jkBMS.totalVoltage);
    for (int i = 0; i < 24; i++)
      Serial.printf("C%02d = %.3f V\n", i + 1, jkBMS.cellVoltage[i]);
    Serial.printf("MOS: %.1f C  T1: %.1f  T2: %.1f\n",
                  jkBMS.mosfetTemp, jkBMS.temp1, jkBMS.temp2);
  }
}
```

## Giao thức (JK02_24S)

- Frame request: `AA 55 90 EB [cmd] [00] [00*4] [00*9] [CRC]` (20 bytes)
- Response header: `55 AA EB 90`
- **CRC = SUM** bytes 0..18 (không phải XOR!)
- **Little-endian** cho mọi giá trị đa byte
- Commands: `0x96` cell info, `0x97` device info, `0xA1` logbook

## Lưu ý kỹ thuật (đã debug 2 ngày 😅)

- Arduino BLE wrapper không nhận notify từ JK BMS → dùng **native ESP-IDF API** (`esp_ble_gattc_register_for_notify`, `esp_ble_gattc_write_char`) kết hợp Arduino scan/connect
- Offset temp KHÁC tài liệu syssi: MOS @144, T1 @162, T2 @164 (syssi ghi 134/130/132 — sai với BMS này)

## Nguồn tham khảo & Lời cảm ơn 🙏

Thư viện này được phát triển dựa trên kiến thức của cộng đồng mã nguồn mở. Xin cảm ơn:

- **[syssi/esphome-jk-bms](https://github.com/syssi/esphome-jk-bms)** — tài liệu giao thức JK02, cấu trúc frame, cách parse dữ liệu
- **[peff74/jkbms](https://github.com/peff74/jkbms)** — thư viện JK BMS cho ESP32, tham khảo cách giao tiếp BLE (lưu ý: thư viện này dùng big-endian, JK BMS thật ra là little-endian)
- **[esphome/esphome](https://github.com/esphome/esphome)** — nền tảng IoT dùng để verify dữ liệu trước khi viết lib riêng
- Cộng đồng Arduino, ESP-IDF và các bài viết chia sẻ về JK BMS trên GitHub/forum

Đặc biệt cảm ơn anh **nkimchauco** — chủ bộ pin LiFePO4 24S đã kiên nhẫn test, chụp ảnh app đối chiếu và cùng debug suốt quá trình. Thư viện này được verify thực tế trên JK-B2A24S20P.

## License

MIT
