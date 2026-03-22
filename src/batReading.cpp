#include "batReading.h"
#include "Filters.h"
#include "buzzer.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define K_OHM 1000
#define BATTERY_READING_INTERVAL 3 * 60 * 1000 // 采样间隔 3分钟*60秒*1000毫秒

static const char*   TAG           = "电池";
static const uint8_t batPin        = 39;
static const float   R1            = 9.9 * K_OHM;
static const float   R2            = 9.9 * K_OHM;
static const float   VMAX          = 4.2 * K_OHM;
static const float   VMIN          = 3.2 * K_OHM;
volatile float       batVoltage    = 0.0;
volatile float       batPercentage = 0.0;

volatile BatteryState_e batteryState = FULL;

Filters::LowPass filter(0.3f);

void batteryInit() {
}

/**  电量读取
 * @brief     上电/按键唤醒显示电量30秒后自动休眠，低于设定阈值后循环报警
 * @param     R1: 分压电阻R1阻值
 * @param     R2: 分压电阻R2阻值
 * @param     batPercentage: 电量百分比
 * @param     batVoltage: 电压值
 */
void bateryReading() {
  batVoltage    = (filter.update(analogReadMilliVolts(batPin))) * (R1 + R2) / R2;
  batPercentage = (batVoltage - VMIN) / (VMAX - VMIN) * 100;
  if (batPercentage >= 80) {
    batteryState = FULL;
  } else if (batPercentage >= 60) {
    batteryState = DECENT;
  } else if (batPercentage >= 40) {
    batteryState = MODERATE;
  } else if (batPercentage >= 20) {
    batteryState = DEPLETED;
  } else {
    batteryState = CRITICAL;
    ESP_LOGE(TAG, "电量过低，请及时充电");
    buzzer(3, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL);
  }
}

//  电量读取任务
void batteryCheck(void* pvParameter) {
  while (1) {
    bateryReading();
    vTaskDelay(batPercentage * BATTERY_READING_INTERVAL / portTICK_PERIOD_MS); // 电量越低读取越频繁
  }
}