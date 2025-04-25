# 🧑🏻‍🦲 Hair Transplant Automation System (Graduation Project)

## 📘 Overview

This is a novel graduation project aimed at supporting **automated hair transplant procedures** using smart mechanical design and real-time patient monitoring. The system ensures accurate positioning and safety by integrating mechanical components, biometric sensors, and visual feedback — all while maintaining a clean, user-friendly setup.

---

## 🧩 Key Features

- 🧱 **Head Positioning Mechanism**  
  A mechanical system automatically aligns and fixes the patient's head onto a custom circular aluminum frame (42cm diameter) before the operation, enhancing accuracy and reducing manual adjustments.

- 💺 **Smart Chair Integration**  
  All electronics and sensors are embedded discreetly within the chair to maintain a professional, wire-free appearance and ensure patient comfort.

- 🩺 **Vital Signs Monitoring**  
  - **Heart Rate Sensor** (MAX30100)  
  - **Body Temperature Sensor** (MLX90614)  
  These sensors track the patient’s health during the operation with threshold-based alerting for abnormal values.

- 📟 **Dual Display System**  
  - **OLED Display (UG-2864)**: Shows live vitals to the patient in a subtle way.  
  - **7-inch LCD via Raspberry Pi**: Provides the doctor with a real-time dashboard using a modern Python-based GUI (Tkinter).

- 🔊 **Audio Alert System**  
  Sound alerts are triggered only when abnormal readings (e.g. high/low pulse or temperature) persist for a configurable number of cycles (e.g. 5 consecutive readings).

---

## 🔧 Technologies Used

- **Microcontrollers**: Arduino, ESP8266  
- **Sensors**: MAX30100 (Pulse), MLX90614 (Temperature)  
- **Displays**: UG-2864 OLED, 7” LCD  
- **GUI**: Python with Tkinter on Raspberry Pi  
- **Mechanical Design**: Aluminum frame, custom fixtures

---

## 🛠️ Setup Instructions

1. **Connect Sensors** to Arduino and ensure correct wiring with hidden integration in the chair.
2. **Upload Code** to Arduino for reading and sending biometric data.
3. **Set Up Raspberry Pi** with Python GUI to receive data and display it on the 7" LCD.
4. **Activate Audio Alerts** only when continuous abnormal readings are detected.

---
## 🧪 Future Enhancements

- Add automatic logging of patient vitals to a database.
- Integrate servo-controlled needle holders for semi-automation of the procedure.
- Introduce wireless data transmission between components.

---

## 🏁 Status

> ✅ In progress – Prototype and monitoring system working. Final testing ongoing.
