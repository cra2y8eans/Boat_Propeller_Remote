#include "ESPNOW.h"
#include "buzzer.h"
#include "led.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));
  ledInit();
  buzzerInit();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}