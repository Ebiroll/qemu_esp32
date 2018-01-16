#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lmic.h"

#define LOW    0
#define HIGH   1

const unsigned TX_INTERVAL = 30;

#define BUILTIN_LED 25


static void portc_init(void)
{
	    gpio_num_t pins[] = {
            BUILTIN_LED
			// JTAG, on the wrover kit
            //12,
            //13,
            //14,
            //15,
			// Other
            //16,
            //17
            //18,
            //19,
            //20,
            //21,
            //22,
            //23,
            //24,
            //25,
            //26,
            //27
    };
    gpio_config_t conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
    };
    for (int i = 0; i < sizeof(pins) / sizeof(gpio_num_t); ++i) {
        conf.pin_bit_mask = 1LL << pins[i];
        gpio_config(&conf);
    }

	//int level0=gpio_get_level(0);
	//int level16=gpio_get_level(16);

#if 0
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Speed = GPIO_SPEED_HIGH;
	gpio_init.Pull = GPIO_PULLDOWN;
#endif
}


static osjob_t sendjob;

u1_t NWKSKEY[16] = { 0x33, 0xDA, 0xEF, 0x09, 0x56, 0xEB, 0x8E, 0xA6, 0xB3, 0x8F, 0xDC, 0x72, 0xB1, 0xEE, 0xE6, 0x69 };
u1_t APPSKEY[16] = { 0x7E, 0x34, 0x7E, 0x65, 0x93, 0x26, 0x90, 0x79, 0x5B, 0x46, 0xBC, 0x7E, 0xEA, 0x16, 0x88, 0x83 };
u4_t DEVADDR = 0x262715DE ;
//u4_t DEVADDR = 0x260115DE ;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "Uela!";

void do_send(osjob_t* j) {
    
  printf ("do_send %d\n",os_getTime());

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    printf("OP_TXRXPEND, not sending\n");
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, mydata, sizeof(mydata) , 0);
    printf("Packet queued");
    gpio_set_level(BUILTIN_LED, HIGH);

  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    printf("%d", os_getTime());
    printf(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            printf("EV_SCAN_TIMEOUT");
            break;
        case EV_BEACON_FOUND:
            printf("EV_BEACON_FOUND");
            break;
        case EV_BEACON_MISSED:
            printf("EV_BEACON_MISSED");
            break;
        case EV_BEACON_TRACKED:
            printf("EV_BEACON_TRACKED");
            break;
        case EV_JOINING:
            printf("EV_JOINING");
            break;
        case EV_JOINED:
            printf("EV_JOINED");
            break;
        case EV_RFU1:
            printf("EV_RFU1");
            break;
        case EV_JOIN_FAILED:
            printf("EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
            printf("EV_REJOIN_FAILED");
            break;
        case EV_TXSTART:
            printf("EV_TXSTART");
            break;        
        case EV_TXCOMPLETE:
            printf("EV_TXCOMPLETE (includes waiting for RX windows)");
            gpio_set_level(BUILTIN_LED, LOW);
            if (LMIC.txrxFlags & TXRX_ACK)
              printf("Received ack");
            if (LMIC.dataLen) {
              printf("Received ");
              printf(LMIC.dataLen);
              printf(" bytes of payload");
            }

           /*
            if (LMIC.opmode & OP_TXRXPEND) {
                printf("OP_TXRXPEND, not sending");
            } else {
                // Prepare upstream data transmission at the next possible time.
                LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
                printf("Packet queued");
            }
            */

            printf ("time %d\n",os_getTime());
             os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
            printf ("time callback %d\n",os_getTime()+ sec2osticks(TX_INTERVAL));

            break;
        case EV_LOST_TSYNC:
            printf("EV_LOST_TSYNC");
            break;
        case EV_RESET:
            printf("EV_RESET");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            printf("EV_RXCOMPLETE");
            break;
        case EV_LINK_DEAD:
            printf("EV_LINK_DEAD");
            break;
        case EV_LINK_ALIVE:
            printf("EV_LINK_ALIVE");
            break;
         default:
            printf("Unknown event: %d", ev);
            break;
    }
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void os_runloop(void) {

  printf ("os_runloop %d\n",os_getTime());

  if (LMIC.opmode & OP_TXRXPEND) {
      printf("OP_TXRXPEND, not sending");
  } else {
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
      printf("Packet queued");
  }

  printf ("time %d\n",os_getTime());

  while(1) {
    os_run();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void app_main(void)
{
  os_init();

  LMIC_reset();
  printf("LMIC RESET");

  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);

  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI); // g2-band

  LMIC_setLinkCheckMode(0);
  LMIC.dn2Dr = DR_SF9;
  LMIC_setDrTxpow(DR_SF7,14);

  for(int i = 1; i <= 8; i++) LMIC_disableChannel(i);
  portc_init();

  do_send(&sendjob);

  gpio_set_level(BUILTIN_LED, LOW);

  //xTaskCreate(os_runloop, "os_runloop", 1024 * 4, (void* )0, 10, NULL);
  os_runloop();
}
