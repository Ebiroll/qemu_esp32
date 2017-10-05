/***************************************************
  This is a library for the FXOS8700 Accel/Mag

  Designed specifically to work with the Adafruit FXOS8700 Breakout
  ----> https://www.adafruit.com/products/

  These sensors use I2C to communicate, 2 pins (I2C)
  are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Kevin "KTOWN" Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>
#include <limits.h>

#include "Adafruit_FXOS8700.h"

#define ACCEL_MG_LSB_2G (0.000244F)
#define ACCEL_MG_LSB_4G (0.000488F)
#define ACCEL_MG_LSB_8G (0.000976F)
#define MAG_UT_LSB      (0.1F)

/***************************************************************************
 PRIVATE FUNCTIONS
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
void Adafruit_FXOS8700::write8(byte reg, byte value)
{
  Wire.beginTransmission(FXOS8700_ADDRESS);
  #if ARDUINO >= 100
    Wire.write((uint8_t)reg);
    Wire.write((uint8_t)value);
  #else
    Wire.send(reg);
    Wire.send(value);
  #endif
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
byte Adafruit_FXOS8700::read8(byte reg)
{
  byte value;

  Wire.beginTransmission((byte)FXOS8700_ADDRESS);
  #if ARDUINO >= 100
    Wire.write((uint8_t)reg);
  #else
    Wire.send(reg);
  #endif
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom((byte)FXOS8700_ADDRESS, (byte)1);
  #if ARDUINO >= 100
    value = Wire.read();
  #else
    value = Wire.receive();
  #endif

  return value;
}

/***************************************************************************
 CONSTRUCTOR
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_FXOS8700 class
*/
/**************************************************************************/
Adafruit_FXOS8700::Adafruit_FXOS8700(int32_t accelSensorID, int32_t magSensorID)
{
  _accelSensorID = accelSensorID;
  _magSensorID = magSensorID;
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Setups the HW
*/
/**************************************************************************/
bool Adafruit_FXOS8700::begin(fxos8700AccelRange_t rng)
{
  /* Enable I2C */
  Wire.begin();

  /* Set the range the an appropriate value */
  _range = rng;

  /* Clear the raw sensor data */
  accel_raw.x = 0;
  accel_raw.y = 0;
  accel_raw.z = 0;
  mag_raw.x = 0;
  mag_raw.y = 0;
  mag_raw.z = 0;

  /* Make sure we have the correct chip ID since this checks
     for correct address and that the IC is properly connected */
  uint8_t id = read8(FXOS8700_REGISTER_WHO_AM_I);
  if (id != FXOS8700_ID)
  {
    return false;
  }

  /* Set to standby mode (required to make changes to this register) */
  write8(FXOS8700_REGISTER_CTRL_REG1, 0);

  /* Configure the accelerometer */
  switch (_range) {
      case (ACCEL_RANGE_2G):
        write8(FXOS8700_REGISTER_XYZ_DATA_CFG, 0x00);
      break;
      case (ACCEL_RANGE_4G):
        write8(FXOS8700_REGISTER_XYZ_DATA_CFG, 0x01);
      break;
      case (ACCEL_RANGE_8G):
        write8(FXOS8700_REGISTER_XYZ_DATA_CFG, 0x02);
      break;
  }
  /* High resolution */
  write8(FXOS8700_REGISTER_CTRL_REG2, 0x02);
  /* Active, Normal Mode, Low Noise, 100Hz in Hybrid Mode */
  write8(FXOS8700_REGISTER_CTRL_REG1, 0x15);

  /* Configure the magnetometer */
  /* Hybrid Mode, Over Sampling Rate = 16 */
  write8(FXOS8700_REGISTER_MCTRL_REG1, 0x1F);
  /* Jump to reg 0x33 after reading 0x06 */
  write8(FXOS8700_REGISTER_MCTRL_REG2, 0x20);

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the most recent sensor event
*/
/**************************************************************************/
bool Adafruit_FXOS8700::getEvent(sensors_event_t* accelEvent, sensors_event_t* magEvent)
{
  /* Clear the event */
  memset(accelEvent, 0, sizeof(sensors_event_t));
  memset(magEvent, 0, sizeof(sensors_event_t));

  /* Clear the raw data placeholder */
  accel_raw.x = 0;
  accel_raw.y = 0;
  accel_raw.z = 0;
  mag_raw.x = 0;
  mag_raw.y = 0;
  mag_raw.z = 0;

  /* Set the static metadata */
  accelEvent->version   = sizeof(sensors_event_t);
  accelEvent->sensor_id = _accelSensorID;
  accelEvent->type      = SENSOR_TYPE_ACCELEROMETER;

  magEvent->version   = sizeof(sensors_event_t);
  magEvent->sensor_id = _magSensorID;
  magEvent->type      = SENSOR_TYPE_MAGNETIC_FIELD;

  /* Read 13 bytes from the sensor */
  Wire.beginTransmission((byte)FXOS8700_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(FXOS8700_REGISTER_STATUS | 0x80);
  #else
    Wire.send(FXOS8700_REGISTER_STATUS | 0x80);
  #endif
  Wire.endTransmission();
  Wire.requestFrom((byte)FXOS8700_ADDRESS, (byte)13);

  /* ToDo: Check status first! */
  #if ARDUINO >= 100
    uint8_t status = Wire.read();
    uint8_t axhi = Wire.read();
    uint8_t axlo = Wire.read();
    uint8_t ayhi = Wire.read();
    uint8_t aylo = Wire.read();
    uint8_t azhi = Wire.read();
    uint8_t azlo = Wire.read();
    uint8_t mxhi = Wire.read();
    uint8_t mxlo = Wire.read();
    uint8_t myhi = Wire.read();
    uint8_t mylo = Wire.read();
    uint8_t mzhi = Wire.read();
    uint8_t mzlo = Wire.read();
  #else
    uint8_t status = Wire.receive();
    uint8_t axhi = Wire.receive();
    uint8_t axlo = Wire.receive();
    uint8_t ayhi = Wire.receive();
    uint8_t aylo = Wire.receive();
    uint8_t azhi = Wire.receive();
    uint8_t azlo = Wire.receive();
    uint8_t mxhi = Wire.receive();
    uint8_t mxlo = Wire.receive();
    uint8_t myhi = Wire.receive();
    uint8_t mylo = Wire.receive();
    uint8_t mzhi = Wire.receive();
    uint8_t mzlo = Wire.receive();
  #endif

  /* Set the timestamps */
  accelEvent->timestamp = millis();
  magEvent->timestamp = accelEvent->timestamp;

  /* Shift values to create properly formed integers */
  /* Note, accel data is 14-bit and left-aligned, so we shift two bit right */
  accelEvent->acceleration.x = (int16_t)((axhi << 8) | axlo) >> 2;
  accelEvent->acceleration.y = (int16_t)((ayhi << 8) | aylo) >> 2;
  accelEvent->acceleration.z = (int16_t)((azhi << 8) | azlo) >> 2;
  magEvent->magnetic.x = (int16_t)((mxhi << 8) | mxlo);
  magEvent->magnetic.y = (int16_t)((myhi << 8) | mylo);
  magEvent->magnetic.z = (int16_t)((mzhi << 8) | mzlo);

  /* Assign raw values in case someone needs them */
  accel_raw.x = accelEvent->acceleration.x;
  accel_raw.y = accelEvent->acceleration.y;
  accel_raw.z = accelEvent->acceleration.z;
  mag_raw.x = magEvent->magnetic.x;
  mag_raw.y = magEvent->magnetic.y;
  mag_raw.z = magEvent->magnetic.z;

  /* Convert accel values to m/s^2 */
  switch (_range) {
      case (ACCEL_RANGE_2G):
          accelEvent->acceleration.x *= ACCEL_MG_LSB_2G * SENSORS_GRAVITY_STANDARD;
          accelEvent->acceleration.y *= ACCEL_MG_LSB_2G * SENSORS_GRAVITY_STANDARD;
          accelEvent->acceleration.z *= ACCEL_MG_LSB_2G * SENSORS_GRAVITY_STANDARD;
      break;
      case (ACCEL_RANGE_4G):
          accelEvent->acceleration.x *= ACCEL_MG_LSB_4G * SENSORS_GRAVITY_STANDARD;
          accelEvent->acceleration.y *= ACCEL_MG_LSB_4G * SENSORS_GRAVITY_STANDARD;
          accelEvent->acceleration.z *= ACCEL_MG_LSB_4G * SENSORS_GRAVITY_STANDARD;
      break;
      case (ACCEL_RANGE_8G):
          accelEvent->acceleration.x *= ACCEL_MG_LSB_8G * SENSORS_GRAVITY_STANDARD;
          accelEvent->acceleration.y *= ACCEL_MG_LSB_8G * SENSORS_GRAVITY_STANDARD;
          accelEvent->acceleration.z *= ACCEL_MG_LSB_8G * SENSORS_GRAVITY_STANDARD;
      break;
  }


  /* Convert mag values to uTesla */
  magEvent->magnetic.x *= MAG_UT_LSB;
  magEvent->magnetic.y *= MAG_UT_LSB;
  magEvent->magnetic.z *= MAG_UT_LSB;

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void  Adafruit_FXOS8700::getSensor(sensor_t* accelSensor, sensor_t* magSensor)
{
  /* Clear the sensor_t object */
  memset(accelSensor, 0, sizeof(sensor_t));
  memset(magSensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy (accelSensor->name, "FXOS8700", sizeof(accelSensor->name) - 1);
  accelSensor->name[sizeof(accelSensor->name) - 1] = 0;
  accelSensor->version     = 1;
  accelSensor->sensor_id   = _accelSensorID;
  accelSensor->type        = SENSOR_TYPE_ACCELEROMETER;
  accelSensor->min_delay   = 0.01F; // 100Hz
  switch (_range) {
      case (ACCEL_RANGE_2G):
          accelSensor->max_value   = 2.0F * SENSORS_GRAVITY_STANDARD;
          accelSensor->min_value   = -1.999F * SENSORS_GRAVITY_STANDARD;
          accelSensor->resolution  = ACCEL_MG_LSB_2G * SENSORS_GRAVITY_STANDARD;
      break;
      case (ACCEL_RANGE_4G):
          accelSensor->max_value   = 4.0F * SENSORS_GRAVITY_STANDARD;
          accelSensor->min_value   = -3.998F * SENSORS_GRAVITY_STANDARD;
          accelSensor->resolution  = ACCEL_MG_LSB_4G * SENSORS_GRAVITY_STANDARD;
      break;
      case (ACCEL_RANGE_8G):
          accelSensor->max_value   = 8.0F * SENSORS_GRAVITY_STANDARD;
          accelSensor->min_value   = -7.996F * SENSORS_GRAVITY_STANDARD;
          accelSensor->resolution  = ACCEL_MG_LSB_8G * SENSORS_GRAVITY_STANDARD;
      break;
  }

  strncpy (magSensor->name, "FXOS8700", sizeof(magSensor->name) - 1);
  magSensor->name[sizeof(magSensor->name) - 1] = 0;
  magSensor->version     = 1;
  magSensor->sensor_id   = _magSensorID;
  magSensor->type        = SENSOR_TYPE_MAGNETIC_FIELD;
  magSensor->min_delay   = 0.01F; // 100Hz
  magSensor->max_value   = 1200.0F;
  magSensor->min_value   = -1200.0F;
  magSensor->resolution  = 0.1F;
}

/* To keep Adafruit_Sensor happy we need a single sensor interface */
/* When only one sensor is requested, return accel data */
bool Adafruit_FXOS8700::getEvent(sensors_event_t* accelEvent)
{
    sensors_event_t mag;

    return getEvent(accelEvent, &mag);
}

/* To keep Adafruit_Sensor happy we need a single sensor interface */
/* When only one sensor is requested, return accel data */
void  Adafruit_FXOS8700::getSensor(sensor_t* accelSensor)
{
    sensor_t mag;

    return getSensor(accelSensor, &mag);
}
