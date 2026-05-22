#pragma once

#include <Arduino.h>

enum recvFromMotor_e {
  HAND_MODE,    // 手动模式
  FOOT_MODE,    // 脚控模式
  CRUISE_MODE,  // 巡航模式
  STANDBY_MODE, // 待机模式
};
struct sendToMotor_t {
  uint16_t speed;
  bool     data[6] = { }; // 0、左转，1、右转，2、电推，3、功能，4、正在充电，5、电池已满
  float    batVoltage, batPercentage, footPadChipTemp;
};

void            esp_now_setup();
void            connection_state_check(void* pvParameters);
void            sendData(void* pvParameters);
recvFromMotor_e getMotormode();
sendToMotor_t   getSendToMotorData();
