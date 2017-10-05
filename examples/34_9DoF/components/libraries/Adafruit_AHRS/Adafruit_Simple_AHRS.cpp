#include "Adafruit_Simple_AHRS.h"

// Create a simple AHRS from an explicit accelerometer and magnetometer sensor.
Adafruit_Simple_AHRS::Adafruit_Simple_AHRS(Adafruit_Sensor* accelerometer, Adafruit_Sensor* magnetometer):
  _accel(accelerometer),
  _mag(magnetometer)
{}

// Create a simple AHRS from a device with multiple sensors.
Adafruit_Simple_AHRS::Adafruit_Simple_AHRS(Adafruit_Sensor_Set& sensors):
  _accel(sensors.getSensor(SENSOR_TYPE_ACCELEROMETER)),
  _mag(sensors.getSensor(SENSOR_TYPE_MAGNETIC_FIELD))
{}

// Compute orientation based on accelerometer and magnetometer data.
bool Adafruit_Simple_AHRS::getOrientation(sensors_vec_t* orientation) {
  // Validate input and available sensors.
  if (orientation == NULL || _accel == NULL || _mag == NULL) return false;

  // Grab an acceleromter and magnetometer reading.
  sensors_event_t accel_event;
  _accel->getEvent(&accel_event);
  sensors_event_t mag_event;
  _mag->getEvent(&mag_event);

  float const PI_F = 3.14159265F;

  // roll: Rotation around the X-axis. -180 <= roll <= 180                                          
  // a positive roll angle is defined to be a clockwise rotation about the positive X-axis          
  //                                                                                                
  //                    y                                                                           
  //      roll = atan2(---)                                                                         
  //                    z                                                                           
  //                                                                                                
  // where:  y, z are returned value from accelerometer sensor                                      
  orientation->roll = (float)atan2(accel_event.acceleration.y, accel_event.acceleration.z);

  // pitch: Rotation around the Y-axis. -180 <= roll <= 180                                         
  // a positive pitch angle is defined to be a clockwise rotation about the positive Y-axis         
  //                                                                                                
  //                                 -x                                                             
  //      pitch = atan(-------------------------------)                                             
  //                    y * sin(roll) + z * cos(roll)                                               
  //                                                                                                
  // where:  x, y, z are returned value from accelerometer sensor                                   
  if (accel_event.acceleration.y * sin(orientation->roll) + accel_event.acceleration.z * cos(orientation->roll) == 0)
    orientation->pitch = accel_event.acceleration.x > 0 ? (PI_F / 2) : (-PI_F / 2);
  else
    orientation->pitch = (float)atan(-accel_event.acceleration.x / (accel_event.acceleration.y * sin(orientation->roll) + \
                                                                     accel_event.acceleration.z * cos(orientation->roll)));

  // heading: Rotation around the Z-axis. -180 <= roll <= 180                                       
  // a positive heading angle is defined to be a clockwise rotation about the positive Z-axis       
  //                                                                                                
  //                                       z * sin(roll) - y * cos(roll)                            
  //   heading = atan2(--------------------------------------------------------------------------)  
  //                    x * cos(pitch) + y * sin(pitch) * sin(roll) + z * sin(pitch) * cos(roll))   
  //                                                                                                
  // where:  x, y, z are returned value from magnetometer sensor                                    
  orientation->heading = (float)atan2(mag_event.magnetic.z * sin(orientation->roll) - mag_event.magnetic.y * cos(orientation->roll), \
                                      mag_event.magnetic.x * cos(orientation->pitch) + \
                                      mag_event.magnetic.y * sin(orientation->pitch) * sin(orientation->roll) + \
                                      mag_event.magnetic.z * sin(orientation->pitch) * cos(orientation->roll));


  // Convert angular data to degree 
  orientation->roll = orientation->roll * 180 / PI_F;
  orientation->pitch = orientation->pitch * 180 / PI_F;
  orientation->heading = orientation->heading * 180 / PI_F;

  return true;
}