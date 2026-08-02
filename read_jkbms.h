/*
 * read_jkbms - JK BMS BLE communication library (minimal)
 * 
 * Cách dùng:
 *   jkBMS.begin("c8:47:8c:10:1a:5b");
 *   if (jkBMS.read()) {
 *     // rawData[0..rawLen-1] = dữ liệu thô, anh tự parse
 *   }
 */

#ifndef READ_JKBMS_H
#define READ_JKBMS_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "esp_gattc_api.h"

class read_jkbms {
public:
  bool connected = false;

  // Raw data buffer
  uint8_t rawData[320];
  uint16_t rawLen = 0;

  // Parsed data
  float cellVoltage[24] = {0};
  float totalVoltage    = 0;
  float current         = 0;
  float power           = 0;
  int   soc             = 0;
  float capacityRemain  = 0;
  float capacityFull    = 0;
  float balanceCurrent  = 0;
  float mosfetTemp      = 0;
  float temp1           = 0;
  float temp2           = 0;
  int   cellCount       = 0;
  bool  charging        = false;
  bool  discharging     = false;
  bool  balancing       = false;
  uint8_t frameType     = 0;

  // Kết nối BLE tới JK BMS
  bool begin(const char* macAddress);

  // Trả về true khi có data mới trong rawData[]
  bool read();

  // Internal (public cho callback)
  bool _dataReady = false;

private:
  String _mac;
  BLEClient* _client = nullptr;
  BLERemoteCharacteristic* _pChar = nullptr;
  BLEAdvertisedDevice* _target = nullptr;
  uint16_t _charHandle = 0;
  unsigned long _lastRequest = 0;

  static void _notifyCb(esp_gattc_cb_event_t e, esp_gatt_if_t gif,
                        esp_ble_gattc_cb_param_t* p);
  static uint8_t _crc(uint8_t* data);
  static void _buildFrame(uint8_t* f, uint8_t cmd);
  void _write(uint8_t* f);
  void _parse();
};

extern read_jkbms jkBMS;

#endif
