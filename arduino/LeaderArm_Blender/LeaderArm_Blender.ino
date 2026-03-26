#include <Arduino.h>

struct JointCalibration {
  int rawMin;
  int rawMax;
  float angleMinDeg;
  float angleMaxDeg;
};

const uint8_t PIN_BASIS    = A0;
const uint8_t PIN_MAIN_ARM = A1;
const uint8_t PIN_FORE_ARM = A2;
const uint8_t PIN_WRIST    = A3;

const unsigned long SEND_INTERVAL_MS = 20;

// ======================================================
// Paste the block from LeaderArm_Calibration.ino here.
// The values below are example values so the sketch
// compiles out of the box.
// ======================================================
const JointCalibration BASIS_CAL    = {0, 1023, 150.00f, -150.00f};
const JointCalibration MAIN_ARM_CAL = {204, 820, 90.00f, -90.00f};
const JointCalibration FORE_ARM_CAL = {204, 820, 90.00f, -90.00f};
const JointCalibration WRIST_CAL    = {204, 820, -90.00f, 90.00f};
// ======================================================

// Set to true to print extra debug information.
const bool PRINT_DEBUG = false;

unsigned long lastSendMs = 0;

float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

int readFilteredAnalog(uint8_t pin, uint8_t samples = 4) {
  long sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(500);
  }
  return (int)(sum / samples);
}

float rawToAngleDeg(int raw, const JointCalibration& cal) {
  if (cal.rawMax == cal.rawMin) {
    return cal.angleMinDeg;
  }

  float t = (float)(raw - cal.rawMin) / (float)(cal.rawMax - cal.rawMin);
  t = clampFloat(t, 0.0f, 1.0f);
  return cal.angleMinDeg + t * (cal.angleMaxDeg - cal.angleMinDeg);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("LEADER ARM SERIAL STREAM");
  Serial.println("CSV format: basis,main,fore,wrist");
}

void loop() {
  if (millis() - lastSendMs < SEND_INTERVAL_MS) {
    return;
  }
  lastSendMs = millis();

  int rawBasis = readFilteredAnalog(PIN_BASIS);
  int rawMain  = readFilteredAnalog(PIN_MAIN_ARM);
  int rawFore  = readFilteredAnalog(PIN_FORE_ARM);
  int rawWrist = readFilteredAnalog(PIN_WRIST);

  float basisDeg = rawToAngleDeg(rawBasis, BASIS_CAL);
  float mainDeg  = rawToAngleDeg(rawMain, MAIN_ARM_CAL);
  float foreDeg  = rawToAngleDeg(rawFore, FORE_ARM_CAL);
  float wristDeg = rawToAngleDeg(rawWrist, WRIST_CAL);

  if (PRINT_DEBUG) {
    Serial.print("RAW: ");
    Serial.print(rawBasis);
    Serial.print(", ");
    Serial.print(rawMain);
    Serial.print(", ");
    Serial.print(rawFore);
    Serial.print(", ");
    Serial.print(rawWrist);
    Serial.print(" | ANGLES: ");
  }

  Serial.print(basisDeg, 2);
  Serial.print(",");
  Serial.print(mainDeg, 2);
  Serial.print(",");
  Serial.print(foreDeg, 2);
  Serial.print(",");
  Serial.println(wristDeg, 2);
}
