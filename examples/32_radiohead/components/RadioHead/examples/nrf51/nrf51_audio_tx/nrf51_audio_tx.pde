// nrf51_audio_tx.pde
// Sample sketch for nRF51822 and RadioHead
//
// Reads audio samples from an electret microphone
// via the built in ADC in the nRF51822
// Blocks of samples are sent by RadioHEad RH_NRF51 driver
// to a matching receiver (see nrf51_audio_rx.pde)
// Works with RedBear nRF51822 board.
// See examples/nrf51_audiotx/nrf51_audio.pdf for connection details

#include <nrf51.h>
#include <nrf51_bitfields.h>
#include <esb/nrf_esb.h>
#include <RH_NRF51.h>

// Number of audio samples per second
// Should match SAMPLE_RATE in nrf51_audio_rx
// Limited by the rate we can output samples in the receiver
#define SAMPLE_RATE 5000

// Number of 8 bit samples per packet
#define PACKET_SIZE 200

// Number of ADC data buffers
#define NUM_BUFFERS 2

// Minimum diff between smallest and largest reading in a given buffer
// before we will send that buffer. We dont transmit quiet signals or silence
#define USE_SQUELCH 0
#define SQUELCH_THRESHOLD 2

// These provide data transfer between the low level ADC interrupt handler and the 
// higher level packet assembly and transmission
volatile uint8_t  buffers[NUM_BUFFERS][PACKET_SIZE];
volatile uint16_t sample_index = 0;          // Of the next sample to write
volatile uint8_t  buffer_index = 0;          // Of the bufferbeing filled
volatile bool     buffer_ready[NUM_BUFFERS]; // Set when a buffer is full

// These hold the state of the high level transmitter code
uint8_t  next_tx_buffer = 0;
 
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

  // Set up ADC
  // Uses the builtin 1.2V bandgap reference and no prescaling
  // AnalogInput2 is A0 on RedBear nrf51822 board
  // Input voltage range is 0.0 to 1.2 V
  NRF_ADC->CONFIG = ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos 
                    | ADC_CONFIG_INPSEL_AnalogInputNoPrescaling << ADC_CONFIG_INPSEL_Pos 
                    | ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos 
                    | ADC_CONFIG_PSEL_AnalogInput2 << ADC_CONFIG_PSEL_Pos;
  NRF_ADC->ENABLE = 1;
  NRF_ADC->INTENSET = ADC_INTENSET_END_Msk; // Interrupt at completion of each sample
    
  // Set up TIMER to trigger ADC samples
  // Use TIMER0
  // Timer freq before prescaling is 16MHz (VARIANT_MCK)
  // We set up a 32 bit timer that restarts every 100us and trggers a new ADC sample
  NRF_TIMER0->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos; 
  NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER0->CC[0] = VARIANT_MCK / SAMPLE_RATE; // Counts per cycle
  // When timer count expires, its cleared and restarts
  NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
  NRF_TIMER0->TASKS_START = 1;

  // When the timer expires, trigger an ADC conversion
  NRF_PPI->CH[0].EEP = (uint32_t)(&NRF_TIMER0->EVENTS_COMPARE[0]);
  NRF_PPI->CH[0].TEP = (uint32_t)(&NRF_ADC->TASKS_START);
  NRF_PPI->CHENSET = PPI_CHEN_CH0_Msk;

  // Enable the ADC interrupt, and set the priority
  // ADC_IRQHandler() will be called after each sample is available
  NVIC_SetPriority(ADC_IRQn, 1);
  NVIC_EnableIRQ(ADC_IRQn);   
}

// Called when a new sample is available from the ADC.
// Add it to the current buffer.
// when the buffer is full, signal that and switch to the other buffer.
void handle_sample()
{
  buffers[buffer_index][sample_index++] = NRF_ADC->RESULT;
  if (sample_index >= PACKET_SIZE)
  {
    sample_index = 0;
    buffer_ready[buffer_index] = true;
    buffer_index = (buffer_index + 1) % NUM_BUFFERS;
    // If the next buffer is still still full, we have an overrun
    if (buffer_ready[buffer_index])
      Serial.println("Overrun");
  }
}

void loop() {
  // Wait for the adc to fill the current buffer
  if (buffer_ready[next_tx_buffer])
  {
#if USE_SQUELCH
    // Honour squelch settings
    uint8_t min_value = 255;
    uint8_t max_value = 0;
    uint16_t i;
    for (i = 0; i < PACKET_SIZE; i++)
    {
      if (buffers[next_tx_buffer][i] > max_value)
        max_value = buffers[next_tx_buffer][i];
      if (buffers[next_tx_buffer][i] < min_value)
        min_value = buffers[next_tx_buffer][i];
    }
    if (max_value - min_value > SQUELCH_THRESHOLD)
#endif
    {
      // OK to send this one
      driver.waitPacketSent(); // Make sure the previous packet has gone
      driver.send((uint8_t*)buffers[next_tx_buffer], PACKET_SIZE);
    }
    
    // Now get ready to wait for the next buffer
    buffer_ready[next_tx_buffer] = false;
    next_tx_buffer = (next_tx_buffer + 1) % NUM_BUFFERS;
  }
}

// Called as an interrupt after each new ADC sample is complete.
void ADC_IRQHandler(void)
{
  NRF_ADC->EVENTS_END = 0; // Clear the end event
  handle_sample();
}

