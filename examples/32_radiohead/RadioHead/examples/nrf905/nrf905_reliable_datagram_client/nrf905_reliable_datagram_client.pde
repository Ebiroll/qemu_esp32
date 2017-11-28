// nrf905_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_NRF905 driver to control a NRF905 radio.
// It is designed to work with the other example nrf905_reliable_datagram_server
// Tested on Teensy3.1 with nRF905 module
// Tested on Arduino Due with nRF905 module (Caution: use the SPI headers for connecting)

#include <RHReliableDatagram.h>
#include <RH_NRF905.h>
#include <SPI.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

// Singleton instance of the radio driver
RH_NRF905 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 433.2 MHz (channel 108), -10dBm
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_NRF905_MAX_MESSAGE_LEN];

void loop()
{
  Serial.println("Sending to nrf905_reliable_datagram_server");
    
  // Send a message to manager_server
  if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS))
  {
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;   
    if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
    {
      Serial.print("got reply from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);
    }
    else
    {
      Serial.println("No reply, is nrf905_reliable_datagram_server running?");
    }
  }
  else
    Serial.println("sendtoWait failed");
  delay(500);
}

