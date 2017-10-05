// This example just provide basic LoRa function test;
// Not the LoRa's farthest distance or strongest interference immunity.
// For more informations, please vist www.heltec.cn or mail to support@heltec.cn

#if 0

#include <SPI.h>
#include <LoRa.h>
#include<Arduino.h>

// WIFI_LoRa_32 ports

// Original
// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)

// OLAS SPARKFUN
// GPIO18  --SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO23 -- SX1278's MOSI
// GPIO17  -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request G0 / DI0??)




#define SS      18
#define RST     14
#define DI0     26
#define BAND    868E6  // 433E6  //915E6 -- 

int counter = 0;

void setup() {
  pinMode(5,OUTPUT); //Send success, LED will bright 1 second

  digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(5, LOW);    // turn the LED off by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(5, LOW);    // turn the LED off by making the voltage LOW
  

  Serial.begin(115200);
  //while (!Serial); //If just the the basic function, must connect to a computer



  //SPI.begin(5,19,27,18);
  SPI.begin(18,19,23,17);
  LoRa.setPins(SS,RST,DI0);
//  Serial.println("LoRa Sender");

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1) {
      digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(100);                       // wait for 100th of a second
      digitalWrite(5, LOW);    // turn the LED off by making the voltage LOW    
      delay(100);                       // wait for a 100th of second      
    }
  }

  LoRa.setFrequency(868.1);
  LoRa.setSpreadingFactor(7);
  
  digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);                       // wait for 100th of a second
  digitalWrite(5, LOW);    // turn the LED off by making the voltage LOW    

  Serial.println("LoRa Initial OK!");
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);


  uint8_t version = LoRa.readRegister(0x42);
  Serial.print("Version: ");
  Serial.println(version);

  // send packet
  LoRa.beginPacket();
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.endPacket();

  counter++;
  digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(5, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
  
  delay(3000);
}



#endif
