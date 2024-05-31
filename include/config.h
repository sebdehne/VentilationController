
#include <Arduino.h>

// reserve flash (in bytes) for firmware and userspace
#define FIRMWARE_SIZE 60000 // should be large enough to hold the firmware

#define DEBUG

#define LOG_BUFFER_SIZE 1024

#define COOCKING_HOOD_PIN PIN_A1

#define DAC_ADDR = 0x5f;