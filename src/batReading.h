#pragma once

#include <Arduino.h>

extern volatile float batPercentage, batVoltage;
extern volatile bool  isCharging, isBatteryFull;
extern TaskHandle_t batteryTaskHandle;

void batteryInit();
void batteryCheck(void* pvParameter);
void batteryChargeTask(void* pvParameter);
