#include <AccelStepper.h>
#include <Servo.h>

AccelStepper stepperX(AccelStepper::DRIVER, 2, 3);
AccelStepper stepperY(AccelStepper::DRIVER, 4, 5);
Servo myServo;

const float STEPS_PER_REV = 200.0;
const float MICROSTEPPING = 2.0;
const float SHAFT_DIAMETER_MM = 410.0;
const float HEAD_RADIUS_MM = 210.0;
const float ARM_LENGTH_MM = 340.0;

const float MM_PER_REV = PI * SHAFT_DIAMETER_MM;
const float STEPS_PER_MM = (STEPS_PER_REV * MICROSTEPPING) / MM_PER_REV;

float current_angle = 0;
float currentX_mm = 0;
int i = 0;

// Ultrasonic
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;
const int MIN_DISTANCE_CM = 15;
bool systemStopped = false;

// Servo control
int lastServoAngle = 180;
int targetServoAngle = 180;
bool isServoMoving = false;


// Joystick
const int joystickX = A0;
const int joystickY = A1;

// Push button to toggle mode
const int modeButtonPin = A2;
bool controlModeJoystick = true;  // true = joystick mode, false = serial mode
bool lastButtonState = HIGH;
bool buttonPressed = false;

// Limit switches
const int xMinSwitch = 11;
const int xMaxSwitch = 12;
const int yMinSwitch = 13;

// Emergency stop button
const int emergencyStopPin = 7;  // PBS3
bool emergencyStopActive = false;
bool lastEmergencyStopState = HIGH;

// Joystick threshold
const int thresholdLow = 400;
const int thresholdHigh = 600;

// Save position for resume
long savedXTarget = 0;
long savedYTarget = 0;
bool wasMovingX = false;
bool wasMovingY = false;

long savedPosition = 0;
bool isStopped = false;

float lastYValue = 0;  


bool motionCompleted = true; 


void setup() {
  Serial.begin(9600);
  Serial.println("System Ready");

  pinMode(modeButtonPin, INPUT_PULLUP);
  pinMode(emergencyStopPin, INPUT_PULLUP);

  stepperX.setMaxSpeed(2000);
  stepperX.setAcceleration(800);
  stepperX.setCurrentPosition(0);

  stepperY.setMaxSpeed(2000);
  stepperY.setAcceleration(800);
  stepperY.setCurrentPosition(0);

  myServo.attach(8);
  myServo.write(lastServoAngle);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(xMinSwitch, INPUT_PULLUP);
  pinMode(xMaxSwitch, INPUT_PULLUP);
  pinMode(yMinSwitch, INPUT_PULLUP);
}

float getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

void checkUltrasonic() {
  float distance = getUltrasonicDistance();

  if (distance < MIN_DISTANCE_CM && !systemStopped && !emergencyStopActive) {
    savedXTarget = stepperX.targetPosition();
    savedYTarget = stepperY.targetPosition();
    wasMovingX = (stepperX.distanceToGo() != 0);
    wasMovingY = (stepperY.distanceToGo() != 0);
    stepperX.stop();
    stepperY.stop();
    systemStopped = true;
    Serial.println("System STOPPED due to obstacle!");
  } else if (distance >= MIN_DISTANCE_CM && systemStopped && !emergencyStopActive) {
    systemStopped = false;
    if (wasMovingX) stepperX.moveTo(savedXTarget);
    if (wasMovingY) stepperY.moveTo(savedYTarget);
    Serial.println("System RESUMED from saved position");
  }
}

unsigned long lastEmergencyStopDebounceTime = 0;
const unsigned long emergencyStopDebounceDelay = 50; 

