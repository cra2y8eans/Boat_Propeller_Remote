#include "led.h"
#include "ESPNOW.h"
#include "batReading.h"
#include "button.h"
#include <Adafruit_NeoPixel.h>
#include <esp_log.h>

#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0
#define STANDARD_BRIGHTNESS 100
#define BATTERY_LED_INTERVAL 10000 // 指示灯休眠间隔, 单位: 毫秒

static const char*       TAG        = "LED";
static const uint8_t     sysRGB_pin = 18;
static const uint8_t     batRGB_pin = 19;
static Adafruit_NeoPixel sysRGB(1, sysRGB_pin, NEO_GRB + NEO_KHZ800);
static Adafruit_NeoPixel batLED(1, batRGB_pin, NEO_GRB + NEO_KHZ800);

void ledInit() {
  sysRGB.begin();
  sysRGB.setBrightness(STANDARD_BRIGHTNESS);
  sysRGB.clear();
  batLED.begin();
  batLED.setBrightness(STANDARD_BRIGHTNESS);
  batLED.clear();
}

void ledTask(void* pvParameters) {
  unsigned long lastBatteryLEDTime = 0;
  while (1) {
    if (isBtnLongPressed) {
      unsigned long currentTime = millis();
      if (currentTime - lastBatteryLEDTime < BATTERY_LED_INTERVAL) {
        sysRGB.clear();
        switch (batteryState) {
        case FULL:
          sysRGB.setPixelColor(0, COLOR_CYAN);
          break;
        case DECENT:
          sysRGB.setPixelColor(0, COLOR_GREEN);
          break;
        case MODERATE:
          sysRGB.setPixelColor(0, COLOR_YELLOW);
          break;
        case DEPLETED:
          sysRGB.setPixelColor(0, COLOR_RED);
          break;
        default:
          break;
        }
      } else {
        sysRGB.clear();
        lastBatteryLEDTime = currentTime;
        isBtnLongPressed   = false;
      }
    } else {
      if (isMotorOnline) {
        sysRGB.clear();
        sysRGB.setPixelColor(0, COLOR_BLUE);
      } else {
        sysRGB.clear();
        sysRGB.setPixelColor(0, COLOR_RED);
      }
    }
    sysRGB.show();
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}