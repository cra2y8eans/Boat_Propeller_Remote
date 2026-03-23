#pragma once

#include <Arduino.h>

enum recvFromMotor_e {
  HAND_MODE,    // 手动模式
  FOOT_MODE,    // 脚控模式
  CRUISE_MODE,  // 巡航模式
  STANDBY_MODE, // 待机模式
};
extern volatile recvFromMotor_e recv_from_motor;

extern volatile bool isMotorOnline;

void esp_now_setup();
void connection_state_check(void* pvParameters);
void sendData(void* pvParameters);
