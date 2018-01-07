//  LoRa simple server with encrypted communications 
//  Philippe.Rochat'at'gmail.com
//  06.07.2017

#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <Speck.h>

RH_RF95 rf95;     // Instanciate a LoRa driver
Speck myCipher;   // Instanciate a Speck block ciphering
RHEncryptedDriver myDriver(rf95, myCipher); // Instantiate the driver with those two

float frequency = 868.0; // Change the frequency here. 
unsigned char encryptkey[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}; // The very secret key 

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available
  Serial.println("LoRa Simple_Encrypted Server");
  if (!rf95.init())
    Serial.println("LoRa init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  myCipher.setKey(encryptkey, 16);
  delay(4000);
  Serial.println("Setup completed");
}

void loop() {
  if (myDriver.available()) {
    // Should be a message for us now   
    uint8_t buf[myDriver.maxMessageLength()];
    uint8_t len = sizeof(buf);
    if (myDriver.recv(buf, &len)) {
      Serial.print("Received: ");
      Serial.println((char *)&buf);
    }
    else
    {
        Serial.println("recv failed");
    }
  }
}
