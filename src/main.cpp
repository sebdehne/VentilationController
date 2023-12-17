#include <Arduino.h>

#include "config.h"
#include "config_service.h"
#include "logger.h"
#include "dac_dfr0971.h"
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <WiFiNINA.h>

struct SensorState
{
  boolean in_above_lower_limit_state;
  uint32_t last_state_change_at;
};

uint32_t wait_for_pin_a1(boolean expected_value);
uint8_t readDuticycle();
uint8_t calculate(uint8_t value, uint8_t settingsIndexInConfig, SensorState *sensorState);
void printValues();
void handle_ventilation();
u_int8_t connect_WiFi(); // 1 == success; 0 == not connected
void handleWebserver();
void printWEB();
void parseAndAcceptNewConfig(char *buf);
bool handleConfigChange(String name, String value);
bool wifiChanged = false;
void urlDecode(String s);

WiFiServer server(80); // server socket
WiFiClient client = server.available();

// bathroom state
boolean is_in_operation = false;
uint32_t started_operation_at = 0;

Adafruit_BME280 bme;

SensorState bathState = {
    false,
    0};
SensorState kitchenState = {
    false,
    0};

void setup()
{
  Serial.begin(115200);
#ifdef DEBUG
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  Log.log("Welcome");
#endif

  WiFi.setTimeout(30 * 1000);

  FlashUtils.init();
  ConfigService.init();

  DacDfr0971.setup();
  DacDfr0971.setDacMillivoltage(0);

  while (!bme.begin())
  {
    Log.log("Could not init BME280 humidity sensor");
    delay(1000);
  }

  pinMode(COOCKING_HOOD_PIN, INPUT);
}

unsigned long last_ventilation_handled_time = 0;
const unsigned long ventilation_control_delay = 5000;

void loop()
{

  // ensure wifi is up and connected
  if (connect_WiFi())
  {
    handleWebserver();
  }
  else
  {
    Log.log("wifi not connected, resetting in 10 seconds...");
    delay(10000);
    NVIC_SystemReset();
  }

  unsigned long now = millis();

  if (now - last_ventilation_handled_time > ventilation_control_delay)
  {
    handle_ventilation();
    last_ventilation_handled_time = now;
  }
}

void handle_ventilation()
{
  char buf[100];

  int baseSetting = atoi(ConfigService.config[CONFIG_POS_BASE]);

  // 1 read kitchen state
  uint8_t kitchen = readDuticycle();
  snprintf(buf, 100, "Kitchen: %u", kitchen);
  Log.log(buf);

  // 2 read bath humidity
  u_int8_t bath = bme.readHumidity();
  snprintf(buf, 100, "Bath: %u", bath);
  Log.log(buf);

  // 3 eval
  uint8_t kitchen_target = calculate(kitchen, CONFIG_POS_KITCHEN_LIMIT_FROM, &kitchenState);
  snprintf(buf, 100, "Kitchen-calculated: %u", kitchen_target);
  Log.log(buf);
  uint8_t bath_target = calculate(bath, CONFIG_POS_BATH_LIMIT_FROM, &bathState);
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
}

