// simulator_reliable_datagram_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RHReliableDatagram class, using the RH_SIMULATOR driver to control a SIMULATOR radio.
// It is designed to work with the other example simulator_reliable_datagram_server
// Tested on Linux
// Build with
// cd whatever/RadioHead 
// tools/simBuild examples/simulator/simulator_reliable_datagram_client/simulator_reliable_datagram_client.pde
// Run with ./simulator_reliable_datagram_client
// Make sure you also have the 'Luminiferous Ether' simulator tools/etherSimulator.pl running

#include <RHReliableDatagram.h>
#include <RH_TCP.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

// Singleton instance of the radio driver
RH_TCP driver;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36

  // Maybe set this address from teh command line
  if (_simulator_argc >= 2)
     manager.setThisAddress(atoi(_simulator_argv[1]));
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_TCP_MAX_MESSAGE_LEN];

void loop()
{
  Serial.println("Sending to simulator_reliable_datagram_server");
    
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
      Serial.println("No reply, is simulator_reliable_datagram_server running?");
    }
  }
  else
    Serial.println("sendtoWait failed");
  delay(500);
}

