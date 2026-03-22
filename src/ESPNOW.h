#pragma once

#include <Arduino.h>

// struct sendToMotor_t {
//   uint16_t speed;
//   bool     data[4] = { }; // 0、左转，1、右转，2、电推，3、功能
//   float    batVoltage;
// };
// extern volatile sendToMotor_t send_to_motor;

enum recvFromMotor_e {
  HAND_MODE,    // 手动模式
  FOOT_MODE,    // 脚控模式
  CRUISE_MODE,  // 巡航模式
  STANDBY_MODE, // 待机模式
};
extern volatile recvFromMotor_e recv_from_motor;

extern volatile bool isMotorOnline;

void esp_now_setup();
void esp_now_connection_check(void* pvParameters);
void dataSent(void* pvParameters);
