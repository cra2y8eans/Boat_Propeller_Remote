#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

// ===== 常量定义 =====
#define SHORT_FLASH_DURATION 200
#define SHORT_FLASH_INTERVAL 200
#define LONG_FLASH_DURATION 500
#define LONG_FLASH_INTERVAL 500

#define COLOR_RED 0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE 0x0000FF
#define COLOR_YELLOW 0xFF2800
#define COLOR_WHITE 0xFFFFFF
#define COLOR_CYAN 0x00FFFF
#define COLOR_OFF 0x000000

void ledInit();
void ledTask(void* pvParameters);