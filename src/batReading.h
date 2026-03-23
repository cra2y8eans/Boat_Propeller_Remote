#pragma once

#include <Arduino.h>

enum BatteryState_e {
  FULL,     // 100% - 80%
  DECENT,   // 80%  - 60%
  MODERATE, // 60%  - 40%
  DEPLETED, // 40%  - 20%
  CRITICAL  // 20%  - 0%
};
extern volatile BatteryState_e batteryState;

extern volatile float batPercentage, batVoltage;


void batteryInit();
void batteryCheck(void* pvParameter);