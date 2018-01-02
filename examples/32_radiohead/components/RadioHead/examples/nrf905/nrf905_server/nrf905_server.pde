// nrf905_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing server
// with the RH_NRF905 class. RH_NRF905 class does not provide for addressing or
// reliability, so you should only use RH_NRF905  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example nrf905_client
// Tested on Teensy3.1 with nRF905 module
// Tested on Arduino Due with nRF905 module (Caution: use the SPI headers for connecting)

#include <SPI.h>
#include <RH_NRF905.h>

// Singleton instance of the radio driver
RH_NRF905 nrf905;

void setup() 
{
  Serial.begin(9600);
  while (!Serial) 
    ; // wait for serial port to connect. Needed for Leonardo only
  if (!nrf905.init())
    Serial.println("init failed");
  // Defaults after init are 433.2 MHz (channel 108), -10dBm
}

void loop()
{
  if (nrf905.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_NRF905_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (nrf905.recv(buf, &len))
    {
//      nrf905.printBuffer("request: ", buf, len);
      Serial.print("got request: ");
      Serial.println((char*)buf);
      
      // Send a reply
      uint8_t data[] = "And hello back to you";
      nrf905.send(data, sizeof(data));
      nrf905.waitPacketSent();
      Serial.println("Sent a reply");
    }
    else
    {
      Serial.println("recv failed");
    }
  }
}

