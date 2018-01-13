#include "lmic.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/timer.h"
#include "esp_log.h"
#include <sys/time.h>


// GPIO5 — SX1278’s SCK
// GPIO19 — SX1278’s MISO
// GPIO27 — SX1278’s MOSI
// GPIO18 — SX1278’s CS
// GPIO14 — SX1278’s RESET
// GPIO26 — SX1278’s IRQ(Interrupt Request)

// GPIO25 — LED

/*
    .miso_io_num = lmic_pins.spi[0],
    .mosi_io_num = lmic_pins.spi[1],
    .sclk_io_num = lmic_pins.spi[2],
*/


// Pin mapping, HELTEC
const lmic_pinmap_t lmic_pins = {
 .nss = 18,
 //.rxtx = LMIC_UNUSED_PIN,
 .rst = 14,
 .dio = { 26, 33 ,32 },
 .spi = { 19, 27, 5 }
};
#if 0
const lmic_pinmap_t lmic_pins = {
    .nss = 22,
    .rst = 19,
    .dio = {23, 18, 5},
    .spi = {4, 2, 17},
};
#endif

// -----------------------------------------------------------------------------
// I/O

static void hal_io_init () {
  ESP_LOGI(TAG, "Starting IO initialization");

  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1<<lmic_pins.nss) | (1<<lmic_pins.rst);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1<<lmic_pins.dio[0]) | (1<<lmic_pins.dio[1]) | (1<<lmic_pins.dio[2]);
  gpio_config(&io_conf);

  ESP_LOGI(TAG, "Finished IO initialization");
}

void hal_pin_rxtx (u1_t val) {
  // unused
}


// set radio NSS pin to given value
void hal_pin_nss (u1_t val) {
  gpio_set_level(lmic_pins.nss, val);
}

// set radio RST pin to given value (or keep floating!)
void hal_pin_rst (u1_t val) {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  io_conf.pin_bit_mask = (1<<lmic_pins.rst);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;

  if(val == 0 || val == 1) { // drive pin
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);

    gpio_set_level(lmic_pins.rst, val);
  } else { // keep pin floating
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);
  }
}

bool dio_states[3];

void hal_io_check() {
  if (dio_states[0] != gpio_get_level(lmic_pins.dio[0])) {
    dio_states[0] = !dio_states[0];
    if (dio_states[0]) {
      ESP_LOGI(TAG, "Fired IRQ0\n");
      radio_irq_handler(0);
    }
  }

  if (dio_states[1] != gpio_get_level(lmic_pins.dio[1])) {
    dio_states[1] = !dio_states[1];
    if (dio_states[1]) {
      ESP_LOGI(TAG, "Fired IRQ1\n");
      radio_irq_handler(1);
    }
  }

  if (dio_states[2] != gpio_get_level(lmic_pins.dio[2])) {
    dio_states[2] = !dio_states[2];
    if (dio_states[2]) {
      ESP_LOGI(TAG, "Fired IRQ2\n");
      radio_irq_handler(2);
    }
  }
}

// -----------------------------------------------------------------------------
// SPI

spi_device_handle_t spi_handle;

static void hal_spi_init () {
  ESP_LOGI(TAG, "Starting SPI initialization");
  esp_err_t ret;

  // init master
  spi_bus_config_t buscfg={
    .miso_io_num = lmic_pins.spi[0],
    .mosi_io_num = lmic_pins.spi[1],
    .sclk_io_num = lmic_pins.spi[2],
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
  };

  // init device
  spi_device_interface_config_t devcfg={
    .clock_speed_hz = 10000000,
    .mode = 1,
    .spics_io_num = -1,
    .queue_size = 7,
  };

  ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
  assert(ret==ESP_OK);

  ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi_handle);
  assert(ret==ESP_OK);

  ESP_LOGI(TAG, "Finished SPI initialization");
}
// Write data. first element of data pointer is address
// perform SPI transaction with radio
u1_t hal_spi (u1_t data) {
  uint8_t rxData = 0;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8;
  t.rxlength = 8;
  t.tx_buffer = &data;
  t.rx_buffer = &rxData;
  esp_err_t ret = spi_device_transmit(spi_handle, &t);
  assert(ret == ESP_OK);

  return (u1_t) rxData;
}

// -----------------------------------------------------------------------------
// TIME

static void hal_time_init () {
  ESP_LOGI(TAG, "Starting initialisation of timer");
  int timer_group = TIMER_GROUP_0;
  int timer_idx = TIMER_1;
  timer_config_t config;
  config.alarm_en = 0;
  config.auto_reload = 0;
  config.counter_dir = TIMER_COUNT_UP;
  config.divider = 120;
  config.intr_type = 0;
  config.counter_en = TIMER_PAUSE;
  /*Configure timer*/
  timer_init(timer_group, timer_idx, &config);
  /*Stop timer counter*/
  timer_pause(timer_group, timer_idx);
  /*Load counter value */
  timer_set_counter_value(timer_group, timer_idx, 0x0);
  /*Start timer counter*/
  timer_start(timer_group, timer_idx);

  ESP_LOGI(TAG, "Finished initalisation of timer");
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

u4_t hal_ticks () {
  uint64_t val;
  timer_get_counter_value(TIMER_GROUP_0, TIMER_1, &val);
  //ESP_LOGD(TAG, "Getting time ticks");
  printf("Getting some ticks\n");
  //vTaskDelay(1 / portTICK_PERIOD_MS);
  uint32_t t = (uint32_t) val;
  //u4_t result = (u4_t) us2osticks(t);
  return t;
}

// Returns the number of ticks until time. Negative values indicate that
// time has already passed.
static s4_t delta_time(u4_t time) {
    return (s4_t)(time - hal_ticks());
}


void hal_waitUntil (u4_t time) {

    ESP_LOGI(TAG, "Wait until");
    s4_t delta = delta_time(time);

    while( delta > 2000){
        vTaskDelay(1 / portTICK_PERIOD_MS);
        delta -= 1000;
    } if(delta > 0){
        vTaskDelay(delta / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Done waiting until");
}

// check and rewind for target time
u1_t hal_checkTimer (u4_t time) {
  return 1;
}

int x_irq_level = 0;

// -----------------------------------------------------------------------------
// IRQ

void hal_disableIRQs () {
  //ESP_LOGD(TAG, "Disabling interrupts");
  if(x_irq_level < 1){
      //taskDISABLE_INTERRUPTS();
  }
  x_irq_level++;
}

void hal_enableIRQs () {
  //ESP_LOGD(TAG, "Enable interrupts");
  if(--x_irq_level == 0){
      //taskENABLE_INTERRUPTS();
      hal_io_check();
  }
}

void hal_sleep () {
  // unused
}

// -----------------------------------------------------------------------------

void hal_init () {
    // configure radio I/O and interrupt handler
    hal_io_init();
    // configure radio SPI
    hal_spi_init();
    // configure timer and interrupt handler
    hal_time_init();
}

void hal_failed () {
    // HALT...
    hal_disableIRQs();
    hal_sleep();
    while(1);
}
