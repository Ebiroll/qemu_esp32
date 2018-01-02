// mrf89_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_MRF89 class. RH_MRF89 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example mrf89_server
// Tested with Teensy and MRF89XAM9A

#include <SPI.h>
#include <RH_MRF89.h>

// Singleton instance of the radio driver
RH_MRF89 mrf89;

void setup() 
{
  Serial.begin(9600);
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB

  if (!mrf89.init())
    Serial.println("init failed");
  // Default after init is 1dBm, 915.4MHz, FSK_Rb20Fd40  
  // But you can change that if you want:
//  mrf89.setTxPower(RH_MRF89_TXOPVAL_M8DBM); // Min power -8dBm
//  mrf89.setTxPower(RH_MRF89_TXOPVAL_13DBM); // Max power 13dBm
//  if (!mrf89.setFrequency(920.0))
//    Serial.println("setFrequency failed");
//  if (!mrf89.setModemConfig(RH_MRF89::FSK_Rb200Fd200)) // Fastest
//    Serial.println("setModemConfig failed");
}

void loop()
{
  Serial.println("Sending to mrf89_server");
  // Send a message to mrf89_server
  uint8_t data[] = "Hello World!";
  mrf89.send(data, sizeof(data));
  
  mrf89.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_MRF89_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (mrf89.waitAvailableTimeout(3000))
  { 
    // Should be a reply message for us now   
    if (mrf89.recv(buf, &len))
   {
      Serial.print("got reply: ");
      Serial.println((char*)buf);
//      Serial.print("RSSI: ");
//      Serial.println(mrf89.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("recv failed");
    }
  }
  else
  {
    Serial.println("No reply, is mrf89_server running?");
  }
  delay(400);
}


