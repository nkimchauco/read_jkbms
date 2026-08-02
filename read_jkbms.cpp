/*
 * read_jkbms — JK BMS BLE library (minimal)
 * 
 * Cách dùng:
 *   jkBMS.begin("c8:47:8c:10:1a:5b");
 *   if (jkBMS.read()) {
 *     // jkBMS.rawData[] + jkBMS.rawLen là dữ liệu thô từ BMS
 *     uint8_t ft = jkBMS.rawData[4]; // 0x01=settings, 0x02=cell data
 *     ...
 *   }
 */

#include "read_jkbms.h"

read_jkbms jkBMS;

uint8_t read_jkbms::_crc(uint8_t* data) {
  uint8_t c = 0;
  for (int i = 0; i < 19; i++) c += data[i];
  return c;
}
void read_jkbms::_buildFrame(uint8_t* f, uint8_t cmd) {
  memset(f, 0, 20);
  f[0] = 0xAA; f[1] = 0x55; f[2] = 0x90; f[3] = 0xEB;
  f[4] = cmd; f[19] = _crc(f);
}

// BLE scan
class _JKScan : public BLEAdvertisedDeviceCallbacks {
  BLEAdvertisedDevice*& _t; String _m;
public:
  _JKScan(BLEAdvertisedDevice*& t, const String& m) : _t(t), _m(m) {}
  void onResult(BLEAdvertisedDevice d) {
    if (strcmp(d.getAddress().toString().c_str(), _m.c_str()) == 0 && !_t)
      _t = new BLEAdvertisedDevice(d);
  }
};

// Notify handler — assemble frame
static read_jkbms* _bms = nullptr;
static uint8_t _fbuf[320]; static int _fidx = 0; static bool _fstart = false;

void read_jkbms::_notifyCb(esp_gattc_cb_event_t e, esp_gatt_if_t gif,
                           esp_ble_gattc_cb_param_t* p) {
  if (!_bms || e != ESP_GATTC_NOTIFY_EVT) return;
  uint8_t* d = p->notify.value; int len = p->notify.value_len;

  if (len >= 4 && d[0] == 0x55 && d[1] == 0xAA && d[2] == 0xEB && d[3] == 0x90) {
    _fidx = 0; _fstart = true;
  }
  if (_fstart) {
    for (int i = 0; i < len && _fidx < 320; i++) _fbuf[_fidx++] = d[i];
    // Frame complete: ~200 bytes for 24-cell BMS
    if (_fidx >= 190) {
      _fstart = false;
      memcpy(_bms->rawData, _fbuf, _fidx);
      _bms->rawLen = _fidx;
      _bms->_dataReady = true;
    }
  }
}

void read_jkbms::_write(uint8_t* f) {
  if (!_client || !_charHandle) return;
  esp_ble_gattc_write_char(_client->getGattcIf(), _client->getConnId(),
    _charHandle, 20, f, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
}

bool read_jkbms::begin(const char* mac) {
  _mac = mac;
  BLEDevice::init("JKBMS");
  BLEScan* s = BLEDevice::getScan();
  s->setAdvertisedDeviceCallbacks(new _JKScan(_target, _mac));
  s->setActiveScan(true); s->setInterval(100); s->setWindow(99);
  _target = nullptr;
  s->start(10, false);
  if (!_target) return false;

  _client = BLEDevice::createClient();
  if (!_client->connect(_target->getAddress())) return false;
  BLERemoteService* svc = _client->getService(BLEUUID("FFE0"));
  if (!svc) return false;
  _pChar = svc->getCharacteristic(BLEUUID("FFE1"));
  if (!_pChar) return false;
  _charHandle = _pChar->getHandle();

  _bms = this;
  esp_ble_gattc_register_callback(_notifyCb);
  esp_bd_addr_t bda; memcpy(bda, _target->getAddress().getNative(), 6);
  esp_ble_gattc_register_for_notify(_client->getGattcIf(), bda, _charHandle);

  connected = true;
  delay(500);
  uint8_t f[20];
  _buildFrame(f, 0x97); _write(f); delay(500);
  _buildFrame(f, 0x96); _write(f);
  _lastRequest = millis();
  return true;
}

bool read_jkbms::read() {
  if (_client && !_client->isConnected()) { connected = false; return false; }
  if (_dataReady) {
    _dataReady = false;
    _parse();
    return true;
  }
  if (connected && millis() - _lastRequest > 10000) {
    uint8_t f[20]; _buildFrame(f, 0x96); _write(f);
    _lastRequest = millis();
  }
  return false;
}

// Parse JK02_24S — ALL LITTLE-ENDIAN
#define LE32(o) ((int32_t)((rawData[o+3]<<24)|(rawData[o+2]<<16)|(rawData[o+1]<<8)|rawData[o]))

void read_jkbms::_parse() {
  if (rawLen < 4) return;
  frameType = rawData[4];

  if (frameType == 0x02 && rawLen >= 150) {
    // 24 cells × 2 bytes LE, bắt đầu từ offset 6
    for (int i = 0; i < 24; i++) {
      int o = 6 + i * 2;
      cellVoltage[i] = ((rawData[o+1] << 8) | rawData[o]) * 0.001f;
    }
    // V tổng = tổng các cell (BMS mắc nối tiếp) — chính xác hơn offset
    float sum = 0;
    for (int i = 0; i < 24; i++) sum += cellVoltage[i];
    totalVoltage = sum;

    int S = 0;
    power          = LE32(122+S) * 0.001f;
    current        = LE32(126+S) * 0.001f;
    capacityFull   = LE32(130+S) * 0.001f;
    capacityRemain = LE32(134+S) * 0.001f;
    balanceCurrent = LE32(78+S)  * 0.001f;
    // Temps: 16-bit LE
    // MOS temp @144 (verify: 37 01 = 31.1°C)
    mosfetTemp = (int16_t)((rawData[145]<<8)|rawData[144]) * 0.1f;
    // T1 @162, T2 @164 (verify: 2E 01 = 30.2°C, 30 01 = 30.4°C — sensor đã lắp)
    temp1 = (int16_t)((rawData[163]<<8)|rawData[162]) * 0.1f;
    temp2 = (int16_t)((rawData[165]<<8)|rawData[164]) * 0.1f;
    soc            = rawData[150+S];
    charging       = rawData[118+S] != 0 || rawData[119+S] != 0;
    discharging    = rawData[122+S] != 0 || rawData[123+S] != 0;
    balancing      = rawData[126+S] != 0 || rawData[127+S] != 0;
    cellCount      = 24;
  }
}
