//Created by Valerii Proskurin.
//Date: 30.01.2018
//parts of his code are taken from
//https://github.com/igrr/esp32-cam-demo
//by Ivan Grokhotkov

#include <WiFiClient.h>
#include <ESP32WebServer.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "rom/lldesc.h"
#include "Arduino.h"
#include "driver/ledc.h"

#define SAMPLES_COUNT 1000
/* wifi host id and password */
const char* ssid = "yourWifiName";
const char* password = "yourWifiPassword";
//#define DEBUG_LOG_ENABLE

const int XCLK = 32;
const int PCLK = 33;

const int D0 = 27;
const int D1 = 17;
const int D2 = 16;
const int D3 = 15;
const int D4 = 14;
const int D5 = 13;
const int D6 = 12;
const int D7 = 4;


#ifdef DEBUG_LOG_ENABLE
  #define DEBUG_PRINTLN(a) Serial.println(a)
  #define DEBUG_PRINT(a) Serial.print(a)
  #define DEBUG_PRINTLNF(a, f) Serial.println(a, f)
  #define DEBUG_PRINTF(a, f) Serial.print(a, f)
#else
  #define DEBUG_PRINTLN(a)
  #define DEBUG_PRINT(a)
  #define DEBUG_PRINTLNF(a, f)
  #define DEBUG_PRINTF(a, f)
#endif

//TODO: it looks like DMA processing is wrong. At least dma burst limit in 4096 uint32 words should be checked.
class DMABuffer
{
  public:
  lldesc_t descriptor;
  unsigned char* buffer;
  DMABuffer(uint32_t bytes)
  {
    buffer = (unsigned char *)malloc(bytes);
    descriptor.length = bytes;
    descriptor.size = descriptor.length;
    descriptor.owner = 1;
    descriptor.sosf = 1;
    descriptor.buf = (uint8_t*) buffer;
    descriptor.offset = 0;
    descriptor.empty = 0;
    descriptor.eof = 1;
    descriptor.qe.stqe_next = 0;
  }

  void next(DMABuffer *next)
  {
    descriptor.qe.stqe_next = &(next->descriptor);
  }

  int sampleCount() const
  {
    return descriptor.length / 2;
  }

  ~DMABuffer()
  {
    if(buffer)
      delete(buffer);
  }
};

bool ClockEnable(int pin, int Hz)
{
    periph_module_enable(PERIPH_LEDC_MODULE);

    ledc_timer_config_t timer_conf;
    timer_conf.bit_num = (ledc_timer_bit_t)1;
    timer_conf.freq_hz = Hz;
    timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        return false;
    }

    ledc_channel_config_t ch_conf;
    ch_conf.channel = LEDC_CHANNEL_0;
    ch_conf.timer_sel = LEDC_TIMER_0;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.duty = 1;
    ch_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    ch_conf.gpio_num = pin;
    err = ledc_channel_config(&ch_conf);
    if (err != ESP_OK) {
        return false;
    }
    return true;
}

void ClockDisable()
{
    periph_module_disable(PERIPH_LEDC_MODULE);
}

class I2C_AdcSampler
{
  public:
  static intr_handle_t i2sInterruptHandle;
  static DMABuffer *dmaBuffer;
  static unsigned char* frame;
  static uint32_t frameLength;
  static volatile bool stopSignal;

  typedef enum {
    /* camera sends byte sequence: s1, s2, s3, s4, ...
     * fifo receives: 00 s1 00 s2, 00 s2 00 s3, 00 s3 00 s4, ...
     */
    SM_0A0B_0B0C = 0,
    /* camera sends byte sequence: s1, s2, s3, s4, ...
     * fifo receives: 00 s1 00 s2, 00 s3 00 s4, ...
     */
    SM_0A0B_0C0D = 1,
    /* camera sends byte sequence: s1, s2, s3, s4, ...
     * fifo receives: 00 s1 00 00, 00 s2 00 00, 00 s3 00 00, ...
     */
    SM_0A00_0B00 = 3,
  } i2s_sampling_mode_t;


