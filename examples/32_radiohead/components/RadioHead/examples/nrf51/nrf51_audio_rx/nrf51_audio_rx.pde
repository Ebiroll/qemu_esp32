// nrf51_audio_rx.pde
// Sample sketch for nRF51822 and RadioHead
//
// Plays audio samples received in the radio receiver 
// through a MCP4725 DAC such as on a SparkFun I2C DAC Breakout - MCP4725 (BOB-12918)
// works with matching transmitter (see nrf51_audio_tx.pde)
// Works with RedBear nRF51822 board.
// See examples/nrf51_audiotx/nrf51_audio.pdf for connection details

#include <nrf51.h>
#include <nrf51_bitfields.h>
#include <esb/nrf_esb.h>
#include <RH_NRF51.h>
#include <Wire.h>

// Number of samples per second to play at.
// Should match SAMPLE_RATE in nrf51_audio_tx
// The limiting factor is the time it takes to output a new sample through the DAC
#define SAMPLE_RATE 5000

// Number of 8 bit samples per packet
// Should equal or exceed the PACKET_SIZE in nrf51_audio_tx
#define MAX_PACKET_SIZE 255

// Singleton instance of the radio driver
RH_NRF51 driver;

void setup() 
{
  delay(1000);
  Serial.begin(9600);
  while (!Serial) 
    ; // wait for serial port to connect. 

  if (!driver.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm

  // Set up TIMER
  // Use TIMER0
  // Timer freq before prescaling is 16MHz (VARIANT_MCK)
  // We set up a 32 bit timer that restarts every 100us and outputs a new sample
  NRF_TIMER0->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos; 
  NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER0->CC[0] = VARIANT_MCK / SAMPLE_RATE; // Counts per cycle
  // When timer count expires, its cleared and restarts
  NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
  NRF_TIMER0->TASKS_START = 1;
  // Enable an interrupt when timer completes
  NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
  
  // Enable the TIMER0 interrupt, and set the priority 
  // TIMER0_IRQHandler() will be called after each sample is available
  NVIC_SetPriority(TIMER0_IRQn, 1);
  NVIC_EnableIRQ(TIMER0_IRQn); 

  // Initialise comms with the I2C DAC as fast as we can
  // Shame the 51822 does not suport the high speed I2C mode that the DAC does
  Wire.begin(TWI_SCL, TWI_SDA, TWI_FREQUENCY_400K);
}

volatile uint32_t count = 0;

uint8_t buffer_length = 0;
uint8_t buffer[MAX_PACKET_SIZE];
uint16_t buffer_play_index = 0;

// Write this sample to analog out
void analog_out(uint8_t val)
{
  // This takes about 120usecs, which
  // is the limiting factor for our sample rate of 5kHz
  // Writes to MCP4725 DAC over I2C using the Wire library
  Wire.beginTransmission(0x60); // 7 bit addressing
  Wire.write((val >> 4) & 0x0f); 
  Wire.write((val << 4) & 0xf0);  
  Wire.endTransmission();
}

// Called by timer interrupt
// Output the next available sample
void output_next_sample()
{
  if (buffer_play_index < buffer_length)
  {
    analog_out(buffer[buffer_play_index++]);
  }
}

void loop() 
{
  // Look for a new packet of samples
  if (driver.available())
  {
    // expect one of these every 40ms = 25Hz
    // This takes about 400us:
    buffer_length = sizeof(buffer);
    driver.recv(buffer, &buffer_length);
    buffer_play_index = 0; // Trigger the interrupt playing of this buffer from the start
  }
}

// This interrupt handler called when the timer interrupt fires
// Time to output the next sample
void TIMER0_IRQHandler(void)
{
  // It is vitally important that analog output completes before
  // the next interrupt becomes due!
  output_next_sample();
  NRF_TIMER0->EVENTS_COMPARE[0] = 0; // Clear the COMPARE[0] event and the interrupt
}

