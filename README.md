# Barbell Kinematics Analyzer

Description: A system to track barbell velocity through an IMU and approximate proximity to failure.

# Project planning
1. Hardware Selection & Interfacing:** Select, solder, and wire the components
2. Raw Sensor Evaluation:** Get raw IMU readings running
3. Live Telemetry Streaming:** Log wireless data packages through the BLE into csv files
4. Rep Detection & Metric Tracking:** Breaking the reps into concentric and eccentric to count reps, get useful metrics like average velocity
5. Physical Mounting:** Model and 3D print a way to mount the system onto the barbel
6. Estimate proximity to failure:** Predict failure points using a simple ML model

## Hardware Architecture & Physical Wiring

Components and usages:
Arduino Nano - microcontroller
MPU6050 - accelerometer and gyroscope
HM-10 BLE Module - wireless capacities
Logic level convertor - makes BLE work with Nano
Power supply - Ekrist 25800mAh Battery Portable Charger 

Because the BLE takes 3.3V and the arduino sends 5V, a logic level converter had to be integrated, which controlled the voltage to save the BLE.

# Wiring


MPU6050:

**VCC - Nano 5V

GND - NANO GND

SDA - NANO A4

SCL - Nano A5

BLE HM-10 RX:

VCC - Nano 3.3V  

GND - GND Nano

TXD - LLC LV2

RXD - LLC LV1

Logic level convertor:

HV:

HV1 - Nano D2

HV2 - Nano D3

LV:

LV1 - HM-10 RX

LV2 - HM-10 TX


Note on wiring- During the physical fabrication stage, the GND and VCC pins were scarce, so I made Y-splices to distribute power to the MPU6050, HM-10 BLE, and Logic Level Converter.


## Software and Engineering Development Logs

### 1. Getting Started Up
**Problem** Whenever I started up a prototype of just the IMU and Nano, the IMU would glitch because the Nano and IMU started up at different times.
**Fail** The first MPU6050 was fried while soldering pins on. The ruined board was used to practice soldering.
**Fix** Wrote code to check if there was a connection to the IMU at address `0x68`. To prevent it from getting stuck booting up, code turning the SCL clock up and down 9 times was implement to wake the IMU back up.

### 2. Sensor Fusion
* **The Problem:** Sensor noise and tilting the IMU caused infinite velocity drift because the orientation was not taken into account.
* **The Fix:** Added a Complementary Filter combining gyroscope angles with accelerometer readings to track tilt and orientation angles. Began subtracting gravity vectors to get true velocity. A  deadband limit paired with ae stillness check zeroes out tilt when the sensor rests between reps.

## BLE Connection and Data Logging


### 1. BLE Communication Debugging

* **The Problem:** Although the HM-10 was pairing successfully, no sensor data was being received by the laptop.
* **The Fix:** Traced the issue to an incorrect TX/RX wiring configuration between the Arduino Nano and HM-10 BLE module. After rewiring the connections, the communication was restored.
* **The Result:** The BLE module was able to reliably transmit sensor data from the Arduino to the laptop.

### 2.. Wireless Data Logging

* **The Problem:** The project required a way to collect sensor data wirelessly and store it in a format suitable for later analysis.
* **The Fix:** Modified a Python logging script to receive BLE packets and save them directly into CSV files (https://github.com/tarik-ibrahimovic/hweeai_IDK/blob/835bf81b631bc8ac133e5d8dc36f2bbcbaa82166/bt_csv.py#L24). A simple handshake protocol was implemented, requiring commands from the laptop before calibration and data collection could begin. Timestamps were transmitted alongside velocity measurements.
* **The Result:** Timestamped sensor data could be streamed wirelessly and be written into csv files.

### 3. Packet Structure & Reliability

* **The Problem:** BLE transmissions can arrive in incomplete chunks, creating the possibility of corrupted or misaligned data packets.
* **The Fix:** Instead of implementing a start-byte synchronization system, a software buffer was added on the Python side to wait until a complete packet had been received before processing it. Existing BLE logging software was adapted by modifying the Arduino packet format to match the expected structure.
* **The Result:** Reliable packet reception was achieved without requiring additional packet framing logic.

### 4. Data Processing Automation

* **The Problem:** To maintain compatibility with the logging software, several unused placeholder values were included in transmitted packets, resulting in unnecessary columns in exported CSV files.
* **The Fix:** Wrote a secondary Python utility to automatically remove unused columns and generate cleaner datasets.
* **The Result:** Data could be exported into a simplified format better suited for analysis and future machine learning applications.



## CAD Modeling the Battery Case and Electronics box (Opted to use commercial barbell clips and attach the separate components)

Designed a model to keep the  Ekrist 25800mAh battery pack ($3"L \times 4.7"W \times 1"Th) in place and safe during lifts through Fusion360.

The design changed across three revisions to abide by physical constraints:
1. **Revision 1 (A box):** A basic rectangular shell with 0.1 inches of clearance in all directions, tailored to the dimensions of the battery.
2. **Revision 2 (Heat and Strength management):** Thickened the base to 0.25 inches to handle barbell drops. Added fillets to help absorb impact and left at least 30% of the battery surface area exposed to prevent thermal overheating, as recommended. And added access to the power button and USB ports.
3. **Revision 3 (Slicing):** To avoid steep overhang failures and messy prints, the casing was modified with a U shaped cut instead of a gaping hole, allowing it to print easier.