  static inline void i2sConfReset()
  {
    const uint32_t lc_conf_reset_flags = I2S_IN_RST_M | I2S_AHBM_RST_M | I2S_AHBM_FIFO_RST_M;
    I2S0.lc_conf.val |= lc_conf_reset_flags;
    I2S0.lc_conf.val &= ~lc_conf_reset_flags;
    
    const uint32_t conf_reset_flags = I2S_RX_RESET_M | I2S_RX_FIFO_RESET_M | I2S_TX_RESET_M | I2S_TX_FIFO_RESET_M;
    I2S0.conf.val |= conf_reset_flags;
    I2S0.conf.val &= ~conf_reset_flags;
    while (I2S0.state.rx_fifo_reset_back);
  }
  
  void start()
  {
    DEBUG_PRINTLN("Sampling started");
    i2sRun();
  }

  void stop()
  {
    stopSignal = true;
    while(stopSignal);
    DEBUG_PRINTLN("Sampling stopped");
  }

  void oneFrame()
  {
    start();
    stop();
  }
  
  static void i2sStop();
  static void i2sRun();

  static void dmaBufferInit(uint32_t bytes);
  static void dmaBufferDeinit();
  
  static void IRAM_ATTR i2sInterrupt(void* arg);
  
  static bool i2sInit(const int PCLK, const int D0, const int D1, const int D2, const int D3, const int D4, const int D5, const int D6, const int D7);
  static bool init(const uint32_t frameBytes, const int XCLK, const int PCLK, const int D0, const int D1, const int D2, const int D3, const int D4, const int D5, const int D6, const int D7);
};


//private variables
ESP32WebServer server(80);
intr_handle_t I2C_AdcSampler::i2sInterruptHandle = 0;
DMABuffer * I2C_AdcSampler::dmaBuffer = 0;
unsigned char* I2C_AdcSampler::frame = 0;
uint32_t I2C_AdcSampler::frameLength = 0;
volatile bool I2C_AdcSampler::stopSignal = false;
I2C_AdcSampler adcSampler;


void IRAM_ATTR I2C_AdcSampler::i2sInterrupt(void* arg)
{
    I2S0.int_clr.val = I2S0.int_raw.val;
    unsigned char* buf = dmaBuffer->buffer;
    uint32_t framePointer = 0;

    DEBUG_PRINTLN("I2S irq handler");
    //stop i2s
    if(stopSignal)
    {
      i2sStop();
      stopSignal = false;
    }
    
    //compress data
    for(uint32_t i = 0; i < frameLength * 2; i += 2)
    {
      frame[framePointer++] = buf[i];
    }
}

void I2C_AdcSampler::i2sStop()
{
    DEBUG_PRINTLN("I2S Stop");
    esp_intr_disable(i2sInterruptHandle);
    i2sConfReset();
    I2S0.conf.rx_start = 0;
}

void I2C_AdcSampler::i2sRun()
{
    DEBUG_PRINTLN("I2S Run");
    esp_intr_disable(i2sInterruptHandle);
    i2sConfReset();
    DEBUG_PRINT("Sample count ");
    DEBUG_PRINTLN(dmaBuffer->sampleCount());
    I2S0.rx_eof_num = dmaBuffer->sampleCount();
    I2S0.in_link.addr = (uint32_t)&(dmaBuffer->descriptor);
    I2S0.in_link.start = 1;
    I2S0.int_clr.val = I2S0.int_raw.val;
    I2S0.int_ena.val = 0;
    I2S0.int_ena.in_done = 1;
    esp_intr_enable(i2sInterruptHandle);
    I2S0.conf.rx_start = 1;
}

