/***************************************************
  This is a library for the FXOS8700 Accel/Mag

  Designed specifically to work with the Adafruit FXOS8700 Breakout
  ----> https://www.adafruit.com/products

  These sensors use I2C to communicate, 2 pins (I2C)
  are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Kevin "KTOWN" Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#ifndef __FXOS8700_H__
#define __FXOS8700_H__

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Adafruit_Sensor.h>
#include <Wire.h>

/*=========================================================================
    I2C ADDRESS/BITS AND SETTINGS
    -----------------------------------------------------------------------*/
    #define FXOS8700_ADDRESS           (0x1F)     // 0011111
    #define FXOS8700_ID                (0xC7)     // 1100 0111
/*=========================================================================*/

/*=========================================================================
    REGISTERS
    -----------------------------------------------------------------------*/
    typedef enum
    {                                             // DEFAULT    TYPE
      FXOS8700_REGISTER_STATUS          = 0x00,
      FXOS8700_REGISTER_OUT_X_MSB       = 0x01,
      FXOS8700_REGISTER_OUT_X_LSB       = 0x02,
      FXOS8700_REGISTER_OUT_Y_MSB       = 0x03,
      FXOS8700_REGISTER_OUT_Y_LSB       = 0x04,
      FXOS8700_REGISTER_OUT_Z_MSB       = 0x05,
      FXOS8700_REGISTER_OUT_Z_LSB       = 0x06,
      FXOS8700_REGISTER_WHO_AM_I        = 0x0D,   // 11000111   r
      FXOS8700_REGISTER_XYZ_DATA_CFG    = 0x0E,
      FXOS8700_REGISTER_CTRL_REG1       = 0x2A,   // 00000000   r/w
      FXOS8700_REGISTER_CTRL_REG2       = 0x2B,   // 00000000   r/w
      FXOS8700_REGISTER_CTRL_REG3       = 0x2C,   // 00000000   r/w
      FXOS8700_REGISTER_CTRL_REG4       = 0x2D,   // 00000000   r/w
      FXOS8700_REGISTER_CTRL_REG5       = 0x2E,   // 00000000   r/w
      FXOS8700_REGISTER_MSTATUS         = 0x32,
      FXOS8700_REGISTER_MOUT_X_MSB      = 0x33,
      FXOS8700_REGISTER_MOUT_X_LSB      = 0x34,
      FXOS8700_REGISTER_MOUT_Y_MSB      = 0x35,
      FXOS8700_REGISTER_MOUT_Y_LSB      = 0x36,
      FXOS8700_REGISTER_MOUT_Z_MSB      = 0x37,
      FXOS8700_REGISTER_MOUT_Z_LSB      = 0x38,
      FXOS8700_REGISTER_MCTRL_REG1      = 0x5B,   // 00000000   r/w
      FXOS8700_REGISTER_MCTRL_REG2      = 0x5C,   // 00000000   r/w
      FXOS8700_REGISTER_MCTRL_REG3      = 0x5D,   // 00000000   r/w
    } fxos8700Registers_t;
/*=========================================================================*/

/*=========================================================================
    OPTIONAL SPEED SETTINGS
    -----------------------------------------------------------------------*/
    typedef enum
    {
      ACCEL_RANGE_2G                    = 0x00,
      ACCEL_RANGE_4G                    = 0x01,
      ACCEL_RANGE_8G                    = 0x02
    } fxos8700AccelRange_t;
/*=========================================================================*/

/*=========================================================================
    RAW GYROSCOPE DATA TYPE
    -----------------------------------------------------------------------*/
    typedef struct fxos8700RawData_s
    {
      int16_t x;
      int16_t y;
      int16_t z;
    } fxos8700RawData_t;
/*=========================================================================*/

class Adafruit_FXOS8700 : public Adafruit_Sensor
{
  public:
    Adafruit_FXOS8700(int32_t accelSensorID = -1, int32_t magSensorID = -1);

    bool begin           ( fxos8700AccelRange_t rng = ACCEL_RANGE_2G );
    bool getEvent        ( sensors_event_t* accel );
    void getSensor       ( sensor_t* accel );
    bool getEvent        ( sensors_event_t* accel, sensors_event_t* mag );
    void getSensor       ( sensor_t* accel, sensor_t* mag );

    fxos8700RawData_t accel_raw; /* Raw values from last sensor read */
    fxos8700RawData_t mag_raw;   /* Raw values from last sensor read */

  private:
    void        write8  ( byte reg, byte value );
    byte        read8   ( byte reg );

    fxos8700AccelRange_t _range;
    int32_t              _accelSensorID;
    int32_t              _magSensorID;
};

#endif
