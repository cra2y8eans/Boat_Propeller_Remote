#pragma once

#include <Arduino.h>

enum BatteryState_e {
  BATTERY_STATE_CHARGING,
  BATTERY_STATE_FULL,
  BATTERY_STATE_IDLE,
};

extern TaskHandle_t batteryTaskHandle;

void  batteryInit();
void  batteryCheck(void* pvParameter);
void  batteryChargeTask(void* pvParameter);
bool  isBatteryCharging();
bool  isFullyCharged();
float getBatteryLevel();
float getBatteryVoltage();
