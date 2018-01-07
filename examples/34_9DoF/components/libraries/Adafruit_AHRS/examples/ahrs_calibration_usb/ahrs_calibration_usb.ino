#include <Wire.h>
#include <Adafruit_Sensor.h>

#define ST_LSM303DLHC_L3GD20        (0)
#define ST_LSM9DS1                  (1)
#define NXP_FXOS8700_FXAS21002      (2)

// Define your target sensor(s) here based on the list above!
// #define AHRS_VARIANT    ST_LSM303DLHC_L3GD20
#define AHRS_VARIANT   NXP_FXOS8700_FXAS21002

// Include appropriate sensor driver(s)
#if AHRS_VARIANT == ST_LSM303DLHC_L3GD20
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_LSM303_U.h>
#elif AHRS_VARIANT == ST_LSM9DS1
// ToDo!
#elif AHRS_VARIANT == NXP_FXOS8700_FXAS21002
#include <Adafruit_FXAS21002C.h>
#include <Adafruit_FXOS8700.h>
#else
#error "AHRS_VARIANT undefined! Please select a target sensor combination!"
#endif

// Create sensor instances.
#if AHRS_VARIANT == ST_LSM303DLHC_L3GD20
Adafruit_L3GD20_Unified       gyro(20);
Adafruit_LSM303_Accel_Unified accel(30301);
Adafruit_LSM303_Mag_Unified   mag(30302);
#elif AHRS_VARIANT == ST_LSM9DS1
// ToDo!
#elif AHRS_VARIANT == NXP_FXOS8700_FXAS21002
Adafruit_FXAS21002C gyro = Adafruit_FXAS21002C(0x0021002C);
Adafruit_FXOS8700 accelmag = Adafruit_FXOS8700(0x8700A, 0x8700B);
#endif

// NOTE: THIS IS A WORK IN PROGRESS!

// This sketch can be used to output raw sensor data in a format that
// can be understoof by MotionCal from PJRC. Download the application
// from http://www.pjrc.com/store/prop_shield.html and make note of the
// magentic offsets after rotating the sensors sufficiently.
//
// You should end up with 3 offsets for X/Y/Z, which are displayed
// in the top-right corner of the application.

void setup()
{
  Serial.begin(115200);

  // Wait for the Serial Monitor to open (comment out to run without Serial Monitor)
  // while(!Serial);

  Serial.println(F("Adafruit nDOF AHRS Calibration Example")); Serial.println("");

  // Initialize the sensors.
  if(!gyro.begin())
  {
    /* There was a problem detecting the gyro ... check your connections */
    Serial.println("Ooops, no gyro detected ... Check your wiring!");
    while(1);
  }

#if AHRS_VARIANT == NXP_FXOS8700_FXAS21002
  if(!accelmag.begin(ACCEL_RANGE_4G))
  {
    Serial.println("Ooops, no FXOS8700 detected ... Check your wiring!");
    while(1);
  }
#else
  if (!accel.begin())
  {
    /* There was a problem detecting the accel ... check your connections */
    Serial.println("Ooops, no accel detected ... Check your wiring!");
    while (1);
  }

  if (!mag.begin())
  {
    /* There was a problem detecting the mag ... check your connections */
    Serial.println("Ooops, no mag detected ... Check your wiring!");
    while (1);
  }
#endif
}

void loop(void)
{
  sensors_event_t event; // Need to read raw data, which is stored at the same time

    // Get new data samples
    gyro.getEvent(&event);
#if AHRS_VARIANT == NXP_FXOS8700_FXAS21002
    accelmag.getEvent(&event);
#else
    accel.getEvent(&event);
    mag.getEvent(&event);
#endif

  // Print the sensor data
  Serial.print("Raw:");
#if AHRS_VARIANT == NXP_FXOS8700_FXAS21002
    Serial.print(accelmag.accel_raw.x);
    Serial.print(',');
    Serial.print(accelmag.accel_raw.y);
    Serial.print(',');
    Serial.print(accelmag.accel_raw.z);
    Serial.print(',');
#else
    Serial.print(accel.raw.x);
    Serial.print(',');
    Serial.print(accel.raw.y);
    Serial.print(',');
    Serial.print(accel.raw.z);
    Serial.print(',');
#endif
    Serial.print(gyro.raw.x);
    Serial.print(',');
    Serial.print(gyro.raw.y);
    Serial.print(',');
    Serial.print(gyro.raw.z);
    Serial.print(',');
#if AHRS_VARIANT == NXP_FXOS8700_FXAS21002
    Serial.print(accelmag.mag_raw.x);
    Serial.print(',');
    Serial.print(accelmag.mag_raw.y);
    Serial.print(',');
    Serial.print(accelmag.mag_raw.z);
    Serial.println();
#else
    Serial.print(mag.raw.x);
    Serial.print(',');
    Serial.print(mag.raw.y);
    Serial.print(',');
    Serial.print(mag.raw.z);
    Serial.println();
#endif

  delay(50);
}
