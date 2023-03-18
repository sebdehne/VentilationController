#include <Arduino.h>

#include "config.h"
#include "logger.h"
#include "dac_dfr0971.h"
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

struct SensorSettings
{
  u_int8_t limit_from;
  u_int8_t limit_to;
  int16_t offset;
  uint16_t curve_x_1000;
};

uint32_t wait_for_pin_a1(boolean expected_value);
uint8_t readDuticycle();
uint8_t calculate(uint8_t value, SensorSettings settings);
void printValues();

Adafruit_BME280 bme;

SensorSettings bathSettings = {
    50,
    90,
    -20,
    1500};
SensorSettings kitchenSettings = {
    1,
    90,
    10,
    1500};
uint8_t baseSetting = 25;

void setup()
{
  Serial.begin(115200);

  DacDfr0971.setup();
  DacDfr0971.setDacMillivoltage(0);

  while (!bme.begin())
  {
    Log.log("Could not init BME280 humidity sensor");
    delay(1000);
  }

  pinMode(COOCKING_HOOD_PIN, INPUT);
}

void loop()
{
  // 1 read kitchen state
  uint8_t kitchen = readDuticycle();
  char buf[100];
  snprintf(buf, 100, "Kitchen: %u", kitchen);
  Log.log(buf);

  // 2 read bath humidity
  u_int8_t bath = bme.readHumidity();
  snprintf(buf, 100, "Bath: %u", bath);
  Log.log(buf);

  // 3 eval
  uint8_t kitchen_target = calculate(kitchen, kitchenSettings);
  snprintf(buf, 100, "Kitchen-calculated: %u", kitchen_target);
  Log.log(buf);
  uint8_t bath_target = calculate(bath, bathSettings);
  snprintf(buf, 100, "Bath-calculated: %u", bath_target);
  Log.log(buf);

  // 4 write to DAC
  uint8_t target = max(max(kitchen_target, bath_target), baseSetting);
  snprintf(buf, 100, "Target: %u", target);
  Log.log(buf);
  uint16_t millivolts = (target * 10000) / 100;
  snprintf(buf, 100, "millivolts: %u", millivolts);
  Log.log(buf);

  DacDfr0971.setDacMillivoltage(millivolts);

  Log.log("=======");

  delay(250);
}

uint8_t calculate(uint8_t value, SensorSettings settings)
{
  if (value > settings.limit_to)
  {
    return 100;
  }
  if (value < settings.limit_from)
  {
    return 0;
  }
  uint8_t calc = ((value * settings.curve_x_1000) / 1000) + settings.offset;
  return min(calc, 100);
}

uint8_t readDuticycle()
{
  uint8_t cycle = 100;

  uint32_t start = wait_for_pin_a1(false);
  if (start > 1)
  {
    // currently at low, wait for edge to high
    uint32_t to_high = wait_for_pin_a1(true);
    if (to_high > 0)
    {
      // switched to high
      uint32_t high = wait_for_pin_a1(false);
      uint32_t low = wait_for_pin_a1(true);

      if (low > 0 && high > 0)
      {
        uint32_t total = low + high;
        cycle = low * 100 / total;
      }
    }
  }
  else
  {
    // timeout: currently at high
    cycle = 0;
  }

  return cycle;
}

uint32_t wait_for_pin_a1(boolean expected_value)
{
  uint8_t sensitivity = 5;
  uint32_t timeout_number_of_reads = 20000;
  uint32_t number_of_reads = 0;
  u_int32_t number_of_changes = 0;

  while (1)
  {
    if (expected_value == digitalRead(COOCKING_HOOD_PIN))
    {
      if (number_of_changes++ >= sensitivity)
      {
        return number_of_reads;
      }
    }
    else
    {
      number_of_changes = 0;
    }

    number_of_reads++;

    if (number_of_reads >= timeout_number_of_reads)
    {
      return 0;
    }
  }
}

