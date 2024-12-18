#include <Arduino.h>
#include <ESP8266WiFi.h>

#define LED_PIN                 2
#define MOTION_INPUT_PIN        4
#define MOTION_OUTPUT_PIN       13
#define PC_IN_USE_OUTPUT_PIN    5
#define TEAMS_ACTIVE_OUTPUT_PIN 12
#define DOOR_CLOSED_INPUT_PIN   14
#define DOOR_CLOSED_OUTPUT_PIN  16

#ifndef MAX
#define MAX(x, y)   ( ((x) > (y))? (x) : (y) )
#endif
#ifndef MIN
#define MIN(x, y)   ( ((x) < (y))? (x) : (y) )
#endif

void SetupVars(void);
void SetupPins(void);
void LedSet(bool ledIsOn);
void SetupWifi(void);
void SerialRxHandle(void);
void ProcessSerialMessage(char* pMsg, uint8_t msgLen);
void MatchStringAndControlPin(const char* pMsg, uint8_t msgLen, const char* pString, uint8_t pin, uint8_t val);
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
  LedSet(false);


  pinMode(PC_IN_USE_OUTPUT_PIN, OUTPUT);
  pinMode(MOTION_OUTPUT_PIN, OUTPUT);
  pinMode(TEAMS_ACTIVE_OUTPUT_PIN, OUTPUT);
  pinMode(DOOR_CLOSED_OUTPUT_PIN, OUTPUT);

  digitalWrite(PC_IN_USE_OUTPUT_PIN, LOW);
  digitalWrite(MOTION_OUTPUT_PIN, LOW);
  digitalWrite(TEAMS_ACTIVE_OUTPUT_PIN, LOW);
  digitalWrite(DOOR_CLOSED_OUTPUT_PIN, LOW);

  pinMode(MOTION_INPUT_PIN, INPUT);
  pinMode(DOOR_CLOSED_INPUT_PIN, INPUT);
  

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
  SerialRxHandle();
  digitalWrite(MOTION_OUTPUT_PIN, digitalRead(MOTION_INPUT_PIN));
  digitalWrite(DOOR_CLOSED_OUTPUT_PIN, digitalRead(DOOR_CLOSED_INPUT_PIN));
}
void SerialRxHandle(void) 
{
  static const char startKey[] = {'S', 't', 'a', 'r', 't', ':', ' '};

  static char rxBuf[300];
  static uint8_t rxBufOffset = 0;

  int numRxBytes = Serial.available();
  int numBytesToRead = MIN(rxBufOffset - sizeof(rxBuf), (uint8_t)numRxBytes);
  numRxBytes -= numBytesToRead;

  while (numBytesToRead--) {
    rxBuf[rxBufOffset++] = Serial.read();
  }

  int consumeSize = 1; //start with a positive number to start the loop.
  while (rxBufOffset && consumeSize) 
  {
    consumeSize = 0;
    int parseOffset = 0;
    if (rxBufOffset >= sizeof(startKey)) 
    {
      consumeSize = 1;
      if (memcmp(&startKey, &rxBuf[parseOffset], sizeof(startKey)) == 0) 
      {
        consumeSize = 0;
        parseOffset += sizeof(startKey);
        char* pEnd = (char*)memchr(&rxBuf[parseOffset], '\n', rxBufOffset - parseOffset);
        if (pEnd)
        {
          consumeSize = pEnd - rxBuf;
          ProcessSerialMessage(&rxBuf[parseOffset], pEnd - &rxBuf[parseOffset]);
        } 
        else 
        {
          break;
        }
      }
    } 
    else 
    {
      break;
    }

    rxBufOffset -= consumeSize;
    memmove(&rxBuf[0], &rxBuf[consumeSize], rxBufOffset);
  }
}
void ProcessSerialMessage(char* pMsg, uint8_t msgLen)
{
  MatchStringAndControlPin(pMsg, msgLen, "workstation locked", PC_IN_USE_OUTPUT_PIN, LOW);
  MatchStringAndControlPin(pMsg, msgLen, "user is active", PC_IN_USE_OUTPUT_PIN, HIGH);
  MatchStringAndControlPin(pMsg, msgLen, "teams call started", TEAMS_ACTIVE_OUTPUT_PIN, HIGH);
  MatchStringAndControlPin(pMsg, msgLen, "no teams use", TEAMS_ACTIVE_OUTPUT_PIN, LOW);
}
void MatchStringAndControlPin(const char* pMsg, uint8_t msgLen, const char* pString, uint8_t pin, uint8_t val)
{
  if (msgLen == strlen(pString) && memcmp(pMsg, pString, msgLen) == 0)
  {
    digitalWrite(pin, val);
    Serial.println(pString);
  }
}
uint32_t RunningTimeMs(void)
{
  return millis() - g.powerOnTimeMs;
}