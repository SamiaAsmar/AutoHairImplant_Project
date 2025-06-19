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

////////////////////////////////////////////////////////////////////////////////
//final

#include <Wire.h>
#include <U8g2lib.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 3000

// تعريف الأصوات حسب ترتيب ملفات الـ SD (ملفات 000.mp3, 001.mp3, ...)
// رقم الملف يبدأ من 1 لأن في الكود تستخدم 1، 2، 3، 4
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
  Serial.begin(9600);   // ديباغ
  Serial1.begin(9600);  // استقبال الحرارة والبيانات من الحساس
  Serial2.begin(9600);  // موديول الصوت (MP3-TF-16P)

  Wire.begin();
  Wire.setClock(25000); // تخفيف سرعة الـ I2C

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

    // معالجة نبض القلب
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

    // معالجة درجة الحرارة
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
      sendCommand(0x03, 1); // تشغيل الملف 000.mp3
      break;
    case SOUND_LOW_PULSE:
      sendCommand(0x03, 2); // تشغيل الملف 001.mp3
      break;
    case SOUND_LOW_TEMP:
      sendCommand(0x03, 3); // تشغيل الملف 002.mp3
      break;
    case SOUND_HIGH_PULSE:
      sendCommand(0x03, 4); // تشغيل الملف 003.mp3
      break;
    default:
      break;
  }
}

void sendCommand(uint8_t command, uint16_t parameter) {
  uint8_t commandLine[10] = {0x7E, 0xFF, 0x06, command, 0x00,
                             (uint8_t)(parameter >> 8), (uint8_t)(parameter & 0xFF),
                             0x00, 0x00, 0xEF};

  // حساب checksum
  uint16_t checksum = 0;
  for (int i = 1; i < 7; i++) {
    checksum += commandLine[i];
  }
  checksum = 0 - checksum;

  commandLine[7] = (uint8_t)(checksum >> 8);
  commandLine[8] = (uint8_t)(checksum & 0xFF);

  // أرسل الأمر على Serial2 لأنه موديول الصوت موصول هنا
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
