// rf22_router_test.pde
// -*- mode: C++ -*-
//
// Test code used during library development, showing how
// to do various things, and how to call various functions

#include <SPI.h>
#include <RHRouter.h>
#include <RH_RF22.h>

#define CLIENT_ADDRESS 1
#define ROUTER_ADDRESS 2
#define SERVER_ADDRESS 3

// Singleton instance of the radio driver
RH_RF22 driver;

// Class to manage message delivery and receipt, using the driver declared above
RHRouter manager(driver, CLIENT_ADDRESS);

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("RF22 init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
}

void test_routes()
{
  manager.clearRoutingTable();
//  manager.printRoutingTable();
  manager.addRouteTo(1, 101);
  manager.addRouteTo(2, 102);
  manager.addRouteTo(3, 103);
  RHRouter::RoutingTableEntry* e;
  e = manager.getRouteTo(0);
  if (e) // Should fail
    Serial.println("getRouteTo 0 failed");
    
  e = manager.getRouteTo(1);
  if (!e)
    Serial.println("getRouteTo 1 failed");
  if (e->dest != 1)
    Serial.println("getRouteTo 2 failed");
  if (e->next_hop != 101)
    Serial.println("getRouteTo 3 failed");
  if (e->state != RHRouter::Valid)
    Serial.println("getRouteTo 4 failed");
    
  e = manager.getRouteTo(2);
  if (!e)
    Serial.println("getRouteTo 5 failed");
  if (e->dest != 2)
    Serial.println("getRouteTo 6 failed");
  if (e->next_hop != 102)
    Serial.println("getRouteTo 7 failed");
  if (e->state != RHRouter::Valid)
    Serial.println("getRouteTo 8 failed");
    
  if (!manager.deleteRouteTo(1))
      Serial.println("deleteRouteTo 1 failed");
  // Route to 1 should now be gone
  e = manager.getRouteTo(1);
  if (e)
    Serial.println("deleteRouteTo 2 failed");
    
  Serial.println("-------------------");

//  manager.printRoutingTable();
  delay(500);

}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
//uint8_t buf[RH_RF22_MAX_MESSAGE_LEN];

void test_tx()
{
  manager.addRouteTo(SERVER_ADDRESS, ROUTER_ADDRESS);
  uint8_t errorcode;
  errorcode = manager.sendtoWait(data, sizeof(data), 100); // Should fail with no route
  if (errorcode != RH_ROUTER_ERROR_NO_ROUTE)
    Serial.println("sendtoWait 1 failed");
  errorcode = manager.sendtoWait(data, 255, 10); // Should fail too big
  if (errorcode != RH_ROUTER_ERROR_INVALID_LENGTH)
    Serial.println("sendtoWait 2 failed");   
  errorcode = manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS); // Should fail after timeouts to 110
  if (errorcode != RH_ROUTER_ERROR_UNABLE_TO_DELIVER)
    Serial.println("sendtoWait 3 failed");
  Serial.println("-------------------");
  delay(500);
}

void loop()
{
//  test_routes();
  test_tx();
}


