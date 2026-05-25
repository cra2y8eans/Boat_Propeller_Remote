#include "batReading.h"
#include "Filters.h"
#include "button.h"
#include "buzzer.h"
#include "esp_log.h"
#include "led.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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

static const uint8_t chargePin  = 2;            // 充电状态检测引脚，需外部上拉
static const uint8_t standbyPin = 8;            // 充满状态检测引脚，需外部上拉
volatile bool        isCharging, isBatteryFull; // 充电状态和电池满状态标志位

static BatteryState_e batteryState = BATTERY_STATE_IDLE;
static portMUX_TYPE   batteryMux   = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t batteryTaskHandle = NULL;

void IRAM_ATTR chargeStateChanged_ISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  isCharging                          = digitalRead(chargePin) == LOW;
  vTaskNotifyGiveFromISR(batteryTaskHandle, &xHigherPriorityTaskWoken); // 发送通知给任务，不携带任何值，并传入 xHigherPriorityTaskWoken 来指示是否需要切换任务
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR(); // 切换任务。注意与ESP32的写法有所不同
  }
}

// void IRAM_ATTR chargeStateChanged_ISR() {
//     isCharging = digitalRead(chargePin) == LOW;
//     vTaskNotifyGiveFromISR(batteryTaskHandle, NULL);
//     portYIELD_FROM_ISR();
// }

void IRAM_ATTR fullStateChanged_ISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  isBatteryFull                       = digitalRead(standbyPin) == LOW;
  vTaskNotifyGiveFromISR(batteryTaskHandle, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

/**  电量读取
 * @brief     上电/按键唤醒显示电量30秒后自动休眠，低于设定阈值后循环报警
 * @param     R1: 分压电阻R1阻值
 * @param     R2: 分压电阻R2阻值
 * @param     batPercentage: 电量百分比
 * @param     batVoltage: 电压值
 */
float batteryReading() {
  taskENTER_CRITICAL(&batteryMux);
  batVoltage    = (batFilter.update(analogReadMilliVolts(batPin))) * (R1 + R2) / R2;
  batPercentage = (batVoltage - VMIN) / (VMAX - VMIN) * 100.f;
  batPercentage = constrain(batPercentage, 0.0f, 100.0f); // 确保不为负数
  taskEXIT_CRITICAL(&batteryMux);
  return batPercentage / 100.f;
}

static BatteryState_e identifyBatteryState() {
  if (isCharging) {
    taskENTER_CRITICAL(&batteryMux);
    return BATTERY_STATE_CHARGING;
    taskEXIT_CRITICAL(&batteryMux);
  } else if (isBatteryFull) {
    taskENTER_CRITICAL(&batteryMux);
    return BATTERY_STATE_FULL;
    taskEXIT_CRITICAL(&batteryMux);
  } else {
    taskENTER_CRITICAL(&batteryMux);
    return BATTERY_STATE_IDLE;
    taskEXIT_CRITICAL(&batteryMux);
  }
}

//  电量读取任务
void batteryCheck(void* pvParameter) {
  // 初始状态读取和LED设置
  isCharging    = digitalRead(chargePin) == LOW;
  isBatteryFull = digitalRead(standbyPin) == LOW;
  batteryState  = identifyBatteryState();
  if (isCharging) {
    ledSetMode(batRGB, LED_ON, COLOR_RED, 0, 0);
  } else if (isBatteryFull) {
    ledSetMode(batRGB, LED_ON, COLOR_GREEN, 0, 0);
  }
  while (1) {
    batteryState = identifyBatteryState();

    // 电池空闲状态
    if (batteryState == BATTERY_STATE_IDLE) {
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
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void batteryChargeTask(void* pvParameter) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // 等待充电状态改变的通知
    batteryState = identifyBatteryState();
    switch (batteryState) {
    case BATTERY_STATE_CHARGING: {
      float batteryLevel = batteryReading();
      ledSetMode(batRGB, LED_ON, COLOR_RED, 0, 0); // 充电时亮红灯
      buzzer(1, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL);
      ESP_LOGI(TAG, "正在充电，电量: %.2f%%, 电压: %.2fV", batPercentage, batVoltage);
      break;
    }
    case BATTERY_STATE_FULL: {
      float batteryLevel = batteryReading();
      ledSetMode(batRGB, LED_ON, COLOR_GREEN, 0, 0); // 充满，亮绿灯
      buzzer(3, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL);
      ESP_LOGI(TAG, "电池已充满，电量: %.2f%%, 电压: %.2fV", batPercentage, batVoltage);
      break;
    }
    case BATTERY_STATE_IDLE: {
      float    batteryLevel = batteryReading();
      uint32_t color        = getBatteryColor(batPercentage);
      ledSetMode(batRGB, LED_ON, color, 0, 0); // 空闲状态根据电量显示颜色
      ESP_LOGI(TAG, "电池空闲，电量: %.2f%%, 电压: %.2fV", batPercentage, batVoltage);
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
  attachInterrupt(digitalPinToInterrupt(chargePin), chargeStateChanged_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(standbyPin), fullStateChanged_ISR, CHANGE);
}

bool isBatteryCharging() {
  taskENTER_CRITICAL(&batteryMux);
  bool charging = isCharging;
  taskEXIT_CRITICAL(&batteryMux);
  return charging;
}

bool isFullyCharged() {
  taskENTER_CRITICAL(&batteryMux);
  bool full = isBatteryFull;
  taskEXIT_CRITICAL(&batteryMux);
  return full;
}

float getBatteryLevel() {
  float level;
  taskENTER_CRITICAL(&batteryMux);
  level = batPercentage;
  taskEXIT_CRITICAL(&batteryMux);
  return level;
}

float getBatteryVoltage() {
  float voltage;
  taskENTER_CRITICAL(&batteryMux);
  voltage = batVoltage;
  taskEXIT_CRITICAL(&batteryMux);
  return voltage;
}