// e32_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_E32 class. RH_E32 class does not provide for addressing or
// reliability, so you should only use RH_E32 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example e32_client
// Tested on Uno with E32-TTL-1W

#include <RH_E32.h>
#include "SoftwareSerial.h"

SoftwareSerial mySerial(7, 6);
RH_E32  driver(&mySerial, 4, 5, 8);

void setup() 
{
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available

  // Init the serial connection to the E32 module
  // which is assumned to be running at 9600baud.
  // If your E32 has been configured to another baud rate, change this:
  mySerial.begin(9600); 
  while (!mySerial) ;

  if (!driver.init())
    Serial.println("init failed");   
  // Defaults after initialising are:
  // 433MHz, 21dBm, 5kbps
  // You can change these as below
//  if (!driver.setDataRate(RH_E32::DataRate1kbps))
//    Serial.println("setDataRate failed"); 
//  if (!driver.setPower(RH_E32::Power30dBm))
//    Serial.println("setPower failed"); 
//  if (!driver.setFrequency(434))
//    Serial.println("setFrequency failed"); 
}

void loop() 
{
  if (driver.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_E32_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (driver.recv(buf, &len))
    {
//      RH_E32::printBuffer("request: ", buf, len);
      Serial.print("got request: ");
      Serial.println((char*)buf);

      // Send a reply
      uint8_t data[] = "And hello back to you";
      driver.send(data, sizeof(data));
      driver.waitPacketSent();
      Serial.println("Sent a reply");
    }
    else
    {
      Serial.println("recv failed");
    }
  }
}
