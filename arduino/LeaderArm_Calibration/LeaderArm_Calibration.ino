#include <Arduino.h>

struct JointCalibration {
  int rawMin;
  int rawMax;
  float angleMinDeg;
  float angleMaxDeg;
};

struct AxisConfig {
  const char* jointName;
  uint8_t pin;
  const char* firstPrompt;
  const char* secondPrompt;
  float firstAngleDeg;
  float secondAngleDeg;
};

const uint8_t PIN_BASIS    = A0;
const uint8_t PIN_MAIN_ARM = A1;
const uint8_t PIN_FORE_ARM = A2;
const uint8_t PIN_WRIST    = A3;

// ======================================================
// Adjust the end angles here if needed.
// These angles are printed again at the end so you can
// paste them directly into the main streaming sketch.
// ======================================================
AxisConfig axes[] = {
  {
    "basis",
    PIN_BASIS,
    "Move the BASIS fully to the left and press Enter...",
    "Move the BASIS fully to the right and press Enter...",
    -150.0f,
     150.0f
  },
  {
    "mainArm",
    PIN_MAIN_ARM,
    "Move the MAIN ARM fully backward and press Enter...",
    "Move the MAIN ARM fully forward and press Enter...",
    -90.0f,
     90.0f
  },
  {
    "foreArm",
    PIN_FORE_ARM,
    "Move the FORE ARM fully backward and press Enter...",
    "Move the FORE ARM fully forward and press Enter...",
    -90.0f,
     90.0f
  },
  {
    "wrist",
    PIN_WRIST,
    "Move the WRIST fully downward and press Enter...",
    "Move the WRIST fully upward and press Enter...",
    -90.0f,
     90.0f
  }
};
// ======================================================

const uint8_t AXIS_COUNT = sizeof(axes) / sizeof(axes[0]);
JointCalibration cal[AXIS_COUNT];

int readStableAnalog(uint8_t pin, uint8_t samples = 20) {
  long sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return (int)(sum / samples);
}

void flushSerialInput() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void waitForEnter() {
  while (true) {
    while (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        delay(50);
        flushSerialInput();
        return;
      }
    }
  }
}

void calibrateAxis(uint8_t index) {
  AxisConfig& a = axes[index];

  Serial.println();
  Serial.print("=== Calibration ");
  Serial.print(a.jointName);
  Serial.println(" ===");

  Serial.println(a.firstPrompt);
  waitForEnter();
  int raw1 = readStableAnalog(a.pin);
  Serial.print("Measured raw = ");
  Serial.println(raw1);

  Serial.println();
  Serial.println(a.secondPrompt);
  waitForEnter();
  int raw2 = readStableAnalog(a.pin);
  Serial.print("Measured raw = ");
  Serial.println(raw2);

  if (raw1 <= raw2) {
    cal[index].rawMin = raw1;
    cal[index].rawMax = raw2;
    cal[index].angleMinDeg = a.firstAngleDeg;
    cal[index].angleMaxDeg = a.secondAngleDeg;
  } else {
    cal[index].rawMin = raw2;
    cal[index].rawMax = raw1;
    cal[index].angleMinDeg = a.secondAngleDeg;
    cal[index].angleMaxDeg = a.firstAngleDeg;
  }

  Serial.println("Saved.");
}

void printPasteBlock() {
  Serial.println();
  Serial.println("============================================");
  Serial.println("COPY AND PASTE THIS BLOCK INTO THE MAIN CODE");
  Serial.println("============================================");
  Serial.println();

  Serial.print("const JointCalibration BASIS_CAL    = {");
  Serial.print(cal[0].rawMin);
  Serial.print(", ");
  Serial.print(cal[0].rawMax);
  Serial.print(", ");
  Serial.print(cal[0].angleMinDeg, 2);
  Serial.print("f, ");
  Serial.print(cal[0].angleMaxDeg, 2);
  Serial.println("f};");

  Serial.print("const JointCalibration MAIN_ARM_CAL = {");
  Serial.print(cal[1].rawMin);
  Serial.print(", ");
  Serial.print(cal[1].rawMax);
  Serial.print(", ");
  Serial.print(cal[1].angleMinDeg, 2);
  Serial.print("f, ");
  Serial.print(cal[1].angleMaxDeg, 2);
  Serial.println("f};");

  Serial.print("const JointCalibration FORE_ARM_CAL = {");
  Serial.print(cal[2].rawMin);
  Serial.print(", ");
  Serial.print(cal[2].rawMax);
  Serial.print(", ");
  Serial.print(cal[2].angleMinDeg, 2);
  Serial.print("f, ");
  Serial.print(cal[2].angleMaxDeg, 2);
  Serial.println("f};");

  Serial.print("const JointCalibration WRIST_CAL    = {");
  Serial.print(cal[3].rawMin);
  Serial.print(", ");
  Serial.print(cal[3].rawMax);
  Serial.print(", ");
  Serial.print(cal[3].angleMinDeg, 2);
  Serial.print("f, ");
  Serial.print(cal[3].angleMaxDeg, 2);
  Serial.println("f};");

  Serial.println();
  Serial.println("Copy the calibration values above into the main sketch.");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("INTERACTIVE CALIBRATION - LEADER ARM");
  Serial.println("For each joint: move to position 1 -> press Enter, then move to position 2 -> press Enter.");

  for (uint8_t i = 0; i < AXIS_COUNT; i++) {
    calibrateAxis(i);
  }

  printPasteBlock();
}

void loop() {
}
