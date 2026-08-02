#include <read_jkbms.h>

#define JK_MAC "c8:47:8c:10:1a:5b"

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== test_read_jkbms ===");

  // JK BMS
  if (!jkBMS.begin(JK_MAC)) { Serial.println("JK BMS FAILED"); return; }
  Serial.println("JK BMS Connected!");
}

void loop() {
  if (jkBMS.read()) {
    Serial.println("\n========== JK BMS ==========");
    Serial.printf("  Voltage:   %.2f V\n", jkBMS.totalVoltage);
    Serial.printf("  Current:   %.2f A\n", jkBMS.current);
    Serial.printf("  SOC:       %d %%\n", jkBMS.soc);
    Serial.printf("  Remain:    %.1f Ah\n", jkBMS.capacityRemain);
    Serial.printf("  MOS Temp:  %.1f C\n", jkBMS.mosfetTemp);
    Serial.printf("  T1/T2:     %.1f / %.1f C\n", jkBMS.temp1, jkBMS.temp2);
    Serial.printf("  Balance:   %.3f A\n", jkBMS.balanceCurrent);
    Serial.printf("  Charge:%d Dis:%d Bal:%d\n",
      jkBMS.charging, jkBMS.discharging, jkBMS.balancing);
    Serial.print("  Cells: ");
    for (int i = 0; i < jkBMS.cellCount; i++) {
      Serial.printf("%.3f ", jkBMS.cellVoltage[i]);
      if ((i+1)%8 == 0 && i+1 < jkBMS.cellCount) Serial.print("\n         ");
    }
    Serial.println("\n=============================");
  }
  delay(200);
}
