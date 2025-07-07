#define USE_RGB_LED 1

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>

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
void SetColorSettings(const char* pMsg, uint8_t msgLen);
bool ValidateAndExtractHsvFromString(const char* pMsg, uint8_t msgLen, CRGB* ledRgb, uint8_t numLeds);
void MatchStringAndControlPin(const char* pMsg, uint8_t msgLen, const char* pString, uint8_t pin, uint8_t val);
uint32_t RunningTimeMs(void);
void ActivateLed(uint8_t pin, uint8_t val);

typedef struct
{
  unsigned long powerOnTimeMs;
  CRGB ledRgbCurrent;
  CRGB ledRgbSettings;
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
  g.ledRgbSettings = CHSV(0, 255, 200);
}
void SetupPins(void)
{
  pinMode(LED_PIN, OUTPUT);
  LedSet(false);

  pinMode(PC_IN_USE_OUTPUT_PIN, OUTPUT);
  pinMode(MOTION_OUTPUT_PIN, OUTPUT);
#if USE_RGB_LED
  FastLED.addLeds<NEOPIXEL, TEAMS_ACTIVE_OUTPUT_PIN>(&g.ledRgbCurrent, 1);
#else
  pinMode(TEAMS_ACTIVE_OUTPUT_PIN, OUTPUT);
#endif
  pinMode(DOOR_CLOSED_OUTPUT_PIN, OUTPUT);

  ActivateLed(PC_IN_USE_OUTPUT_PIN, LOW);
  ActivateLed(MOTION_OUTPUT_PIN, LOW);
  ActivateLed(TEAMS_ACTIVE_OUTPUT_PIN, LOW);
  ActivateLed(DOOR_CLOSED_OUTPUT_PIN, LOW);

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
  ActivateLed(MOTION_OUTPUT_PIN, digitalRead(MOTION_INPUT_PIN));
  ActivateLed(DOOR_CLOSED_OUTPUT_PIN, digitalRead(DOOR_CLOSED_INPUT_PIN));
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
          if (pEnd[-1] == '\r')
          {
            pEnd--;
          }
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
  SetColorSettings(pMsg, msgLen);
  MatchStringAndControlPin(pMsg, msgLen, "workstation locked", PC_IN_USE_OUTPUT_PIN, LOW);
  MatchStringAndControlPin(pMsg, msgLen, "user is active", PC_IN_USE_OUTPUT_PIN, HIGH);
  MatchStringAndControlPin(pMsg, msgLen, "teams call started", TEAMS_ACTIVE_OUTPUT_PIN, HIGH);
  MatchStringAndControlPin(pMsg, msgLen, "no teams use", TEAMS_ACTIVE_OUTPUT_PIN, LOW);
}
void SetColorSettings(const char* pMsg, uint8_t msgLen)
{
  const char* pString = "set color settings: ";
  uint8_t len = strlen(pString);
  if (memcmp(pMsg, pString, len) == 0)
  {
    if (ValidateAndExtractHsvFromString(pMsg + len, msgLen - len, &g.ledRgbSettings, 1))
    {
      Serial.println(pMsg);
    }
  }
}
bool ValidateAndExtractHsvFromString(const char* pMsg, uint8_t msgLen, CRGB* ledRgb, uint8_t numLeds)
{
  const char* pMsgOriginal = pMsg;
  bool expectingNumber = true;
  uint8_t numberCount = 0;
  uint8_t spaceCount = 0;
  while (msgLen--)
  {
    if (expectingNumber)
    {
      if (isdigit(*pMsg))
      {
        pMsg++;
        numberCount++;
        expectingNumber = false;
        continue;
      }
      else
      {
        return false;
      }
    }

    if (isdigit(*pMsg))
    {
      pMsg++;
    }
    else if (isspace(*pMsg))
    {
      pMsg++;
      spaceCount++;
      expectingNumber = true;
    }
  }
  
  if ((numberCount != numLeds*3) || (spaceCount != (numLeds*3 - 1)))
  {
    return false;
  }

  pMsg = pMsgOriginal;
  for (uint8_t numIdx = 0; numIdx < numberCount; numIdx++)
  {
    int32_t number = atoi(pMsg);
    if (number > 255)
    {
      return false;
    }
    if (numIdx < spaceCount)
    {
      pMsg = strchr(pMsg, ' ') + 1;
    }
  }

  pMsg = pMsgOriginal;
  for (uint8_t ledIdx = 0; ledIdx < numLeds; ledIdx++)
  {
    uint32_t hue = atoi(pMsg);
    pMsg = strchr(pMsg, ' ') + 1;
    uint32_t saturation = atoi(pMsg);
    pMsg = strchr(pMsg, ' ') + 1;
    uint32_t value = atoi(pMsg);

    ledRgb[ledIdx] = CHSV(hue, saturation, value);

    if ((ledIdx + 1) < numLeds)
    {
      pMsg = strchr(pMsg, ' ') + 1;
    }
  }

  return true;
}
void MatchStringAndControlPin(const char* pMsg, uint8_t msgLen, const char* pString, uint8_t pin, uint8_t val)
{
  if (msgLen == strlen(pString) && memcmp(pMsg, pString, msgLen) == 0)
  {
    ActivateLed(pin, val);
    Serial.println(pString);
  }
}
uint32_t RunningTimeMs(void)
{
  return millis() - g.powerOnTimeMs;
}
void ActivateLed(uint8_t pin, uint8_t val)
{
#if USE_RGB_LED
  if (pin == TEAMS_ACTIVE_OUTPUT_PIN)
  {
    if (val == HIGH)
    {
      g.ledRgbCurrent = g.ledRgbSettings;
    }
    else
    {
      g.ledRgbCurrent = CRGB(0, 0, 0);
    }
    FastLED.show();
  }
  else
  {
    digitalWrite(pin, val);
  }
#else
  digitalWrite(pin, val);
#endif
}