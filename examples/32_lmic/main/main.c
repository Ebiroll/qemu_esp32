#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "lmic.h"

const unsigned TX_INTERVAL = 30;



u1_t NWKSKEY[16] = { 0x33, 0xDA, 0xEF, 0x09, 0x56, 0xEB, 0x8E, 0xA6, 0xB3, 0x8F, 0xDC, 0x72, 0xB1, 0xEE, 0xE6, 0x69 };
u1_t APPSKEY[16] = { 0x7E, 0x34, 0x7E, 0x65, 0x93, 0x26, 0x90, 0x79, 0x5B, 0x46, 0xBC, 0x7E, 0xEA, 0x16, 0x88, 0x83 };
u4_t DEVADDR = 0x262715DE ;
//u4_t DEVADDR = 0x260115DE ;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "Uela!";


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
        case EV_TXCOMPLETE:
            printf("EV_TXCOMPLETE (includes waiting for RX windows)");
            if (LMIC.txrxFlags & TXRX_ACK)
              printf("Received ack");
            if (LMIC.dataLen) {
              printf("Received ");
              printf(LMIC.dataLen);
              printf(" bytes of payload");
            }

            if (LMIC.opmode & OP_TXRXPEND) {
                printf("OP_TXRXPEND, not sending");
            } else {
                // Prepare upstream data transmission at the next possible time.
                LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
                printf("Packet queued");
            }
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

  if (LMIC.opmode & OP_TXRXPEND) {
      printf("OP_TXRXPEND, not sending");
  } else {
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
      printf("Packet queued");
  }


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
  LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);

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

  xTaskCreate(os_runloop, "os_runloop", 1024 * 2, (void* )0, 10, NULL);
}
