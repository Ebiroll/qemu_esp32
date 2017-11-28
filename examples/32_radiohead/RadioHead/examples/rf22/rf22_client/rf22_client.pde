// rf22_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_RF22 class. RH_RF22 class does not provide for addressing or
// reliability, so you should only use RH_RF22 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf22_server
// Tested on Duemilanove, Uno with Sparkfun RFM22 wireless shield
// Tested on Flymaple with sparkfun RFM22 wireless shield
// Tested on ChiKit Uno32 with sparkfun RFM22 wireless shield

#include <SPI.h>
#include <RH_RF22.h>

// Singleton instance of the radio driver
RH_RF22 rf22;

void setup() 
{
  Serial.begin(9600);
  if (!rf22.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
}

void loop()
{
  Serial.println("Sending to rf22_server");
  // Send a message to rf22_server
  uint8_t data[] = "Hello World!";
  rf22.send(data, sizeof(data));
  
  rf22.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF22_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf22.waitAvailableTimeout(500))
  { 
    // Should be a reply message for us now   
    if (rf22.recv(buf, &len))
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
    Serial.println("No reply, is rf22_server running?");
  }
  delay(400);
}

