#include "button.h"
#include "buzzer.h"
#include "esp_log.h"
#include <OneButton.h>

#define LONG_PRESS_DEBOUNCE_MS 800
static const char*   TAG               = "按钮";
static const uint8_t functionButtonPin = 33;
volatile bool        isBtnShortPressed = false;
volatile bool        isBtnLongPressed  = false;

static OneButton functionButton;

static void functionButtonShortPressed() {
  isBtnShortPressed = !isBtnShortPressed;
  ESP_LOGI(TAG, "短按");
  buzzer(1, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL);
}

static void functionButtonLongPressed() {
  isBtnLongPressed = true;
  ESP_LOGI(TAG, "长按");
  buzzer(1, LONG_BEEP_DURATION, LONG_BEEP_INTERVAL);
}

void buttonInit() {
  functionButton.setup(functionButtonPin, INPUT_PULLUP);
  functionButton.attachClick(functionButtonShortPressed);
  functionButton.attachLongPressStart(functionButtonLongPressed);
  functionButton.setPressMs(LONG_PRESS_DEBOUNCE_MS);
}

void buttonTask(void* pvParameters) {
  while (1) {
    functionButton.tick();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