bool I2C_AdcSampler::init(const uint32_t frameBytes, const int XCLK, const int PCLK, const int D0, const int D1, const int D2, const int D3, const int D4, const int D5, const int D6, const int D7)
{
  ClockEnable(XCLK, 40000000); //sampling clock is 40Msps
  frameLength = frameBytes;
  frame = (unsigned char*)malloc(frameBytes);
  if(!frame)
  {
    DEBUG_PRINTLN("Not enough memory for frame buffer!");
    return false;
  }
  i2sInit(PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  dmaBufferInit(frameBytes * 2);  //two bytes per dword packing
  return true;
}

bool I2C_AdcSampler::i2sInit(const int PCLK, const int D0, const int D1, const int D2, const int D3, const int D4, const int D5, const int D6, const int D7)
{    
  int pins[] = {PCLK, D0, D1, D2, D3, D4, D5, D6, D7};    
  gpio_config_t conf = {
    .pin_bit_mask = 0,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
    for (int i = 0; i < sizeof(pins) / sizeof(gpio_num_t); ++i) {
        conf.pin_bit_mask = 1LL << pins[i];
        gpio_config(&conf);
    }

    // Route input GPIOs to I2S peripheral using GPIO matrix, last parameter is invert
    gpio_matrix_in(D0,    I2S0I_DATA_IN0_IDX, false);
    gpio_matrix_in(D1,    I2S0I_DATA_IN1_IDX, false);
    gpio_matrix_in(D2,    I2S0I_DATA_IN2_IDX, false);
    gpio_matrix_in(D3,    I2S0I_DATA_IN3_IDX, false);
    gpio_matrix_in(D4,    I2S0I_DATA_IN4_IDX, false);
    gpio_matrix_in(D5,    I2S0I_DATA_IN5_IDX, false);
    gpio_matrix_in(D6,    I2S0I_DATA_IN6_IDX, false);
    gpio_matrix_in(D7,    I2S0I_DATA_IN7_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN8_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN9_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN10_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN11_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN12_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN13_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN14_IDX, false);
    gpio_matrix_in(0x30,  I2S0I_DATA_IN15_IDX, false);

    //for i2s in parallel camera input mode data is receiver only when V_SYNC = H_SYNC = H_ENABLE = 1. We don't use these inputs so simply set them High
    gpio_matrix_in(0x38, I2S0I_V_SYNC_IDX, false);
    gpio_matrix_in(0x38, I2S0I_H_SYNC_IDX, false);  //0x30 sends 0, 0x38 sends 1
    gpio_matrix_in(0x38, I2S0I_H_ENABLE_IDX, false);
    gpio_matrix_in(PCLK, I2S0I_WS_IN_IDX, false);

    // Enable and configure I2S peripheral
    periph_module_enable(PERIPH_I2S0_MODULE);
    
    // Toggle some reset bits in LC_CONF register
    // Toggle some reset bits in CONF register
    i2sConfReset();
    // Enable slave mode (sampling clock is external)
    I2S0.conf.rx_slave_mod = 1;
    // Enable parallel mode
    I2S0.conf2.lcd_en = 1;
    // Use HSYNC/VSYNC/HREF to control sampling
    I2S0.conf2.camera_en = 1;
    // Configure clock divider
    I2S0.clkm_conf.clkm_div_a = 1;
    I2S0.clkm_conf.clkm_div_b = 0;
    I2S0.clkm_conf.clkm_div_num = 2;
    // FIFO will sink data to DMA
    I2S0.fifo_conf.dscr_en = 1;
    // FIFO configuration
    //two bytes per dword packing
    I2S0.fifo_conf.rx_fifo_mod = SM_0A0B_0C0D;  //pack two bytes in one UINT32
    I2S0.fifo_conf.rx_fifo_mod_force_en = 1;
    I2S0.conf_chan.rx_chan_mod = 1;
    // Clear flags which are used in I2S serial mode
    I2S0.sample_rate_conf.rx_bits_mod = 0;
    I2S0.conf.rx_right_first = 0;
    I2S0.conf.rx_msb_right = 0;
    I2S0.conf.rx_msb_shift = 0;
    I2S0.conf.rx_mono = 0;
    I2S0.conf.rx_short_sync = 0;
    I2S0.timing.val = 0;

    // Allocate I2S interrupt, keep it disabled
    esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, &i2sInterrupt, NULL, &i2sInterruptHandle);
    return true;
}

void I2C_AdcSampler::dmaBufferInit(uint32_t bytes)
{
  dmaBuffer = (DMABuffer*) malloc(sizeof(DMABuffer*));
  dmaBuffer = new DMABuffer(bytes);
}

void I2C_AdcSampler::dmaBufferDeinit()
{
    if (!dmaBuffer) return;
    delete(dmaBuffer);
    dmaBuffer = 0;
}

/* my page */
char mainPage[] = 
"<!DOCTYPE html>\n\
<html>\n\
  <head>\n\
  <!-- Plotly.js -->\n\
  <script src='https://cdn.plot.ly/plotly-latest.min.js'></script>\n\
  </head>\n\
  \n\
  <body>\n\
      <p style='text-align: left;'><strong>EspScope</strong></p>\n\
      <div id='myDiv'><!-- Plotly chart will be drawn inside this DIV --></div>\n\
      <hr />\n\
      <p style='text-align: left;'>\n\
          Open source <a href='github.com/easyvolts/espScope'>project</a> of esp32 based wireless oscilloscope. &nbsp; &nbsp; &nbsp; &nbsp;\n\
          Trigger mode:\n\
          <input type='radio' name='triggerSelection' value='auto'> Auto\n\
          <input type='radio' name='triggerSelection' value='single'> Single\n\
          <input type='radio' name='triggerSelection' value='none'  onclick='startUpdate()'> None\n\
          <input type='radio' name='triggerSelection' value='stop'  onclick='stopUpdate()' checked='checked'> Stop\n\
      </p>\n\
      <script>\n\
          <!-- JAVASCRIPT CODE GOES HERE -->\n\
          var data = [\n\
              { y: [0,0,0,0],\n\
                x: [0,1,2,3],\n\
                type: 'lines+markers' } ];\n\
          var counter = 1;\n\
          var intervalID;\n\
          \n\
          Plotly.newPlot('myDiv', data);\n\
          \n\
          function httpGet(theUrl)\n\
          {\n\
              var xmlHttp = new XMLHttpRequest();\n\
              xmlHttp.open( 'GET', theUrl, false ); /*false for synchronous request*/\n\
              xmlHttp.send( null );\n\
              return xmlHttp.responseText;\n\
          }\n\
          \n\
          function plotUpdate() {\n\
                  /*alert('called');*/\n\
                  scopeData = httpGet('/scope?');\n\
                  /* remove the first trace*/\n\
                  Plotly.deleteTraces('myDiv', 0);\n\
                  scopeDataArray = scopeData.split(',');\n\
                  Plotly.addTraces('myDiv', {y: scopeDataArray});\n\
                  /*Plotly.extendTraces('myDiv', {y: [1,2,3,1]}, [0]);*/\n\
          }\n\
          function startUpdate() {\n\
            intervalID = setInterval(plotUpdate, 300);\n\
          }\n\
          function stopUpdate() {\n\
            clearInterval(intervalID);\n\
          }\n\
          \n\
      </script>\n\
  </body>\n\
</html>";

void handleRoot() {
    server.send(200, "text/html", mainPage);
}

/* send data points in responce to "/scope?" request */
char str[50000];
void handleDataRequest() {
  int samplePointer = 0;
  int strPointer = 0;
  DEBUG_PRINTLN("Answer to the request.");
  adcSampler.oneFrame();
  //convert bin data to str
  for(samplePointer = 0; samplePointer < adcSampler.frameLength; samplePointer++) {
    strPointer += sprintf(&str[strPointer], "%u,", adcSampler.frame[samplePointer]);
  }
  //char str[] = "1,12,24,23,33,2"; //send data bytes only
  /* respond to the request */
  server.send(200, "text/plain", str);
}

/* cannot handle request so return 404 */
void handleNotFound(){
  String message = "File Not Found\n\n\n\n";
  server.send(404, "text/plain", message);
}

void setup(void){
  adcSampler.init(SAMPLES_COUNT, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  Serial.begin(115200);
  Serial.println("Application start");
  WiFi.begin(ssid, password);

  /* indication of connection process */
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* main page handler */
  server.on("/", handleRoot);
  /* this callback handle GPIO request and respond*/
  server.on("/scope", handleDataRequest);

  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
