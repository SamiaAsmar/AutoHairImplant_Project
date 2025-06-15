import serial
import time
import os

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
SOUNDS_DIR = '/home/jood/'

alert_sounds = {
    "low_bpm": "test.mp3",
    "high_bpm": "test2.mp3",
    "low_temp": "test3.mp3",
    "high_temp": "test4.mp3"
}

# Counters
low_bpm_count = 0
high_bpm_count = 0
low_temp_count = 0
high_temp_count = 0

last_mode = None

def play_sound(file_name):
    file_path = os.path.join(SOUNDS_DIR, file_name)
    if os.path.exists(file_path):
        print(f"Playing sound: {file_path}")
        os.system(f'mpg123 "{file_path}"')

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print("Listening for alerts from Arduino...")
    time.sleep(2)

    while True:
        line = ser.readline().decode('utf-8').strip()
        if line:
            print(f"Received: {line}")
            try:
                temp_str, bpm_str, sp02_str = line.split('|')
                temp = float(temp_str.split(':')[1].strip())
                bpm = float(bpm_str.split(':')[1].strip())
                sp02 = float(sp02_str.split(':')[1].strip())

                if bpm > 0 and sp02 > 0:
                    last_mode = "bpm"
                    # Reset Ø­Ø±Ø§Ø±Ø©
                    low_temp_count = high_temp_count = 0

                    if bpm < 50:
                        low_bpm_count += 1
                        high_bpm_count = 0
                        if low_bpm_count >= 3:
                            play_sound(alert_sounds["low_bpm"])
                            low_bpm_count = 0
                    elif bpm > 100:
                        high_bpm_count += 1
                        low_bpm_count = 0
                        if high_bpm_count >= 3:
                            play_sound(alert_sounds["high_bpm"])
                            high_bpm_count = 0
                    else:
                        # Ø·Ø¨ÙØ¹Ù
                        low_bpm_count = 0
                        high_bpm_count = 0

                elif temp > 0:
                    last_mode = "temp"
                    # Reset ÙØ¨Ø¶
                    low_bpm_count = high_bpm_count = 0

                    if temp < 18:
                        low_temp_count += 1
                        high_temp_count = 0
                        if low_temp_count >= 3:
                            play_sound(alert_sounds["low_temp"])
                            low_temp_count = 0
                    elif temp > 38.0:
                        high_temp_count += 1
                        low_temp_count = 0
                        if high_temp_count >= 3:
                            play_sound(alert_sounds["high_temp"])
                            high_temp_count = 0
                    else:
                        # Ø·Ø¨ÙØ¹Ù
                        low_temp_count = 0
                        high_temp_count = 0

            except ValueError:
                print("Error parsing data.")

except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
except KeyboardInterrupt:
    print("Exiting...")


//////////////////////////////////////////////////////////
import serial
import time
import os
import subprocess

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 9600
SOUNDS_DIR = '/home/jood/'

alert_sounds = {
    "LOW_BPM": "LowPulse.mp3",
    "HIGH_BPM": "HighPulse.mp3",
    "LOW_TEMP": "LowTemp.mp3",
    "HIGH_TEMP": "HighTemp.mp3"
}

def play_sound(file_name):
    file_path = os.path.join(SOUNDS_DIR, file_name)
    if os.path.exists(file_path):
        print(f"Playing: {file_path}")
        subprocess.Popen(['mpg123', file_path])
    else:
        print(f"Sound file not found: {file_path}")

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    print("Listening for alerts from Arduino...")
    time.sleep(2)

    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"Received: {line}")
                if line in alert_sounds:
                    play_sound(alert_sounds[line])
        time.sleep(0.01)

except serial.SerialException as e:
    print(f"Serial Error: {e}")
except KeyboardInterrupt:
    print("Exiting...")
