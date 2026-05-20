#include "batReading.h"
#include "Filters.h"
#include "buzzer.h"
#include "esp_log.h"
#include <button.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led.h>

#define K_OHM 1000
#define BATTERY_READING_INTERVAL 3 * 60 * 1000 // 采样间隔 3分钟*60秒*1000毫秒
#define BATTERY_ALERT_INTERVAL 3 * 10 * 1000   // 报警间隔 3分钟*10秒*1000毫秒

static const char*   TAG           = "电池";
static const uint8_t batPin        = 1;
static const float   R1            = 9.9 * K_OHM;
static const float   R2            = 9.9 * K_OHM;
static const float   VMAX          = 4.2 * K_OHM;
static const float   VMIN          = 3.2 * K_OHM;
volatile float       batVoltage    = 0.0;
volatile float       batPercentage = 0.0;

Filters::LowPass batFilter(0.3f);

TaskHandle_t         chargeTaskHandle = NULL;
static const uint8_t chargePin        = 2;      // 充电状态检测引脚，需外部上拉
static const uint8_t standbyPin       = 8;      // 充满状态检测引脚，需外部上拉
volatile bool        isCharging, isBatteryFull; // 充电状态和电池满状态标志位

enum BatteryState_e {
  BATTERY_STATE_IDLE,
  BATTERY_STATE_CHARGING,
  BATTERY_STATE_FULL,
};
static BatteryState_e batteryState = BATTERY_STATE_IDLE;

// 定义临界区变量
static portMUX_TYPE batteryMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR chargeStatus_ISR() {
  taskENTER_CRITICAL(&batteryMux);
  isCharging    = digitalRead(chargePin) == LOW;  // 充电状态引脚为低电平表示正在充电
  isBatteryFull = digitalRead(standbyPin) == LOW; // 充满状态引脚为低电平表示电池已充满
  taskEXIT_CRITICAL(&batteryMux);
}

/**  电量读取
 * @brief     上电/按键唤醒显示电量30秒后自动休眠，低于设定阈值后循环报警
 * @param     R1: 分压电阻R1阻值
 * @param     R2: 分压电阻R2阻值
 * @param     batPercentage: 电量百分比
 * @param     batVoltage: 电压值
 */
float batteryReading() {
  batVoltage    = (batFilter.update(analogReadMilliVolts(batPin))) * (R1 + R2) / R2;
  batPercentage = (batVoltage - VMIN) / (VMAX - VMIN) * 100.f;
  batPercentage = constrain(batPercentage, 0.0f, 100.0f); // 确保不为负数
  return batPercentage / 100.f;
}

BatteryState_e getBatteryState() {
  if (isCharging) {
    return BATTERY_STATE_CHARGING;
  } else if (isBatteryFull) {
    return BATTERY_STATE_FULL;
  } else {
    return BATTERY_STATE_IDLE;
  }
}

//  电量读取任务
void batteryCheck(void* pvParameter) {
  static bool isBatteryChargeBeep = false;
  static bool isBatteryFullBeep   = false;
  while (1) {
    batteryState = getBatteryState();
    switch (batteryState) {
    case BATTERY_STATE_CHARGING: {
      float    batteryLevel = batteryReading();
      uint32_t color        = getBatteryColor(batPercentage);
      ledSetMode(batRGB, LED_ON, color, 0, 0);
      if (!isBatteryChargeBeep) {
        buzzer(1, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL); // 开始充电时鸣叫一次
        isBatteryChargeBeep = true;
      }
      ESP_LOGI(TAG, "正在充电，电量: %.2f%%, 电压: %.2fV", batPercentage, batVoltage);
      vTaskDelay(pdMS_TO_TICKS(2000)); // 充电状态下每2秒更新一次电量显示
      break;
    }
    case BATTERY_STATE_FULL:
      ledSetMode(batRGB, LED_ON, COLOR_GREEN, 0, 0);
      if (!isBatteryFullBeep) {
        buzzer(3, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL); // 充满时鸣叫两次
        isBatteryFullBeep = true;
      }
      ESP_LOGI(TAG, "电池已充满，电量: %.2f%%, 电压: %.2fV", batPercentage, batVoltage);
      break;
    case BATTERY_STATE_IDLE: {
      // 电量指示，颜色根据电量百分比动态调整
      float    batteryLevel = batteryReading();
      uint32_t color        = getBatteryColor(batPercentage);
      ledSetMode(batRGB, LED_ON, color, 0, 0);
      // 低电量报警
      if (batPercentage <= 10.0f) {
        static unsigned long lastAlertTime = 0;
        if (!isBtnLongPressed && millis() - lastAlertTime > BATTERY_ALERT_INTERVAL) { // isBtnLongPressed初始值为false，低电量报警默认开启
          ESP_LOGE(TAG, "电量过低，请及时充电");
          buzzer(3, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL);
          lastAlertTime = millis();
        }
      }
      // 根据电量动态调整读取频率，电量越低读取越频繁，最高每秒一次，最低每10秒一次
      float interval = batteryLevel * BATTERY_READING_INTERVAL;
      if (interval > BATTERY_READING_INTERVAL) interval = BATTERY_READING_INTERVAL;
      if (interval < 1000) interval = 1000;
      vTaskDelay(interval / portTICK_PERIOD_MS);
      break;
    }

    default:
      break;
    }
  }
}

void batteryInit() {
  analogReadResolution(12);
  pinMode(chargePin, INPUT);  // 已外部上拉
  pinMode(standbyPin, INPUT); // 已外部上拉
  isCharging    = digitalRead(chargePin) == LOW;
  isBatteryFull = digitalRead(standbyPin) == LOW;
  attachInterrupt(digitalPinToInterrupt(chargePin), chargeStatus_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(standbyPin), chargeStatus_ISR, CHANGE);
  batteryState = getBatteryState();
}