void checkEmergencyStop() {
  bool currentEmergencyState = (digitalRead(emergencyStopPin) == LOW); // LOW = pressed

  // Debounce logic
  if (currentEmergencyState != lastEmergencyStopState) {
    lastEmergencyStopDebounceTime = millis();
  }

  if ((millis() - lastEmergencyStopDebounceTime) > emergencyStopDebounceDelay) {
    if (currentEmergencyState != emergencyStopActive) {
      emergencyStopActive = currentEmergencyState;

      if (emergencyStopActive) {
        savedXTarget = stepperX.targetPosition();
        savedYTarget = stepperY.targetPosition();
        wasMovingX = (stepperX.distanceToGo() != 0);
        wasMovingY = (stepperY.distanceToGo() != 0);

        stepperX.stop();
        stepperY.stop();
        stepperX.disableOutputs();
        stepperY.disableOutputs();
        Serial.println("EMERGENCY_ON");
      } else {
        stepperX.enableOutputs();
        stepperY.enableOutputs();
        if (wasMovingX) stepperX.moveTo(savedXTarget);
        if (wasMovingY) stepperY.moveTo(savedYTarget);
        Serial.println("EMERGENCY_OFF");
      }
    }
  }

  lastEmergencyStopState = currentEmergencyState;
}

void processAngleCommand(String data) {
  if (emergencyStopActive) {
    Serial.println("Command ignored - Emergency stop active");
    return;
  }

  int commaIndex = data.indexOf(',');
  if (commaIndex == -1) {
    Serial.println("Invalid ANGLE command!");
    return;
  }

  float new_angle = data.substring(commaIndex + 1).toFloat();
  float delta_angle = new_angle - current_angle;

  float arc_length_mm = (PI * SHAFT_DIAMETER_MM * delta_angle) / 360.0;
  long steps = round(arc_length_mm * STEPS_PER_MM);

  stepperY.moveTo(730);
  while (stepperY.distanceToGo() != 0) {
    checkUltrasonic();
    checkEmergencyStop();
    if (!systemStopped && !emergencyStopActive) {
      stepperY.run();
    } else {
      delay(10);
    }
  }
  delay(100);

  stepperX.moveTo(stepperX.currentPosition() + (steps * 10));
  while (stepperX.distanceToGo() != 0) {
    checkUltrasonic();
    checkEmergencyStop();
    if (!systemStopped && !emergencyStopActive) {
      stepperX.run();
    } else {
      delay(10);
    }
  }
  delay(100);

  stepperY.moveTo(-12);
  while (stepperY.distanceToGo() != 0) {
    checkUltrasonic();
    checkEmergencyStop();
    if (!systemStopped && !emergencyStopActive) {
      stepperY.run();
    } else {
      delay(10);
    }
  }
  delay(100);

  current_angle = new_angle;
  // Serial.println("Done");
}

void processCoordinateCommand(String data) {
  if (emergencyStopActive) {
    // Serial.println("EMERGENCY_ON");
    return;
  }

  else if (data.indexOf(',') > 0) {
  int commaIndex = data.indexOf(',');
  float newX_mm = data.substring(0, commaIndex).toFloat();
  float inputY_angle_ratio = data.substring(commaIndex + 1).toFloat();

  float phi_y = (inputY_angle_ratio / HEAD_RADIUS_MM) * (PI / 2);
  float y_pos_mm = ARM_LENGTH_MM * sin(phi_y);
  long targetY_steps = round(y_pos_mm * STEPS_PER_MM);

  // float deltaX = (newX_mm - currentX_mm) * 4;
  // long targetX_steps = round(deltaX * STEPS_PER_MM);

  // if (targetX_steps > stepperX.currentPosition()) {
  //   stepperX.moveTo(stepperX.currentPosition() + (targetX_steps + 60));
  // } else {
  //   stepperX.moveTo(stepperX.currentPosition() + (targetX_steps - 60));
  // }

  // while (stepperX.distanceToGo() != 0) {
  //   checkUltrasonic();
  //   if (!systemStopped) stepperX.run();
  // }
  float deltaX = (newX_mm - currentX_mm) * 4;
  long targetX_steps = round(deltaX * STEPS_PER_MM);

  // أضف هذا الشرط قبل التحريك
  if (abs(targetX_steps) > 5) { 
    if (targetX_steps > 0) {
      stepperX.moveTo(stepperX.currentPosition() + (targetX_steps + 60));
    } else {
      stepperX.moveTo(stepperX.currentPosition() + (targetX_steps - 60));
    }
  }
    while (stepperX.distanceToGo() != 0) {
    checkUltrasonic();
    checkEmergencyStop();

    if (emergencyStopActive) {
      // Serial.println("EMERGENCY_ON");
      continue;
    }

    if (!systemStopped) stepperX.run();
  }

  delay(50);

  int yOffset = 30;
  if (lastYValue != 0) {
    if (inputY_angle_ratio < lastYValue) {
      yOffset = 20;
    } else {
      yOffset = 30;
    }
  }

  stepperY.moveTo(targetY_steps + yOffset);

  while (stepperY.distanceToGo() != 0) {
  checkUltrasonic();
  checkEmergencyStop();

  if (emergencyStopActive) {
    // Serial.println("EMERGENCY_ON");
    continue;
  }

  if (!systemStopped) stepperY.run();
}    
   if (systemStopped || emergencyStopActive) {
    Serial.println("Servo move skipped due to stop condition!");
  } else {
    targetServoAngle = 0;
    isServoMoving = true;
  }


  currentX_mm = newX_mm;
  lastYValue = inputY_angle_ratio;
  // Serial.println("Done");
  motionCompleted = false;

}
}

