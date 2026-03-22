#include "led.h"
#include "ESPNOW.h"
#include "batReading.h"
#include "button.h"
#include <esp_log.h>

#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0
#define STANDARD_BRIGHTNESS 100
#define BATTERY_LED_INTERVAL 10000 // 指示灯休眠间隔, 单位: 毫秒

static const char*       TAG        = "LED";
static const uint8_t     WS2812_PIN = 17;
static Adafruit_NeoPixel ws2812(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);

void ledInit() {
  ws2812.begin();
  ws2812.setBrightness(STANDARD_BRIGHTNESS);
  ws2812.clear();
}

void ledTask(void* pvParameters) {
  unsigned long lastBatteryLEDTime = 0;
  while (1) {
    if (isBtnLongPressed) {
      unsigned long currentTime = millis();
      if (currentTime - lastBatteryLEDTime < BATTERY_LED_INTERVAL) {
        ws2812.clear();
        switch (batteryState) {
        case FULL:
          ws2812.setPixelColor(0, COLOR_CYAN);
          break;
        case DECENT:
          ws2812.setPixelColor(0, COLOR_GREEN);
          break;
        case MODERATE:
          ws2812.setPixelColor(0, COLOR_YELLOW);
          break;
        case DEPLETED:
          ws2812.setPixelColor(0, COLOR_RED);
          break;
        default:
          break;
        }
      } else {
        ws2812.clear();
        lastBatteryLEDTime = currentTime;
        isBtnLongPressed   = false;
      }
    } else {
      if (isMotorOnline) {
        ws2812.clear();
        ws2812.setPixelColor(0, COLOR_BLUE);
      } else {
        ws2812.clear();
        ws2812.setPixelColor(0, COLOR_RED);
      }
    }
    ws2812.show();
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}