#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Mahony.h"
#include "Madgwick.h"
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

// Note: This sketch is a WORK IN PROGRESS

#define BLUEFRUIT51_ST_LSM303DLHC_L3GD20        (0)
#define BLUEFRUIT51_ST_LSM9DS1                  (1)
#define BLUEFRUIT51_NXP_FXOS8700_FXAS21002      (2)

// Define your target sensor(s) here based on the list above!
// #define AHRS_VARIANT    BLUEFRUIT51_ST_LSM303DLHC_L3GD20
#define AHRS_VARIANT   BLUEFRUIT51_NXP_FXOS8700_FXAS21002

// Config
#define FACTORYRESET_ENABLE      1

#if AHRS_VARIANT == BLUEFRUIT51_ST_LSM303DLHC_L3GD20
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_LSM303_U.h>
#elif AHRS_VARIANT == BLUEFRUIT51_ST_LSM9DS1
// ToDo!
#elif AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
#include <Adafruit_FXAS21002C.h>
#include <Adafruit_FXOS8700.h>
#else
#error "AHRS_VARIANT undefined! Please select a target sensor combination!"
#endif

// Create sensor instances.
#if AHRS_VARIANT == BLUEFRUIT51_ST_LSM303DLHC_L3GD20
Adafruit_L3GD20_Unified       gyro(20);
Adafruit_LSM303_Accel_Unified accel(30301);
Adafruit_LSM303_Mag_Unified   mag(30302);
#elif AHRS_VARIANT == BLUEFRUIT51_ST_LSM9DS1
// ToDo!
#elif AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
Adafruit_FXAS21002C gyro = Adafruit_FXAS21002C(0x0021002C);
Adafruit_FXOS8700 accelmag = Adafruit_FXOS8700(0x8700A, 0x8700B);
#endif

// Bluetooth
// ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// Mag calibration values are calculated via ahrs_calibration.
// These values must be determined for each baord/environment.
// See the image in this sketch folder for the values used
// below.

// Offsets applied to raw x/y/z mag values
float mag_offsets[3]            = { 2.45F, -4.55F, -26.93F };

// Soft iron error compensation matrix
float mag_softiron_matrix[3][3] = { {  0.961,  -0.001,  0.025 },
                                    {  0.001,  0.886,  0.015 },
                                    {  0.025,  0.015,  1.176 } };

float mag_field_strength        = 44.12F;

// Offsets applied to compensate for gyro zero-drift error for x/y/z
// Raw values converted to rad/s based on 250dps sensitiviy (1 lsb = 0.00875 rad/s)
float rawToDPS = 0.00875F;
float dpsToRad = 0.017453293F;
float gyro_zero_offsets[3]      = { 175.0F * rawToDPS * dpsToRad,
                                   -729.0F * rawToDPS * dpsToRad,
                                    101.0F * rawToDPS * dpsToRad };


// Mahony is lighter weight as a filter and should be used
// on slower systems
Mahony filter;
//Madgwick filter;

void setup()
{
  Serial.begin(115200);

  // Wait for the Serial Monitor to open (comment out to run without Serial Monitor)
  //while(!Serial);

  Serial.println(F("Adafruit AHRS BLE Fusion Example")); Serial.println("");

  // Initialize the sensors.
  if(!gyro.begin())
  {
    /* There was a problem detecting the gyro ... check your connections */
    Serial.println("Ooops, no gyro detected ... Check your wiring!");
    while(1);
  }

#if AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
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

  // Filter expects 25 samples per second
  // The filter only seems to be responsive with a much smaller value, though!
  // ToDo: Figure out the disparity between actual refresh rate and the value
  // provided here!
  filter.begin(5);

    // Initialise the module
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    Serial.println(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  // Factory Reset
  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      Serial.println(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  /* Wait for a connection before starting the test */
  Serial.println("Waiting for a BLE connection to continue ...");

  ble.verbose(false);  // debug info is a little annoying after this point!

  // Wait for the connection to complete
  delay(1000);

  Serial.println(F("CONNECTED!"));
  Serial.println(F("**********"));

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));
}

void loop(void)
{
  sensors_event_t gyro_event;
  sensors_event_t accel_event;
  sensors_event_t mag_event;

  while ( ble.isConnected() ) {

    // Get new data samples
    gyro.getEvent(&gyro_event);
#if AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
    accelmag.getEvent(&accel_event, &mag_event);
#else
    accel.getEvent(&accel_event);
    mag.getEvent(&mag_event);
#endif

    // Apply mag offset compensation (base values in uTesla)
    float x = mag_event.magnetic.x - mag_offsets[0];
    float y = mag_event.magnetic.y - mag_offsets[1];
    float z = mag_event.magnetic.z - mag_offsets[2];

    // Apply mag soft iron error compensation
    float mx = x * mag_softiron_matrix[0][0] + y * mag_softiron_matrix[0][1] + z * mag_softiron_matrix[0][2];
    float my = x * mag_softiron_matrix[1][0] + y * mag_softiron_matrix[1][1] + z * mag_softiron_matrix[1][2];
    float mz = x * mag_softiron_matrix[2][0] + y * mag_softiron_matrix[2][1] + z * mag_softiron_matrix[2][2];

    // Apply gyro zero-rate error compensation
    float gx = gyro_event.gyro.x - gyro_zero_offsets[0];
    float gy = gyro_event.gyro.y - gyro_zero_offsets[1];
    float gz = gyro_event.gyro.z - gyro_zero_offsets[2];

    // The filter library expects gyro data in degrees/s, but adafruit sensor
    // uses rad/s so we need to convert them first (or adapt the filter lib
    // where they are being converted)
    gx *= 57.2958F;
    gy *= 57.2958F;
    gz *= 57.2958F;

    // Update the filter
    filter.update(gx, gy, gz,
                  accel_event.acceleration.x, accel_event.acceleration.y, accel_event.acceleration.z,
                  mx, my, mz);

  /*
    // Print the orientation filter output
    float roll = filter.getRoll();
    float pitch = filter.getPitch();
    float heading = filter.getYaw();
    Serial.print(millis());
    Serial.print(" - Orientation: ");
    Serial.print(pitch);
    Serial.print(" ");
    Serial.print(heading);
    Serial.print(" ");
    Serial.println(roll);

    // Send sensor data to Bluetooth
    char prefix[] = "V";
    ble.write(prefix, strlen(prefix));
    ble.write((uint8_t *)&pitch, sizeof(float));
    ble.write((uint8_t *)&heading, sizeof(float));
    ble.write((uint8_t *)&roll, sizeof(float));
  */

    // Print the orientation filter output in quaternions.
    // This avoids the gimbal lock problem with Euler angles when you get
    // close to 180 degrees (causing the model to rotate or flip, etc.)
    float qw, qx, qy, qz;
    filter.getQuaternion(&qw, &qx, &qy, &qz);
    Serial.print(millis());
    Serial.print(" - Quat: ");
    Serial.print(qw);
    Serial.print(" ");
    Serial.print(qx);
    Serial.print(" ");
    Serial.print(qy);
    Serial.print(" ");
    Serial.println(qz);

     // Send sensor data to Bluetooth
    char prefix[] = "V";
    ble.write(prefix, strlen(prefix));
    ble.write((uint8_t *)&qw, sizeof(float));
    ble.write((uint8_t *)&qx, sizeof(float));
    ble.write((uint8_t *)&qy, sizeof(float));
    ble.write((uint8_t *)&qz, sizeof(float));

    delay(10);
  }
}
