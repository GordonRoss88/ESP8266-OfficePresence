#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LED_PIN                 2

void SetupVars(void);
void SetupPins(void);
void LedSet(bool ledIsOn);
void SetupWifi(void);
uint32_t RunningTimeMs(void);

unsigned long powerOnTimeMs;
bool lastLedState = true;

void setup(void)
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n");
  Serial.print(millis());
  Serial.println("Power On");
  SetupVars();
  SetupPins();
  SetupWifi();
}
void SetupVars(void)
{
  powerOnTimeMs = millis();
}
void SetupPins(void)
{
  pinMode(LED_PIN, OUTPUT);
  LedSet(false);
}
void SetupWifi(void)
{
  //Disable wifi
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);
}
void LedSet(bool on)
{
  digitalWrite(LED_PIN, on == true ? LOW : HIGH);
}
void loop(void)
{
  static unsigned long int nextToggleTime = 0;
  if (millis() > nextToggleTime)
  {
    nextToggleTime += 1000;
    lastLedState = !lastLedState;
    LedSet(lastLedState);
  }
}
uint32_t RunningTimeMs(void)
{
  return millis() - powerOnTimeMs;
}