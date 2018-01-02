// rf24_lowpower_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple message transmitter
// which sleeps between transmissions (every 8 secs) to reduce power consumnption
// It uses the watchdog timer and the CPU sleep mode and the radio sleep mode
// to reduce quiescent power to 1.7mA
// Tested on Anarduino Mini http://www.anarduino.com/mini/ with RFM24W and RFM26W

#include <SPI.h>
#include <RH_RF24.h>
#include <avr/sleep.h>
#include <avr/power.h>

// Singleton instance of the radio driver
RH_RF24 rf24;

// Watchdog timer interrupt handler
ISR(WDT_vect)
{
  // Dont need to do anything, just override the default vector which causes a reset
}

// Go into sleep mode until WDT interrupt
void sleep()
{
  // Select the sleep mode we want. This is the lowest power
  // that can wake with WDT interrupt
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode(); // Sleep here and wake on WDT interrupt every 8 secs
}

void setup() 
{
  Serial.begin(9600);
  if (!rf24.init())
    Serial.println("init failed");
  // The default radio config is for 30MHz Xtal, 434MHz base freq 2GFSK 5kbps 10kHz deviation
  // power setting 0x10
  // If you want a different frequency mand or modulation scheme, you must generate a new
  // radio config file as per the RH_RF24 module documentation and recompile
  // You can change a few other things programatically:
  //rf24.setFrequency(435.0); // Only within the same frequency band
  //rf24.setTxPower(0x7f);

  // Set the watchdog timer to interrupt every 8 secs
  noInterrupts();
  // Set the watchdog reset bit in the MCU status register to 0.
  MCUSR &= ~(1<<WDRF);
  // Set WDCE and WDE bits in the watchdog control register.
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // Set watchdog clock prescaler bits to a value of 8 seconds.
  WDTCSR = (1<<WDP0) | (1<<WDP3);
  // Enable watchdog as interrupt only (no reset).
  WDTCSR |= (1<<WDIE);
  // Enable interrupts again.
  interrupts();
}


void loop()
{
  Serial.println("Sending to rf24_server");
  
  // Send a message to rf24_server
  uint8_t data[] = "Hello World!";
  rf24.send(data, sizeof(data));
  // Make sure its gone before we sleep
  rf24.waitPacketSent();

  // Anarduino Mini + RFM26, no UART connection (power only)
  // 9mA quiescent without any sleep (more during Tx)
  // 1.7mA quiescent with radio and CPU sleeping

  // radio is 1.58mA while sleeping (in STANDBY state but the antenna switch seems to take some power too)
  // 2mA when in Ready state
  rf24.sleep();
  
  // Sleep inside here until next WDT in 8 secs
  sleep();
}