uint8_t calculate(uint8_t value, uint8_t settingsIndexInConfig, SensorState *sensorState)
{

  int limit_from = atoi(ConfigService.config[settingsIndexInConfig]);
  int limit_to = atoi(ConfigService.config[settingsIndexInConfig + 1]);
  int offset = atoi(ConfigService.config[settingsIndexInConfig + 2]);
  int curve_x_1000 = atoi(ConfigService.config[settingsIndexInConfig + 3]);
  int give_up_after_time_minutes = atoi(ConfigService.config[settingsIndexInConfig + 4]);

  if (value < limit_from)
  {
    sensorState->in_above_lower_limit_state = false;
    return 0;
  }

  if (give_up_after_time_minutes >= 0)
  {
    if (!sensorState->in_above_lower_limit_state)
    {
      sensorState->in_above_lower_limit_state = true;
      sensorState->last_state_change_at = millis();
    }
    else
    {
      uint32_t delta = millis() - sensorState->last_state_change_at;
      if ((delta / 1000 / 60) >= give_up_after_time_minutes)
      {
        // disabled
        return 0;
      }
    }
  }

  if (value > limit_to)
  {
    return 100;
  }

  uint8_t calc = ((value * curve_x_1000) / 1000) + offset;
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

u_int8_t connect_WiFi()
{
  char buf[100];
  int status = WiFi.status();

  for (int i = 0; i < 2; i++)
  {
    if (!wifiChanged && status == WL_CONNECTED)
    {
      return 1;
    }

    snprintf(buf, 100, "Attempting to connect to SSID: %s / %s", ConfigService.config[CONFIG_POS_WIFI1_SSID + (i * 2)], ConfigService.config[CONFIG_POS_WIFI1_PASSWD + (i * 2)]);
    Log.log(buf);

    status = WiFi.begin(ConfigService.config[CONFIG_POS_WIFI1_SSID + (i * 2)], ConfigService.config[CONFIG_POS_WIFI1_PASSWD + (i * 2)]);
    wifiChanged = false;

    if (status == WL_CONNECTED)
    {
      server.begin();

      IPAddress ip = WiFi.localIP();
      snprintf(buf, 100, "Wifi is now connected - %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      Log.log(buf);
    }
    else
    {
      delay(10000);
    }
  }

  snprintf(buf, 100, "Wifi failed");
  Log.log(buf);
  return 0;
}

void handleWebserver()
{

  client = server.available();
  if (client)
  {
    printWEB();
  }
}

void printHtmlFormInput();

void printHtmlFormInput(const char *label, char *value)
{
  char buf[300];
  snprintf(buf, 300, "<label for=\"%s\">%s:</label><input type=\"text\" id=\"%s\" name=\"%s\" value=\"%s\"><br><br>\r\n", label, label, label, label, value);
  client.write(buf);
}

void printWEB()
{

  // 1: GET MAIN PAGE
  // 2: POST FORM
  // 3: unknown
  int request_type = 0; //
  int contentLength = 0;

  if (client)
  {
    Log.log("new client");
    String currentLine = "";
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();

        if (c == '\n')
        {

#ifdef DEBUG
          Serial.println(currentLine);
#endif
          if (request_type == 0)
          {
            if (currentLine.startsWith("GET / "))
            {
              request_type = 1;
            }
            else if (currentLine.startsWith("POST /submit "))
            {
              request_type = 2;
            }
            else
            {
              request_type = 3;
            }
          }

          if (contentLength == 0 && currentLine.startsWith("Content-Length: "))
          {
            contentLength = currentLine.substring(16).toInt();
          }

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            if (request_type == 1)
            {

              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              client.println("<!DOCTYPE html>");
              client.println("<html><body>");
              client.println("<form action=\"/submit\" method=\"post\">");

              for (int i = 0; i < CONFIG_LEN; i++)
              {
                printHtmlFormInput(configNames[i], ConfigService.config[i]);
              }

              client.println("<input type=\"submit\" value=\"Submit\">");
              client.println("</form>");
              client.println("</body></html>");

              // The HTTP response ends with another blank line:
              client.println();
            }
            else if (request_type == 2)
            {
              // read the body
              char buf[1000];
              int pos = 0;

              while (pos < contentLength)
              {
                if (client.available())
                {
                  buf[pos++] = client.read();
                }
              }
              buf[pos] = 0;

              parseAndAcceptNewConfig(buf);

              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              client.println("<!DOCTYPE html>");
              client.println("<html><body>");
              client.println("<h3>Thanks</h3>");
              client.println("<p><a href=\"/\">Back</a></p>");
              client.println("</body></html>");
            }
            else if (request_type == 3)
            {
              client.println("HTTP/1.1 404 Not Found");
              client.println("Content-type:text/plain");
              client.println();

              client.println("No such page");
            }

            // break out of the while loop:
            break;
          }
          else
          { // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Log.log("client disconnected");
  }
}

void parseAndAcceptNewConfig(char *buf)
{

  String s(buf);
  String temp = "";
  String name;
  String value;

  bool somethingChanged = false;

  int pos = 0;
  while (pos < s.length())
  {
    char c = s.charAt(pos++);
    if (c == 0)
    {
      break;
    }

    if (c == '=')
    {
      name = temp;
      urlDecode(name);
      temp = "";
    }
    else if (c == '&')
    {
      value = temp;
      urlDecode(value);
      temp = "";

      somethingChanged = somethingChanged | handleConfigChange(name, value);
      name = "";
      value = "";
    }
    else
    {
      temp += c;
    }
  }

  if (name.length() > 0 && temp.length() > 0)
  {
    urlDecode(temp);
    somethingChanged = somethingChanged | handleConfigChange(name, temp);
  }

  if (somethingChanged)
  {
    ConfigService.writeConfig();
    Log.log("Config updated");
  }
}

bool handleConfigChange(String name, String value)
{
#ifdef DEBUG
  Serial.print("name=");
  Serial.print(name);
  Serial.print(", value=");
  Serial.println(value);
#endif

  for (int i = 0; i < CONFIG_LEN; i++)
  {
    String c(configNames[i]);
    if (c == name)
    {
      String current(ConfigService.config[i]);
      if (current != value)
      {
        strcpy(ConfigService.config[i], value.c_str());
        if (name.startsWith("wifi"))
        {
          wifiChanged = true;
        }
        return true;
      }
      return false;
    }
  }

  return false;
}

void urlDecode(String s)
{
  s.replace("%3D", "=");
  s.replace("%3d", "=");
  s.replace("%26", "&");
}