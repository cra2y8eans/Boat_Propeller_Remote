#pragma once

#include <Arduino.h>

extern volatile float batPercentage, batVoltage;
extern volatile bool  isCharging, isBatteryFull;

void batteryInit();
void batteryCheck(void* pvParameter);