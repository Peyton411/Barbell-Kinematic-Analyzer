#include <Wire.h>

#define MPU 0x68

float gx_offset = 0, gy_offset = 0, gz_offset = 0;
float ax_offset = 0, ay_offset = 0, az_offset = 0;

void calibrate() {
  Serial.println("Calibrating - keep still...");
  long sumGX = 0, sumGY = 0, sumGZ = 0;
  long sumAX = 0, sumAY = 0, sumAZ = 0;
  int samples = 1000;

  for (int i = 0; i < samples; i++) {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);  // Start at accel
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 14, true);  // accel + temp + gyro

    sumAX += (int16_t)(Wire.read() << 8 | Wire.read());
    sumAY += (int16_t)(Wire.read() << 8 | Wire.read());
    sumAZ += (int16_t)(Wire.read() << 8 | Wire.read());
    Wire.read(); Wire.read();  // skip temp
    sumGX += (int16_t)(Wire.read() << 8 | Wire.read());
    sumGY += (int16_t)(Wire.read() << 8 | Wire.read());
    sumGZ += (int16_t)(Wire.read() << 8 | Wire.read());
    delay(3);
  }

  ax_offset = (sumAX / samples) / 16384.0;
  ay_offset = (sumAY / samples) / 16384.0;
  az_offset = (sumAZ / samples) / 16384.0 - 1.0;
  gx_offset = (sumGX / samples) / 131.0;
  gy_offset = (sumGY / samples) / 131.0;
  gz_offset = (sumGZ / samples) / 131.0;

  Serial.println("Done.");
}

void setup() {
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  for (int i = 0; i < 9; i++) {
    digitalWrite(A5, HIGH);
    delayMicroseconds(5);
    digitalWrite(A5, LOW);
    delayMicroseconds(5);
  }
  digitalWrite(A4, HIGH);
  digitalWrite(A5, HIGH);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  Wire.begin();
  Serial.begin(9600);

  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  delay(100);
  calibrate();
}

void loop() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();
  int16_t tmp = Wire.read() << 8 | Wire.read();
  int16_t gx = Wire.read() << 8 | Wire.read();
  int16_t gy = Wire.read() << 8 | Wire.read();
  int16_t gz = Wire.read() << 8 | Wire.read();

  Serial.print("Accel (g) X: "); Serial.print((ax / 16384.0) - ax_offset);
  Serial.print("  Y: ");         Serial.print((ay / 16384.0) - ay_offset);
  Serial.print("  Z: ");         Serial.println((az / 16384.0) - az_offset);

  Serial.print("Gyro (deg/s) X: "); Serial.print((gx / 131.0) - gx_offset);
  Serial.print("  Y: ");            Serial.print((gy / 131.0) - gy_offset);
  Serial.print("  Z: ");            Serial.println((gz / 131.0) - gz_offset);

  Serial.println();
  delay(500);
}