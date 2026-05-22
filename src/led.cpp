#include "led.h"
#include "ESPNOW.h"
#include "batReading.h"
#include "button.h"
#include <esp_log.h>

#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0
#define STANDARD_BRIGHTNESS 100

static const char*   TAG        = "LED";
static const uint8_t sysRGB_pin = 18;
static const uint8_t batRGB_pin = 19;
Adafruit_NeoPixel    sysRGB(1, sysRGB_pin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel    batRGB(1, batRGB_pin, NEO_GRB + NEO_KHZ800);

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

static LedController sysRGBCtrl = { LED_STATE_IDLE, 0, 0, 0, 0 };
static LedController batRGBCtrl = { LED_STATE_IDLE, 0, 0, 0, 0 };

void ledInit() {
  sysRGB.begin();
  sysRGB.setBrightness(STANDARD_BRIGHTNESS);
  sysRGB.clear();
  batRGB.begin();
  batRGB.setBrightness(STANDARD_BRIGHTNESS);
  batRGB.clear();
}

uint32_t getBatteryColor(float percent) {
  // 限制范围
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  // 定义关键颜色点 (百分比, R, G, B)
  struct ColorPoint {
    float   pct;
    uint8_t r, g, b;
  };
  const ColorPoint points[] = {
    { 100.0, 0, 0, 255 },  // 蓝
    { 80.0, 0, 255, 255 }, // 青
    { 60.0, 0, 255, 0 },   // 绿
    { 40.0, 255, 255, 0 }, // 黄
    { 20.0, 255, 165, 0 }, // 橙
    { 10.0, 255, 0, 0 }    // 红
  };
  const int numPoints = sizeof(points) / sizeof(points[0]);

  // 低于最低点（10%）直接返回红色
  if (percent <= points[numPoints - 1].pct) {
    return (0xFF << 16) | (0x00 << 8) | 0x00;
  }

  // 查找所在区间并线性插值
  for (int i = 0; i < numPoints - 1; i++) {
    if (percent >= points[i + 1].pct && percent <= points[i].pct) {
      float   t = (percent - points[i + 1].pct) / (points[i].pct - points[i + 1].pct);
      uint8_t r = points[i + 1].r + t * (points[i].r - points[i + 1].r);
      uint8_t g = points[i + 1].g + t * (points[i].g - points[i + 1].g);
      uint8_t b = points[i + 1].b + t * (points[i].b - points[i + 1].b);
      return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
  }
  return COLOR_OFF; // fallback
}

/**
 * @brief 设置LED模式（非阻塞）
 */
void ledSetMode(Adafruit_NeoPixel& myRGB, enum LEDMode mode, uint32_t color, uint16_t duration, uint16_t interval) {
  LedController* ctrl = (&myRGB == &sysRGB) ? &sysRGBCtrl : &batRGBCtrl;

  ctrl->color    = color;
  ctrl->duration = duration;
  ctrl->interval = interval;

  switch (mode) {
  case LED_ON:
    myRGB.clear();
    myRGB.setPixelColor(0, color);
    myRGB.show();
    ctrl->state = LED_STATE_IDLE;
    break;

  case LED_OFF:
    myRGB.clear();
    myRGB.show();
    ctrl->state = LED_STATE_IDLE;
    break;

  case LED_BLINK:
    ctrl->state          = LED_STATE_BLINK_ON;
    ctrl->stateStartTime = millis();
    myRGB.clear();
    myRGB.setPixelColor(0, color);
    myRGB.show();
    break;

  case LED_IDLE:
    ctrl->state = LED_STATE_IDLE;
    break;
  }
}

/**
 * @brief LED状态机更新函数（非阻塞）
 */
void ledUpdate(void* pvParameter) {
  while (1) {
    // 更新系统LED
    LedController* sysCtrl        = &sysRGBCtrl;
    uint32_t       sysElapsedTime = millis() - sysCtrl->stateStartTime;

    switch (sysCtrl->state) {
    case LED_STATE_BLINK_ON:
      if (sysElapsedTime >= sysCtrl->duration) {
        sysRGB.clear();
        sysRGB.show();
        sysCtrl->state          = LED_STATE_BLINK_OFF;
        sysCtrl->stateStartTime = millis();
      }
      break;
    case LED_STATE_BLINK_OFF:
      if (sysElapsedTime >= sysCtrl->interval) {
        sysRGB.clear();
        sysRGB.setPixelColor(0, sysCtrl->color);
        sysRGB.show();
        sysCtrl->state          = LED_STATE_BLINK_ON;
        sysCtrl->stateStartTime = millis();
      }
      break;

    default:
      break;
    }
    // 更新电池LED
    LedController* batCtrl        = &batRGBCtrl;
    uint32_t       batElapsedTime = millis() - batCtrl->stateStartTime;

    switch (batCtrl->state) {
    case LED_STATE_BLINK_ON:
      if (batElapsedTime >= batCtrl->duration) {
        batRGB.clear();
        batRGB.show();
        batCtrl->state          = LED_STATE_BLINK_OFF;
        batCtrl->stateStartTime = millis();
      }
      break;
    case LED_STATE_BLINK_OFF:
      if (batElapsedTime >= batCtrl->interval) {
        batRGB.clear();
        batRGB.setPixelColor(0, batCtrl->color);
        batRGB.show();
        batCtrl->state          = LED_STATE_BLINK_ON;
        batCtrl->stateStartTime = millis();
      }
      break;
    default:
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
