// nrf24_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple encrypted messageing client
// with the RH_NRF24 class. RH_NRF24 class does not provide for addressing or
// reliability, so you should only use RH_NRF24 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example nrf24_encrypted_server.
// Tested on Duemilanove with Sparkfun NRF25L01 module

#include <SPI.h>
#include <RH_NRF24.h>
#include <RHEncryptedDriver.h>
#include <Speck.h>

// Singleton instance of the radio driver
RH_NRF24 nrf24;
// RH_NRF24 nrf24(8, 7); // use this to be electrically compatible with Mirf
// RH_NRF24 nrf24(8, 10);// For Leonardo, need explicit SS pin
// RH_NRF24 nrf24(8, 7); // For RFM73 on Anarduino Mini

// You can choose any of several encryption ciphers
Speck myCipher;   // Instantiate a Speck block ciphering
// The RHEncryptedDriver acts as a wrapper for the actual radio driver
RHEncryptedDriver driver(nrf24, myCipher); 
// The key MUST be the same as the one in the server
unsigned char encryptkey[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; 

void setup() 
{
  Serial.begin(9600);
  while (!Serial) 
    ; // wait for serial port to connect. Needed for Leonardo only
  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");  

  // Now set up the encryption key in our cipher
  myCipher.setKey(encryptkey, sizeof(encryptkey));
          
}


void loop()
{
  Serial.println("Sending to nrf24_encrypted_server");
  // Send a message to nrf24_server
  uint8_t data[] = "Hello World!"; // Dont make this too long
  driver.send(data, sizeof(data));
  
  driver.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (driver.waitAvailableTimeout(500))
  { 
    // Should be a reply message for us now   
    if (driver.recv(buf, &len))
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
    Serial.println("No reply, is nrf24_encrypted_server running?");
  }
  delay(400);
}

