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
  Serial1.begin(9500);   

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
