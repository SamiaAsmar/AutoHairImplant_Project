#include <Wire.h>
#include <U8g2lib.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

PulseOximeter pox;
uint32_t tsLastReport = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

String tempStr = "N/A";
float temperature = 0;

void setup() {
  Serial.begin(9600);  
  Serial1.begin(9600);   

  Wire.begin();
  u8g2.begin();
  displayMessage("Initializing...");

  if (!pox.begin()) {
    displayMessage("Sensor Fail!");
    while (1);
  }

  displayMessage("Ready");
}

void loop() {
  pox.update();

  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      temperature = tempStr.toFloat();
      tempStr = "";
    } else {
      tempStr += c;
    }
  }

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis();

    float bpm = pox.getHeartRate();
    float spo2 = pox.getSpO2();

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);  

    u8g2.setCursor(5, 20);
    u8g2.print("BPM: "); u8g2.print(bpm, 0);

    u8g2.setCursor(5, 40);
    u8g2.print("SpO2: "); u8g2.print(spo2, 0); u8g2.print(" %");

    u8g2.setCursor(3, 60);
    u8g2.print("Temp: "); u8g2.print(temperature, 1); u8g2.print(" C");

    u8g2.sendBuffer(); 

    // Optional Debug
    Serial.print("Temp: "); Serial.print(temperature);
    Serial.print(" | BPM: "); Serial.print(bpm);
    Serial.print(" | SpO2: "); Serial.println(spo2);
  }
}

void displayMessage(const char* msg) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 30, msg);
  u8g2.sendBuffer();
}






//////////////////////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <U8g2lib.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

PulseOximeter pox;
uint32_t tsLastReport = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

String tempStr = "N/A";
float temperature = 0;

// Counters
int lowBpmCount = 0, highBpmCount = 0;
int lowTempCount = 0, highTempCount = 0;

// Flags to avoid repeated alerts
bool lowBpmSent = false, highBpmSent = false;
bool lowTempSent = false, highTempSent = false;

void setup() {
  Serial.begin(9600);      // USB Serial (to Raspberry Pi)
  Serial1.begin(9600);     // Temperature input from another Arduino (if any)

  Wire.begin();
  u8g2.begin();
  displayMessage("Initializing...");

  if (!pox.begin()) {
    displayMessage("Sensor Fail!");
    while (1);
  }

  displayMessage("Ready");
}

void loop() {
  pox.update();

  // Read temperature from Serial1
  if (Serial1.available() > 0) {
    char c = Serial1.read();
    if (c == '\n') {
      temperature = tempStr.toFloat();
      tempStr = "";  // Reset
    } else {
      tempStr += c;
    }
  }

  // Update display once per second
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis();

    float bpm = pox.getHeartRate();
    float spo2 = pox.getSpO2();

    // Update OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.setCursor(5, 20); u8g2.print("BPM: "); u8g2.print(bpm, 0);
    u8g2.setCursor(5, 40); u8g2.print("SpO2: "); u8g2.print(spo2, 0); u8g2.print(" %");
    u8g2.setCursor(3, 60); u8g2.print("Temp: "); u8g2.print(temperature, 1); u8g2.print(" C");
    u8g2.sendBuffer();

    // Logic for alerts
    if (bpm > 0 && spo2 > 0) {
      if (bpm < 50) {
        lowBpmCount++;
        highBpmCount = 0;
        if (lowBpmCount >= 3 && !lowBpmSent) {
          Serial.println("LOW_BPM");
          delay(5);
          lowBpmSent = true;
          highBpmSent = false;
        }
      } else if (bpm > 100) {
        highBpmCount++;
        lowBpmCount = 0;
        if (highBpmCount >= 3 && !highBpmSent) {
          Serial.println("HIGH_BPM");
          delay(5);
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
          delay(5);
          lowTempSent = true;
          highTempSent = false;
        }
      } else if (temperature > 38.0) {
        highTempCount++;
        lowTempCount = 0;
        if (highTempCount >= 3 && !highTempSent) {
          Serial.println("HIGH_TEMP");
          delay(5);
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

void displayMessage(const char* msg) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 30, msg);
  u8g2.sendBuffer();
}
