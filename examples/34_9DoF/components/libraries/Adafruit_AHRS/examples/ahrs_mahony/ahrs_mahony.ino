#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_LSM303_U.h>
#include <MahonyAHRS.h>
#include <MadgwickAHRS.h>

// Note: This sketch requires the MahonyAHRS sketch from
// https://github.com/PaulStoffregen/MahonyAHRS, or the
// MagdwickAHRS sketch from https://github.com/PaulStoffregen/MadgwickAHRS

// Note: This sketch is a WORK IN PROGRESS

// Create sensor instances.
Adafruit_L3GD20_Unified       gyro(20);
Adafruit_LSM303_Accel_Unified accel(30301);
Adafruit_LSM303_Mag_Unified   mag(30302);

// Mag calibration values are calculated via ahrs_calibration.
// These values must be determined for each baord/environment.
// See the image in this sketch folder for the values used
// below.

// Offsets applied to raw x/y/z values
float mag_offsets[3]            = { -2.20F, -5.53F, -26.34F };

// Soft iron error compensation matrix
float mag_softiron_matrix[3][3] = { { 0.934, 0.005, 0.013 },
                                    { 0.005, 0.948, 0.012 },
                                    { 0.013, 0.012, 1.129 } }; 

float mag_field_strength        = 48.41F;

// Mahony is lighter weight as a filter and should be used
// on slower systems
Mahony filter;
//Madgwick filter;

void setup()
{
  Serial.begin(115200);

  // Wait for the Serial Monitor to open (comment out to run without Serial Monitor)
  // while(!Serial);
  
  Serial.println(F("Adafruit 10 DOF Board AHRS Calibration Example")); Serial.println("");
  
  // Initialize the sensors.
  if(!gyro.begin())
  {
    /* There was a problem detecting the L3GD20 ... check your connections */
    Serial.println("Ooops, no L3GD20 detected ... Check your wiring!");
    while(1);
  }
  
  if(!accel.begin())
  {
    /* There was a problem detecting the LSM303DLHC ... check your connections */
    Serial.println("Ooops, no L3M303DLHC accel detected ... Check your wiring!");
    while(1);
  }
  
  if(!mag.begin())
  {
    /* There was a problem detecting the LSM303DLHC ... check your connections */
    Serial.println("Ooops, no L3M303DLHC mag detected ... Check your wiring!");
    while(1);
  }

  // Filter expects 50 samples per second
  filter.begin(50);
}

void loop(void)
{
  sensors_event_t gyro_event;
  sensors_event_t accel_event;
  sensors_event_t mag_event;

  // Get new data samples
  gyro.getEvent(&gyro_event);
  accel.getEvent(&accel_event);
  mag.getEvent(&mag_event);

  // Apply mag offset compensation (base values in uTesla)
  float x = mag_event.magnetic.x - mag_offsets[0];
  float y = mag_event.magnetic.y - mag_offsets[1];
  float z = mag_event.magnetic.z - mag_offsets[2];

  // Apply mag soft iron error compensation
  float mx = x * mag_softiron_matrix[0][0] + y * mag_softiron_matrix[0][1] + z * mag_softiron_matrix[0][2];
  float my = x * mag_softiron_matrix[1][0] + y * mag_softiron_matrix[1][1] + z * mag_softiron_matrix[1][2];
  float mz = x * mag_softiron_matrix[2][0] + y * mag_softiron_matrix[2][1] + z * mag_softiron_matrix[2][2];

  // The filter library expects gyro data in degrees/s, but adafruit sensor
  // uses rad/s so we need to convert them first (or adapt the filter lib
  // where they are being converted)
  float gx = gyro_event.gyro.x * 57.2958F;
  float gy = gyro_event.gyro.y * 57.2958F;
  float gz = gyro_event.gyro.z * 57.2958F;

  // Update the filter
  filter.update(gx, gy, gz,
                accel_event.acceleration.x, accel_event.acceleration.y, accel_event.acceleration.z,
                mx, my, mz);

  // Print the orientation filter output
  float roll = filter.getRoll();
  float pitch = filter.getPitch();
  float heading = filter.getYaw();
  Serial.print(millis());
  Serial.print(" - Orientation: ");
  Serial.print(heading);
  Serial.print(" ");
  Serial.print(pitch);
  Serial.print(" ");
  Serial.println(roll);
    
  delay(10);
}
