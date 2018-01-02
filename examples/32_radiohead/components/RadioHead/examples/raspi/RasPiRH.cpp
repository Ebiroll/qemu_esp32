// RasPiRH.cpp
//
// Example program showing how to use RH_NRF24 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the NRF24L01
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi
// make
// sudo ./RasPiRH
//
// Creates a RHReliableDatagram manager and listens and prints for reliable datagrams
// sent to it on the default Channel 2.
//
// Contributed by Mike Poublon

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <RHReliableDatagram.h>
#include <RH_NRF24.h>

//Function Definitions
void sig_handler(int sig);
void printbuffer(uint8_t buff[], int len);

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

// Create an instance of a driver
// Chip enable is pin 22
// Slave Select is pin 24
RH_NRF24 nrf24(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24);
RHReliableDatagram manager(nrf24, SERVER_ADDRESS);

//Flag for Ctrl-C
volatile sig_atomic_t flag = 0;

//Main Function
int main (int argc, const char* argv[] )
{
  signal(SIGINT, sig_handler);

  if (!bcm2835_init())
  {
    printf( "\n\nRasPiRH Tester Startup Failed\n\n" );
    return 1;
  }

  printf( "\nRasPiRH Tester Startup\n\n" );

  /* Begin Driver Only Init Code
  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");
  End Driver Only Init Code */

  /* Begin Reliable Datagram Init Code */
  if (!manager.init())
  {
    printf( "Init failed\n" );
  }
  /* End Reliable Datagram Init Code */

  uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

  //Begin the main body of code
  while (true)
  {
    uint8_t len = sizeof(buf);
    uint8_t from, to, id, flags;

    /* Begin Driver Only code
    if (nrf24.available())
    {
      // Should be a message for us now
      //uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      if (nrf24.recv(buf, &len))
      {
        Serial.print("got request: ");
        Serial.println((char*)buf);
        Serial.println("");
      }
      else
      {
        Serial.println("recv failed");
      }
    }
    End Driver Only Code*/

    /* Begin Reliable Datagram Code */
    if (manager.available())
    {
      // Wait for a message addressed to us from the client
      uint8_t len = sizeof(buf);
      uint8_t from;
      if (manager.recvfromAck(buf, &len, &from))
      {
        Serial.print("got request from : 0x");
        Serial.print(from, HEX);
        Serial.print(": ");
        Serial.println((char*)buf);
      }
    }
    /* End Reliable Datagram Code */

    if (flag)
    {
      printf("\n---CTRL-C Caught - Exiting---\n");
      break;
    }
    //sleep(1);
    delay(25);
  }
  printf( "\nRasPiRH Tester Ending\n" );
  bcm2835_close();
  return 0;
}

void sig_handler(int sig)
{
  flag=1;
}

void printbuffer(uint8_t buff[], int len)
{
  for (int i = 0; i< len; i++)
  {
    printf(" %2X", buff[i]);
  }
}
