#include "buzzer.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "蜂鸣器";

// ===== 硬件配置 =====
static uint8_t buzzerPin = 10;

// ===== 状态机定义 =====
typedef enum {
  BUZZER_IDLE,
  BUZZER_ON,
  BUZZER_OFF
} BuzzerState;

// ===== 状态机变量 =====
static volatile BuzzerState buzzerState     = BUZZER_IDLE;
static volatile uint32_t    stateStartTime  = 0;
static volatile uint8_t     remainingBeeps  = 0;
static volatile uint16_t    currentDuration = 0;
static volatile uint16_t    currentInterval = 0;

// ===== 互斥锁（临界区） =====
static portMUX_TYPE buzzerMux = portMUX_INITIALIZER_UNLOCKED;

// ===== 公共接口函数 =====

void buzzerInit() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
}

void buzzer(uint8_t times, uint16_t duration, uint16_t interval) {
  if (times == 0) return;

  taskENTER_CRITICAL(&buzzerMux);
  remainingBeeps  = times;
  currentDuration = duration;
  currentInterval = (times == 1) ? 0 : interval;
  buzzerState     = BUZZER_ON;
  stateStartTime  = millis();
  taskEXIT_CRITICAL(&buzzerMux);

  digitalWrite(buzzerPin, HIGH);
}

void buzzerUpdate(void* pvParameter) {
  while (1) {
    if (buzzerState == BUZZER_IDLE) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    uint32_t currentTime = millis();
    uint32_t elapsedTime;

    // 读取状态快照（加锁保护）
    taskENTER_CRITICAL(&buzzerMux);
    BuzzerState state = buzzerState;
    uint32_t    start = stateStartTime;
    uint16_t    dur   = currentDuration;
    uint16_t    inter = currentInterval;
    uint8_t     beeps = remainingBeeps;
    taskEXIT_CRITICAL(&buzzerMux);

    elapsedTime = currentTime - start;

    switch (state) {
    case BUZZER_ON:
      if (elapsedTime >= dur) {
        digitalWrite(buzzerPin, LOW);
        taskENTER_CRITICAL(&buzzerMux);
        remainingBeeps--;
        if (remainingBeeps > 0 && inter > 0) {
          buzzerState    = BUZZER_OFF;
          stateStartTime = currentTime;
        } else {
          buzzerState = BUZZER_IDLE;
        }
        taskEXIT_CRITICAL(&buzzerMux);
      }
      break;

    case BUZZER_OFF:
      if (elapsedTime >= inter) {
        taskENTER_CRITICAL(&buzzerMux);
        buzzerState    = BUZZER_ON;
        stateStartTime = currentTime;
        taskEXIT_CRITICAL(&buzzerMux);
        digitalWrite(buzzerPin, HIGH);
      }
      break;

    default:
      taskENTER_CRITICAL(&buzzerMux);
      buzzerState = BUZZER_IDLE;
      taskEXIT_CRITICAL(&buzzerMux);
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}