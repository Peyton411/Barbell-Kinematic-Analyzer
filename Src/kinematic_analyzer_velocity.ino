#include <Wire.h>
#include <math.h>
#include <SoftwareSerial.h>
#define MPU 0x68
union DataPacket {
  struct {
    float now_val;
    float vz_val;
    float fake1;
    float fake2;
    float fake3;
    float fake4;
  } values;
  byte bytes[24];
};
SoftwareSerial bleSerial(2,3);
float gx_offset = 0, gy_offset = 0, gz_offset = 0;
float ax_offset = 0, ay_offset = 0, az_offset = 0;
float vz = 0;
float anglex = 0, angley = 0;
unsigned long lastTime = 0;
unsigned long lastBleSend = 0; 
const unsigned long bleInterval = 50; //time between transmission
bool liftStart = false; // Is the lift started?
bool connection = false; //is the BLE connected?

void calibrate() {
  Serial.println("Calibrating - keep still...");
  long sumGX = 0, sumGY = 0, sumGZ = 0;
  long sumAX = 0, sumAY = 0, sumAZ = 0;
  int samples = 1000;

  for (int i = 0; i < samples; i++) {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 14, true);

    sumAX += (int16_t)(Wire.read() << 8 | Wire.read());
    sumAY += (int16_t)(Wire.read() << 8 | Wire.read());
    sumAZ += (int16_t)(Wire.read() << 8 | Wire.read());
    Wire.read(); Wire.read(); // skip temp
    sumGX += (int16_t)(Wire.read() << 8 | Wire.read());
    sumGY += (int16_t)(Wire.read() << 8 | Wire.read());
    sumGZ += (int16_t)(Wire.read() << 8 | Wire.read());
    delay(3);
  }

  ax_offset = (sumAX / samples) / 16384.0;
  ay_offset = (sumAY / samples) / 16384.0;
  az_offset = (sumAZ / samples) / 16384.0 - 1.0; // bias only tje gravity is handled separately
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
  bleSerial.begin(9600);

  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  delay(100);
  Serial.println("Waiting for connection...");
  while (!connection) {
    if (bleSerial.available() > 0) {
      char incomingsignal = bleSerial.read();
      if (incomingsignal == 'C') { // 'C' for connected to my laptop
        connection = true;
      }
    }
    delay(10);
  }
  Serial.println("Connection established");
  while (!liftStart) {
    if (bleSerial.available() > 0) {
      char incomingsignal2 = bleSerial.read();
      if (incomingsignal2 == 'S') { // 'S' for starting the lift
        liftStart = true;
      }
    }
    delay(10);
  }
  calibrate();
  lastTime = millis();
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

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  // calibrated values - az_offset has bias only, so az_real  ks always around 1.0g at rest flat
  float ax_real = (ax / 16384.0) - ax_offset;
  float ay_real = (ay / 16384.0) - ay_offset;
  float az_real = (az / 16384.0) - az_offset;
  float gx_deg = (gx / 131.0) - gx_offset;
  float gy_deg = (gy / 131.0) - gy_offset;

  // checks for stillness
  // regardless of orientation, It will change when the bar accelerates
  float totalAccel = sqrt(ax_real*ax_real + ay_real*ay_real + az_real*az_real);
  bool isStill = (abs(totalAccel - 1.0) < 0.07);

  // only update tilt estimate when still - during movement accel is screwed up
  // by linear acceleration and gives wrong angles
  if (isStill) { 
    float accelAngleX = atan2(ay_real, az_real) * 180.0 / PI;
    float accelAngleY = atan2(ax_real, az_real) * 180.0 / PI;
    anglex = 0.98 * (anglex + gx_deg * dt) + 0.02 * accelAngleX;
    angley = 0.98 * (angley + gy_deg * dt) + 0.02 * accelAngleY;
  }

  // gravity vector from current tilt estimate (trig)
  float gravityZ = cos(anglex * PI / 180.0) * cos(angley * PI / 180.0);

  // subtract gravity to get real linear acceleration, convert to m/s^2
  float linear_az = (az_real - gravityZ) * 9.81;

  // small deadband for noise
  if (abs(linear_az) < 0.02) linear_az = 0;

  // adding
  vz += linear_az * dt;

  // bleed when still, which means just letting the velocity decrease
  if (isStill) {
    vz *= 0.9;
  }

  // reset near zero
  if (abs(vz) < 0.02) vz = 0;
static unsigned long stillStart = 0;
bool nearStill = (abs(vz) < 0.05) && (abs(totalAccel - 1.0) < 0.03);

if (nearStill) {
  if (stillStart == 0) stillStart = millis();

  if (millis() - stillStart > 200) {
    vz = 0;
  }
} else {
  stillStart = 0;
}
DataPacket packet;
packet.values.now_val = (float)now; // Turns unsigned long time into a float
packet.values.vz_val  = vz;   // Velocity
packet.values.fake1  = 0.0;    // Fills the fakes with zero
packet.values.fake2  = 0.0;
packet.values.fake3  = 0.0;
packet.values.fake4  = 0.0;

// Write out 24 bytes
bleSerial.write(packet.bytes, 24);
  delay(50);
}
