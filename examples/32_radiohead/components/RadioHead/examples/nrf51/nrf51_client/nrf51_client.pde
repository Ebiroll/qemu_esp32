// nrf51_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_NRF51 class. RH_NRF51 class does not provide for addressing or
// reliability, so you should only use RH_NRF51 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example nrf51_server.
// Tested on RedBearLabs nRF51822 and BLE Nano kit, built with Arduino 1.6.4.
// See http://redbearlab.com/getting-started-nrf51822/
// for how to set up your Arduino build environment
// Also tested with Sparkfun nRF52832 breakout board, witth Arduino 1.6.13 and
// Sparkfun nRF52 boards manager 0.2.3
#include <RH_NRF51.h>

// Singleton instance of the radio driver
RH_NRF51 nrf51;

void setup() 
{
  delay(1000); // Wait for serial port etc to be ready
  Serial.begin(9600);
  while (!Serial) 
    ; // wait for serial port to connect. 
  if (!nrf51.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf51.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf51.setRF(RH_NRF51::DataRate2Mbps, RH_NRF51::TransmitPower0dBm))
    Serial.println("setRF failed"); 
  
  // AES encryption can be enabled by setting the same key in the sender and receiver
//  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
//                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
//  nrf51.setEncryptionKey(key);

//  nrf51.printRegisters();
}


void loop()
{
  Serial.println("Sending to nrf51_server");
  // Send a message to nrf51_server
  uint8_t data[] = "Hello World!";
  nrf51.send(data, sizeof(data));
  nrf51.waitPacketSent();

  // Now wait for a reply
  uint8_t buf[RH_NRF51_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (nrf51.waitAvailableTimeout(500))
  { 
    // Should be a reply message for us now   
    if (nrf51.recv(buf, &len))
    {
      Serial.print("got reply: ");
      Serial.println((char*)buf);
    }
    else
    {
      Serial.println("recv failed");
    }
  }
  else
  {
    Serial.println("No reply, is nrf51_server running?");
  }

  delay(400);
}

