#include <Wire.h>
#include <U8g2lib.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 3000

#define SOUND_HIGH_TEMP   1  // 000.mp3
#define SOUND_LOW_PULSE   2  // 001.mp3
#define SOUND_LOW_TEMP    3  // 002.mp3
#define SOUND_HIGH_PULSE  4  // 003.mp3

PulseOximeter pox;
uint32_t tsLastReport = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

String tempStr = "";
float temperature = 0;

int lowBpmCount = 0, highBpmCount = 0;
int lowTempCount = 0, highTempCount = 0;

bool lowBpmSent = false, highBpmSent = false;
bool lowTempSent = false, highTempSent = false;

void setup() {
  delay(3000);
  Serial.begin(9600);   
  Serial1.begin(9600);  
  Serial2.begin(9600);  

  Wire.begin();
  Wire.setClock(25000); 

  u8g2.begin();
  delay(100);
  displayMessage("Initializing...");

  if (!pox.begin()) {
    displayMessage("Sensor Fail!");
    while (1);
  }

  displayMessage("Ready");
}

void readTemperature() {
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    if (c == '\n') {
      temperature = tempStr.toFloat();
      tempStr = "";
    } else {
      tempStr += c;
    }
  }
}

void loop() {
  pox.update();
  readTemperature();

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis();

    float bpm = pox.getHeartRate();
    float spo2 = pox.getSpO2();

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.setCursor(5, 20); u8g2.print("BPM: "); u8g2.print(bpm, 0);
    u8g2.setCursor(5, 40); u8g2.print("SpO2: "); u8g2.print(spo2, 0); u8g2.print(" %");
    u8g2.setCursor(3, 60); u8g2.print("Temp: "); u8g2.print(temperature, 1); u8g2.print(" C");
    u8g2.sendBuffer();

    if (bpm > 0 && spo2 > 0) {
      if (bpm < 50) {
        lowBpmCount++;
        highBpmCount = 0;
        if (lowBpmCount >= 3 && !lowBpmSent) {
          Serial.println("LOW_PULSE");
          playSound(SOUND_LOW_PULSE);
          lowBpmSent = true;
          highBpmSent = false;
        }
      } else if (bpm > 100) {
        highBpmCount++;
        lowBpmCount = 0;
        if (highBpmCount >= 3 && !highBpmSent) {
          Serial.println("HIGH_PULSE");
          playSound(SOUND_HIGH_PULSE);
          highBpmSent = true;
          lowBpmSent = false;
        }
      } else {
        lowBpmCount = highBpmCount = 0;
        lowBpmSent = highBpmSent = false;
      }
    }

    if (temperature > 0) {
      if (temperature < 18.0) {
        lowTempCount++;
        highTempCount = 0;
        if (lowTempCount >= 3 && !lowTempSent) {
          Serial.println("LOW_TEMP");
          playSound(SOUND_LOW_TEMP);
          lowTempSent = true;
          highTempSent = false;
        }
      } else if (temperature > 38.0) {
        highTempCount++;
        lowTempCount = 0;
        if (highTempCount >= 3 && !highTempSent) {
          Serial.println("HIGH_TEMP");
          playSound(SOUND_HIGH_TEMP);
          highTempSent = true;
          lowTempSent = false;
        }
      } else {
        lowTempCount = highTempCount = 0;
        lowTempSent = highTempSent = false;
      }
    }
  }
}

void playSound(uint8_t soundType) {
  switch (soundType) {
    case SOUND_HIGH_TEMP:
      sendCommand(0x03, 1); 
      break;
    case SOUND_LOW_PULSE:
      sendCommand(0x03, 2); 
      break;
    case SOUND_LOW_TEMP:
      sendCommand(0x03, 3); 
      break;
    case SOUND_HIGH_PULSE:
      sendCommand(0x03, 4); 
      break;
    default:
      break;
  }
}

void sendCommand(uint8_t command, uint16_t parameter) {
  uint8_t commandLine[10] = {0x7E, 0xFF, 0x06, command, 0x00,
                             (uint8_t)(parameter >> 8), (uint8_t)(parameter & 0xFF),
                             0x00, 0x00, 0xEF};

  uint16_t checksum = 0;
  for (int i = 1; i < 7; i++) {
    checksum += commandLine[i];
  }
  checksum = 0 - checksum;

  commandLine[7] = (uint8_t)(checksum >> 8);
  commandLine[8] = (uint8_t)(checksum & 0xFF);

  for (int i = 0; i < 10; i++) {
    Serial2.write(commandLine[i]);
  }
}

void displayMessage(const char* msg) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 30, msg);
  u8g2.sendBuffer();
}
