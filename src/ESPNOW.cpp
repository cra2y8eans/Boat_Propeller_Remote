#include "ESPNOW.h"
#include "Filters.h"
#include "batReading.h"
#include "button.h"
#include "buzzer.h"
#include "led.h"
#include <WiFi.h>
#include <esp_log.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LOW_PASS_ALPHA 0.3f
#define RECV_TIMEOUT 500

// 定义临界区变量
static portMUX_TYPE esp_now_Mux = portMUX_INITIALIZER_UNLOCKED;

static esp_now_peer_info_t Remote;

static const char*            TAG               = "ESPNOW";
static const uint8_t          motorMacAddr[6]   = { 0xd8, 0x85, 0xac, 0xa0, 0x49, 0xd8 };
static const uint8_t          speedPin          = 0;
static const uint8_t          turnRightPin      = 7;
static const uint8_t          turnLeftPin       = 3;
static const uint8_t          throttlePin       = 5;
static volatile unsigned long lastRecvFromMotor = 0;
volatile bool                 isMotorOnline     = false;

static sendToMotor_t sendToMotor;

volatile recvFromMotor_e recv_from_motor;
static Filters::LowPass  speedFilter(LOW_PASS_ALPHA);

static void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  taskENTER_CRITICAL(&esp_now_Mux);
  memcpy((void*)&recv_from_motor, incomingData, sizeof(recv_from_motor));
  isMotorOnline     = true; // 如果是脚控发来的数据，说明脚控在线
  lastRecvFromMotor = millis();
  taskEXIT_CRITICAL(&esp_now_Mux);
}

// 发送回调
static void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
}

void esp_now_setup() {
  pinMode(turnRightPin, INPUT_PULLUP);
  pinMode(turnLeftPin, INPUT_PULLUP);
  pinMode(throttlePin, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // 设置较低的发射功率以降低MCU功耗和电磁干扰
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(TAG, "ESP NOW 初始化失败");
    buzzer(3, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL);
    return;
  } else {
    ESP_LOGI(TAG, "ESP NOW 初始化成功");
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  memcpy(Remote.peer_addr, motorMacAddr, 6);
  Remote.channel = 1;
  esp_now_add_peer(&Remote); // 添加脚控对等节点
}

void connection_state_check(void* pvParameters) {
  TickType_t       xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod       = pdMS_TO_TICKS(200); // 延时 100ms，频率 = 1000 / 100 = 10 Hz，即每秒执行 10 次。
  static bool      last_state    = false;
  while (1) {
    unsigned long currentTime = millis();
    isMotorOnline             = (currentTime - lastRecvFromMotor <= RECV_TIMEOUT);
    if (isMotorOnline != last_state) {
      last_state = isMotorOnline;
      if (!isMotorOnline) {
        ledSetMode(sysRGB, LED_BLINK, COLOR_RED, SHORT_FLASH_DURATION, SHORT_FLASH_INTERVAL);
        buzzer(3, SHORT_BEEP_DURATION, SHORT_BEEP_INTERVAL);
        ESP_LOGE(TAG, "掉线！请检查网络连接！");
      } else {
        ledSetMode(sysRGB, LED_ON, COLOR_BLUE, 0, 0);
        ESP_LOGI(TAG, "已连接");
      }
    }
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}

void sendData(void* pvParameters) {
  TickType_t       xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod       = pdMS_TO_TICKS(20); // 延时 20ms，频率 = 1000 / 20 = 50 Hz，即每秒执行 50 次。
  while (1) {                                         // 每 1000 次循环或每 5 秒检查一次栈水位
    sendToMotor.speed           = speedFilter.update(analogRead(speedPin));
    sendToMotor.data[0]         = !digitalRead(turnLeftPin);
    sendToMotor.data[1]         = !digitalRead(turnRightPin);
    sendToMotor.data[2]         = !digitalRead(throttlePin);
    sendToMotor.data[3]         = getBtnShortPressed();
    sendToMotor.data[4]         = isBatteryCharging();
    sendToMotor.data[5]         = isFullyCharged();
    sendToMotor.batVoltage      = getBatteryVoltage();
    sendToMotor.batPercentage   = getBatteryLevel();
    sendToMotor.footPadChipTemp = temperatureRead();
    esp_now_send(motorMacAddr, (uint8_t*)&sendToMotor, sizeof(sendToMotor));
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}

recvFromMotor_e getMotormode() {
  taskENTER_CRITICAL(&esp_now_Mux);
  recvFromMotor_e mode = recv_from_motor;
  taskEXIT_CRITICAL(&esp_now_Mux);
  return mode;
}

sendToMotor_t getSendToMotorData() {
  taskENTER_CRITICAL(&esp_now_Mux);
  sendToMotor_t data = sendToMotor;
  taskEXIT_CRITICAL(&esp_now_Mux);
  return data;
}
