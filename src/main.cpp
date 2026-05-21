#include "ESPNOW.h"
#include "batReading.h"
#include "button.h"
#include "buzzer.h"
#include "esp_log.h"
#include "led.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void setup() {
  Serial.begin(115200);
  esp_now_setup();
  ledInit();
  buzzerInit();
  buttonInit();
  xTaskCreatePinnedToCore(batteryChargeTask, "batteryChargeTask", 1024 * 10, NULL, 3, &batteryTaskHandle, 1);
  batteryInit();

  xTaskCreatePinnedToCore(buzzerUpdate, "buzzerUpdate", 1024 * 10, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(ledUpdate, "ledUpdate", 1024 * 10, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sendData, "sendData", 1024 * 10, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(connection_state_check, "esp_now_connection_check", 1024 * 10, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(buttonTask, "buttonTask", 1024 * 10, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(batteryCheck, "batteryTask", 1024 * 10, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}