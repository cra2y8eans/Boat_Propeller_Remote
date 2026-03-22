#include "led.h"

#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0
#define STANDARD_BRIGHTNESS 100

static const char* TAG = "LED";

// ===== 硬件配置 =====
static const uint8_t WS2812_PIN = 17;

// ===== LED对象 =====
Adafruit_NeoPixel ws2812(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// ===== LED状态机 =====
typedef enum {
  LED_STATE_IDLE,
  LED_STATE_BLINK_ON,
  LED_STATE_BLINK_OFF,
} LedState;

struct LedController {
  LedState state;
  uint32_t stateStartTime;
  uint32_t color;
  uint16_t duration;
  uint16_t interval;
};

static LedController ledCtrl = { LED_STATE_IDLE, 0, 0, 0, 0 };

// ===== 公共接口 =====

void ledInit() {
  ws2812.begin();
  ws2812.setBrightness(STANDARD_BRIGHTNESS);
  ws2812.clear();
}

void ledSetMode(Adafruit_NeoPixel& myRGB, enum LEDMode mode, uint32_t color, uint16_t duration, uint16_t interval) {
  // 只允许操作全局的 ws2812 对象
  if (&myRGB != &ws2812) return;

  ledCtrl.color    = color;
  ledCtrl.duration = duration;
  ledCtrl.interval = interval;

  switch (mode) {
  case LED_ON:
    myRGB.clear();
    myRGB.setPixelColor(0, color);
    myRGB.show();
    ledCtrl.state = LED_STATE_IDLE;
    break;

  case LED_OFF:
    myRGB.clear();
    myRGB.show();
    ledCtrl.state = LED_STATE_IDLE;
    break;

  case LED_BLINK:
    ledCtrl.state          = LED_STATE_BLINK_ON;
    ledCtrl.stateStartTime = millis();
    myRGB.clear();
    myRGB.setPixelColor(0, color);
    myRGB.show();
    break;

  case LED_IDLE:
    ledCtrl.state = LED_STATE_IDLE;
    break;
  }
}

void ledUpdate(void* pvParameter) {
  uint32_t lastCheck = 0;
  while (1) {
    // 每5秒检查栈水位
    if (millis() - lastCheck > 5000) {
      UBaseType_t stackHighWater = uxTaskGetStackHighWaterMark(NULL);
      ESP_LOGI(TAG, "Stack left: %d words", stackHighWater);
      lastCheck = millis();
    }

    uint32_t elapsedTime = millis() - ledCtrl.stateStartTime;

    switch (ledCtrl.state) {
    case LED_STATE_BLINK_ON:
      if (elapsedTime >= ledCtrl.duration) {
        ws2812.clear();
        ws2812.show();
        ledCtrl.state          = LED_STATE_BLINK_OFF;
        ledCtrl.stateStartTime = millis();
      }
      break;

    case LED_STATE_BLINK_OFF:
      if (elapsedTime >= ledCtrl.interval) {
        ws2812.clear();
        ws2812.setPixelColor(0, ledCtrl.color);
        ws2812.show();
        ledCtrl.state          = LED_STATE_BLINK_ON;
        ledCtrl.stateStartTime = millis();
      }
      break;

    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}