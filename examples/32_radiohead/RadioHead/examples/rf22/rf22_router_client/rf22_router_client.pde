// rf22_router_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, routed reliable messaging client
// with the RHRouter class.
// It is designed to work with the other examples rf22_router_server*

#include <RHRouter.h>
#include <RH_RF22.h>
#include <SPI.h>

// In this small artifical network of 4 nodes,
// messages are routed via intermediate nodes to their destination
// node. All nodes can act as routers
// CLIENT_ADDRESS <-> SERVER1_ADDRESS <-> SERVER2_ADDRESS<->SERVER3_ADDRESS
#define CLIENT_ADDRESS 1
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4

// Singleton instance of the radio driver
RH_RF22 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHRouter manager(driver, CLIENT_ADDRESS);

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  
  // Manually define the routes for this network
  manager.addRouteTo(SERVER1_ADDRESS, SERVER1_ADDRESS);  
  manager.addRouteTo(SERVER2_ADDRESS, SERVER2_ADDRESS);
  manager.addRouteTo(SERVER3_ADDRESS, SERVER3_ADDRESS);
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];

void loop()
{
  Serial.println("Sending to rf22_router_server3");
    
  // Send a message to a rf22_router_server
  // It will be routed by the intermediate
  // nodes to the destination node, accorinding to the
  // routing tables in each node
  if (manager.sendtoWait(data, sizeof(data), SERVER3_ADDRESS) == RH_ROUTER_ERROR_NONE)
  {
    // It has been reliably delivered to the next node.
    // Now wait for a reply from the ultimate server
    uint8_t len = sizeof(buf);
    uint8_t from;    
    if (manager.recvfromAckTimeout(buf, &len, 3000, &from))
    {
      Serial.print("got reply from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);
    }
    else
    {
      Serial.println("No reply, is rf22_router_server1, rf22_router_server2 and rf22_router_server3 running?");
    }
  }
  else
    Serial.println("sendtoWait failed. Are the intermediate router servers running?");
}

