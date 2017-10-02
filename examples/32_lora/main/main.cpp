//file: main.cpp
#include "Arduino.h"

void setup(){
  Serial.begin(115200);
}

void loop(){
  Serial.println("loop");
  delay(1000);
}
