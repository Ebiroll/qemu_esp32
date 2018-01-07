// nrf51_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_NRF51 driver to control a NRF51 radio.
// It is designed to work with the other example nrf51_reliable_datagram_server
// Tested on RedBearLabs nRF51822 and BLE Nano kit, built with Arduino 1.6.4.
// See http://redbearlab.com/getting-started-nrf51822/
// for how to set up your Arduino build environment
// Also tested with Sparkfun nRF52832 breakout board, witth Arduino 1.6.13 and
// Sparkfun nRF52 boards manager 0.2.3

#include <RHReliableDatagram.h>
#include <RH_NRF51.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

// Singleton instance of the radio driver
RH_NRF51 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

void setup() 
{  
  delay(1000); // Wait for serial port etc to be ready
  Serial.begin(9600);
  while (!Serial) 
    ; // wait for serial port to connect. 

  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_NRF51_MAX_MESSAGE_LEN];

void loop()
{
  Serial.println("Sending to nrf51_reliable_datagram_server");
    
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
      Serial.println("No reply, is nrf51_reliable_datagram_server running?");
    }
  }
  else
    Serial.println("sendtoWait failed");
  delay(500);
}

