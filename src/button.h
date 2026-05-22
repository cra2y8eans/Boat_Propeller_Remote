#pragma once

#include <Arduino.h>

extern volatile bool isBtnShortPressed;
extern volatile bool isBtnLongPressed;

void buttonInit();
void buttonTask(void* pvParameters);
bool getBtnShortPressed();