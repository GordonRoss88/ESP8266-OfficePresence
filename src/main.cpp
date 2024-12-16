#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LED_PIN                 2
#define MOTION_PIN              4

void SetupVars(void);
void SetupPins(void);
void LedSet(bool ledIsOn);
void SetupWifi(void);
uint32_t RunningTimeMs(void);

typedef struct
{
  unsigned long powerOnTimeMs;
}G;
static G g;

void setup(void)
{
  memset(&g, 0x00, sizeof(g));
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
  g.powerOnTimeMs = millis();
}
void SetupPins(void)
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTION_PIN, INPUT);
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
  LedSet(digitalRead(MOTION_PIN) == HIGH);
}
uint32_t RunningTimeMs(void)
{
  return millis() - g.powerOnTimeMs;
}