#ifndef __ADAFRUIT_SENSOR_SET_H__
#define __ADAFRUIT_SENSOR_SET_H__

#include <Adafruit_Sensor.h>

// Interface for a device that has multiple sensors that can be queried by type.
class Adafruit_Sensor_Set
{
public:
  virtual ~Adafruit_Sensor_Set() {}
  virtual Adafruit_Sensor* getSensor(sensors_type_t type) { return NULL; }
};

#endif