void stepMotor(int stepPin) {
  if (emergencyStopActive) return;
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(500);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(500);
}

void stepMotorY(int stepPin) {
  if (emergencyStopActive) return;
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(1100);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(1100);
}

void loop() {
  checkEmergencyStop();

  bool currentButtonState = digitalRead(modeButtonPin);
  if (lastButtonState == HIGH && currentButtonState == LOW && !emergencyStopActive) {
    // زر ضغط
    controlModeJoystick = !controlModeJoystick; 
    Serial.print("Control mode changed to: ");
    Serial.println(controlModeJoystick ? "Joystick" : "Serial");
    delay(300);  
  }
  lastButtonState = currentButtonState;

  checkUltrasonic();

  if (!systemStopped && !emergencyStopActive) {
    if (controlModeJoystick) {
      int xValue = analogRead(joystickX);
      int yValue = analogRead(joystickY);

      bool xMinPressed = digitalRead(xMinSwitch) == LOW;
      bool xMaxPressed = digitalRead(xMaxSwitch) == LOW;
      bool yMinPressed = digitalRead(yMinSwitch) == LOW;

      if (xValue < thresholdLow && !xMinPressed) {
        digitalWrite(3, LOW); 
        stepMotor(2);
      } else if (xValue > thresholdHigh && !xMaxPressed) {
        digitalWrite(3, HIGH); 
        stepMotor(2);
      }

      if (yValue < thresholdLow && !yMinPressed) {
        digitalWrite(5, LOW);  
        stepMotorY(4);
      } else if (yValue > thresholdHigh) {
        digitalWrite(5, HIGH); 
        stepMotorY(4);
      }
    } else {
      if (Serial.available()) {
        String data = Serial.readStringUntil('\n');
        Serial.println("Received: [" + data + "]");
        data.trim();

        if (data.startsWith("ANGLE")) {
          processAngleCommand(data);
        } else if (data.indexOf(',') > 0) {
          processCoordinateCommand(data);
        }
      }
    }

    stepperX.run();
    stepperY.run();

    if (!motionCompleted &&
    stepperX.distanceToGo() == 0 &&
    stepperY.distanceToGo() == 0 &&
    !isServoMoving &&
    !emergencyStopActive &&
    !systemStopped) {
  Serial.println("Done");
  motionCompleted = true;
}
  }
 if (isServoMoving && !emergencyStopActive) {
    if (!systemStopped) {
      if (lastServoAngle != targetServoAngle) {
        lastServoAngle += (lastServoAngle > targetServoAngle) ? -1 : 1;
        myServo.write(lastServoAngle);
        delay(1);
      } else {
        if (targetServoAngle == 0) {
          delay(500);
          targetServoAngle = 180;
        } else {
          isServoMoving = false;
        }
      }
    }
  }
}
