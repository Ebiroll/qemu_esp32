#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#endif

#define BLUEFRUIT51_ST_LSM303DLHC_L3GD20        (0)
#define BLUEFRUIT51_ST_LSM9DS1                  (1)
#define BLUEFRUIT51_NXP_FXOS8700_FXAS21002      (2)

// Define your target sensor(s) here based on the list above!
// #define AHRS_VARIANT    BLUEFRUIT51_ST_LSM303DLHC_L3GD20
#define AHRS_VARIANT   BLUEFRUIT51_NXP_FXOS8700_FXAS21002

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

// Include
#include "ArdPrintf.h"

// Config
#define FACTORYRESET_ENABLE      1

// Include generic sensors headers
#include <Wire.h>
#include <Adafruit_Sensor.h>

// Include appropriate sensor driver(s)
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

// Bluetooth
// ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

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

void setup()
{
  Serial.begin(115200);

  // Wait for the Serial Monitor to open (comment out to run without Serial Monitor)
  // while(!Serial);

  Serial.println(F("Adafruit nDOF AHRS Calibration Example")); Serial.println("");

  // Initialize the sensors.
  if (!gyro.begin())
  {
    /* There was a problem detecting the gyro ... check your connections */
    Serial.println("Ooops, no gyroscope detected ... Check your wiring!");
    while (1);
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

  // Initialise the module
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  // Factory Reset
  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
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
  while ( ble.isConnected() ) {
    sensors_event_t event; // Need to read raw data, which is stored at the same time

    // Get new data samples
    gyro.getEvent(&event);
#if AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
    accelmag.getEvent(&event);
#else
    accel.getEvent(&event);
    mag.getEvent(&event);
#endif

    // Print the sensor data
    Serial.print("Raw:");
#if AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
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
#if AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
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

    // Send sensor data to Bluetooth
    char prefix[] = "R";
    ble.write(prefix, strlen(prefix));
    //ble.write((uint8_t *)&event, sizeof(sensors_event_t));
#if AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
    ble.write((uint8_t *)&accelmag.accel_raw.x, sizeof(int16_t));
    ble.write((uint8_t *)&accelmag.accel_raw.y, sizeof(int16_t));
    ble.write((uint8_t *)&accelmag.accel_raw.z, sizeof(int16_t));
#else
    ble.write((uint8_t *)&accel.raw.x, sizeof(int16_t));
    ble.write((uint8_t *)&accel.raw.y, sizeof(int16_t));
    ble.write((uint8_t *)&accel.raw.z, sizeof(int16_t));
#endif
    ble.write((uint8_t *)&gyro.raw.x, sizeof(int16_t));
    ble.write((uint8_t *)&gyro.raw.y, sizeof(int16_t));
    ble.write((uint8_t *)&gyro.raw.z, sizeof(int16_t));
#if AHRS_VARIANT == BLUEFRUIT51_NXP_FXOS8700_FXAS21002
    ble.write((uint8_t *)&accelmag.mag_raw.x, sizeof(int16_t));
    ble.write((uint8_t *)&accelmag.mag_raw.y, sizeof(int16_t));
    ble.write((uint8_t *)&accelmag.mag_raw.z, sizeof(int16_t));
#else
    ble.write((uint8_t *)&mag.raw.x, sizeof(int16_t));
    ble.write((uint8_t *)&mag.raw.y, sizeof(int16_t));
    ble.write((uint8_t *)&mag.raw.z, sizeof(int16_t));
#endif

    delay(100);
  }
}

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
