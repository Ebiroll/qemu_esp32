/*
MIT License

Copyright (c) 2017 Olof Astrand (Ebiroll)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <sys/time.h>

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
//#include "SSD1306.h" // alias for `
#include "SSD1306Wire.h"
#include <SPI.h>
#include<Arduino.h>

SSD1306Wire display(0x3c, 4, 15);

static const char *TAG = "radio";


// Pin definetion of WIFI LoRa 32
// HelTec AutoMation 2017 support@heltec.cn 
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI0     26   // GPIO26 -- SX127x's IRQ(Interrupt Request)


#define BAND 868100000.00
#define spreadingFactor 7
#define SignalBandwidth 128.0E3
#define preambleLength 8

char gpsLine[512];

// This task only prints what is received on UART1
static void uartGPSTask(void *inpar) {
//        rxPin = 22;
  char* data;

  uart_port_t uart_num = UART_NUM_1;                                     //uart port number
  uart_config_t uart_config = {
      .baud_rate = 9600,                     //baudrate
      .data_bits = UART_DATA_8_BITS,          //data bit mode
      .parity = UART_PARITY_DISABLE,          //parity mode
      .stop_bits = UART_STOP_BITS_1,          //stop bit mode
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  //hardware flow control(cts/rts)
      .rx_flow_ctrl_thresh = 122,             //flow control threshold
  };
  ESP_LOGI(TAG, "Setting UART configuration number %d...", uart_num);
  ESP_ERROR_CHECK( uart_param_config(uart_num, &uart_config));
  QueueHandle_t uart_queue;
  ESP_ERROR_CHECK( uart_set_pin(uart_num, 22, -1, -1, -1));
  ESP_ERROR_CHECK( uart_driver_install(uart_num, 512 * 2, 512 * 2, 10,  &uart_queue,0));

  printf("ESP32 uart echo\n");

  while(1) {
     //data=readLine(uart_num,gpsLine,256);
     //vTaskDelay(100 / portTICK_PERIOD_MS);
     int size = uart_read_bytes(uart_num, (unsigned char *)gpsLine, 1, portMAX_DELAY);
     printf("%c",gpsLine[0]);
  }
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

// Similar to uint32_t system_get_time(void)
uint32_t get_usec() {

 struct timeval tv;
 gettimeofday(&tv,NULL);
 return (tv.tv_sec*1000000 + tv.tv_usec);
 //uint64_t tmp=get_time_since_boot();
 //uint32_t ret=(uint32_t)tmp;
 //return ret;
}


#include <SPI.h>
#include <RH_RF95.h>
#include "modem_config.h"

// Singleton instance of the radio driver
RH_RF95 rf95(SS);

RH_RF95::ModemConfig myconfig =  {  RH_RF95_BW_125KHZ | RH_RF95_CODING_RATE_4_8, RH_RF95_SPREADING_FACTOR_128CPS, 0x04};

//#define Serial SerialUSB
void setup() 
{
  pinMode(25,OUTPUT); //Send success, LED will bright 1 second

  pinMode(16,OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH);

  Serial.begin(115200);
  while (!Serial) ; // Wait for serial port to be available
   // uint8_t slaveSelectPin, uint8_t interruptPin, RHGenericSPI& spi
  SPI.begin(SCK,MISO,MOSI,SS);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.drawString(30,30,"setup!");

  Serial.println("init");
  if (!rf95.init()) {
     Serial.println("init failed");
     display.drawString(10,10,"rf95.init() failed!");
  }
  //rf95.printRegisters();

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
//  driver.setTxPower(23, false);


//  driver.setTxPower(14, true);
//#define BAND 868100000.00
//#define spreadingFactor 7
//#define SignalBandwidth 128.0E3

  rf95.setFrequency(BAND);
  //rf95.setSf(spreadingFactor);
  //rf95.setBw(SignalBandwidth);
  //driver.setModemConfig(myconfig;
  rf95.setModemRegisters(&myconfig);
  rf95.setPreambleLength(preambleLength);


  //rf95.printRegisters();

}
void loop()
{
  //Serial.println("Sending to rf95_server");
  // Send a message to rf95_server

  uint8_t data[]="Hello..";


  //for (int j=0;j<10000;j++) {
    //Serial.println("Hello");

    //sprintf((char *)data,"Hello.. %d",j);
  
    display.drawString(2,10,String((const char *)data));

    rf95.send(data,sizeof(data));
    delay(500);
    digitalWrite(25, HIGH);
    delay(500);

    //rf95.waitPacketSent();
    digitalWrite(25, LOW);

  //}
  #if 0
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  if (rf95.waitAvailableTimeout(3000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("got reply: ");
      Serial.println((char*)buf);
//      Serial.print("RSSI: ");
//      Serial.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("recv failed");
    }
  }
  else
  {
    Serial.println("No reply, is rf95_server running?");
  }
  #endif
  //delay(400);
}



void loopTask(void *pvParameters)
{
    setup();
    for(;;) {
        micros(); //update overflow
        loop();
    }
}

extern "C" void app_main(void)
{
    initArduino();
    //nvs_flash_init();
    //xTaskCreatePinnedToCore(&uartGPSTask, "gps", 2*4096, NULL, 20, NULL, 1);
    //pinMode(4, OUTPUT);
    //digitalWrite(4, HIGH);
    xTaskCreatePinnedToCore(loopTask, "loopTask", 2*8192, NULL, 1, NULL, 0);

#if 0
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
            .ssid = "access_point_name",
            .password = "password",
            .bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
#endif


}
