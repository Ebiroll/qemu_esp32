/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "soc/rtc_cntl_reg.h" 
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
// Wifi/net related
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include <string.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "netif/etharp.h"
//#include "freertos/heap_regions.h"
#include "emul_ip.h"

#include "esp_eth.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"

#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"

#include "tcpip_adapter.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

static const char *TAG = "eth_demo";

extern err_t ethoc_init(struct netif *netif);

// Extern echo function
extern void echo_application_thread(void *pvParameters);


/* For testing of how code is generated  */
#if 0
/*         "wsr 	a3, ps\n"\ */
         "movi.n	a2, 0x3ffe0400\n"
         "movi.n	a3,0x400807BC\n"
        "movi.n	a2, 0x3ffe0400\n"\
        "movi.n	a3, 0x400807BC\n"\
        "s32i.n        a2, a3 , 0\n"\

#endif

	 // Load a9 with adress to user_entr  
	 // Load a8 with adress to elf_entry  
void patch() {
    asm volatile (\
    "j      0x40007023\n"\
	"nop.n\n"\
	"nop\n"\
	"l32r     a9,0x3ffe040\n"\
	"l32r     a8,0x40\n"\
    "s32i.n   a9,a8,0\n"\
	"movi.n	a2, 0\n");
}

void retint() {
      unsigned int*mem_location=(unsigned int*)0x40000000;
      *mem_location=0x3ffe0400;
}

int test(int in) {
    return (in & 0xfffffff7);
}

void jump() {
    asm volatile (\
         "RFE\n"\
         "jx a0\n");
    retint();
}


/* produce simple hex dump output */
void simpleHex(int toOutout)
{

    printf( "%08X", toOutout);
}



void dump_task(void *pvParameter)
{
    unsigned char*mem_location=(unsigned char*)0x40000000;
    unsigned int*simple_mem_location=(unsigned int*)mem_location;
    unsigned int* end=(unsigned int*)0x400C1FFF;

    printf("\n");

    int *GPIO_STRAP_TEST=(int *)0x3ff44038;
    printf( "GPIO STRAP REG=%08X\n", *GPIO_STRAP_TEST);

    //RTC_CNTL_RESET_STATE_REG  3ff48034

    int *test=(int *)RTC_CNTL_RESET_STATE_REG;
    printf( "RTC_CNTL_RESET_STATE_REG,%08X=%08X\n",RTC_CNTL_RESET_STATE_REG, *test);


    test=(int *)RTC_CNTL_INT_ST_REG;
    printf( "RTC_CNTL_INT_ST_REG; %08X=%08X\n",RTC_CNTL_INT_ST_REG, *test);

    // RTCIO_RTC_GPIO_PIN3_REG 

    //test=(int *)RTCIO_RTC_GPIO_PIN3_REG;
    //printf( "RTCIO_RTC_GPIO_PIN3_REG; %08X=%08X\n",RTCIO_RTC_GPIO_PIN3_REG, *test);


    test=(int *)0x3FF00044;
    printf( "GPIO_STATUS_REG ; %08X=%08X\n",0x3FF00044, *test);

    test=(int *)0x3FF00040;
    printf( "GPIO_IN1_REG 32-39; %08X=%08X\n",0x3FF00040, *test);


    test=(int *)0x3FF003f0;
    printf( "DPORT DPORT_PRO_DCACHE_DBUG0_REG REG; %08X=%08X\n",0x3FF003f0, *test);


    test=(int *)0x3FF00038;
    printf( "DPORT 38 REG; %08X=%08X\n",0x3FF00038, *test);

    test=(int *)0x3FF4800c;
    printf( "DPORT RTC_CNTL_TIME_UPDATE_REG REG; %08X=%08X\n",0x3FF4800c, *test);

    printf("ROM DUMP\n");
    printf("\n");
    printf("\n");
    fflush(stdout);

    unsigned int simple_crc=0;
    unsigned int pos=0;

    while(simple_mem_location<end) {
        simpleHex(*simple_mem_location);
        simple_crc=simple_crc ^ (*simple_mem_location);
        printf(",");
        if (pos++==7) {
            printf( ":%08X:\n", simple_crc);
            fflush(stdout);
            simple_crc=0;
            pos=0;
            //vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        simple_mem_location++;
    }
    fflush(stdout);


    printf("\n");
    printf("\n");
    printf("ROM1 Dump.\n");
    fflush(stdout);

    simple_crc=0;
    pos=0;
    // rom1 3FF9_0000 0x3FF9_FFFF
    mem_location=(unsigned char*)0x3FF90000;
    simple_mem_location=(unsigned int*)mem_location;
    end=(unsigned int*)0x3FF9FFFF;

    while(simple_mem_location<end) {
        simpleHex(*simple_mem_location);
        simple_crc=simple_crc ^ (*simple_mem_location);
        printf(",");
        if (pos++==7) {
            printf( ":%08X:\n", simple_crc);
            fflush(stdout);
            simple_crc=0;
            pos=0;
            //vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        simple_mem_location++;
    }
    fflush(stdout);



    printf("\n");
    printf("Done\n");
    printf("\n");


    int times=0;
    while (true) {
	    printf("Done %d\n",times++);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }

#if 0

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    while (true) {
        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
	    printf("blink\n");
        printf( "DPORT RTC_CNTL_TIME_UPDATE_REG REG; %08X=%08X\n",0x3FF00038, *test);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
#endif
    //system_restart();
}

/*
typedef struct {
    system_event_id_t     event_id;     
    system_event_info_t   event_info;   
    } system_event_t;
*/

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_GOT_IP:
        {
            struct netif *netif;
            // List all available interfaces
            for (netif = netif_list; netif != NULL; netif = netif->next) {
                printf("%c%c%d\n",netif->name[0],netif->name[1],netif->num);
                fflush(stdout);
            }
        }
        default:
          break;
    }
    return ESP_OK;
}


void wifi_task(void *pvParameter) {
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t sta_config = {
        .sta = {
	     //#include "secret.h"
	   .ssid = "ssid",
	   .password = "password",
	//.bssid_set = false
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_ERROR_CHECK( esp_wifi_connect() );
    printf("Connected\n");

    xTaskCreate(&echo_application_thread, "echo_thread", 2048, NULL, 5, NULL);


    //gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    int level = 0;
    // Send arp request

    struct netif *netif;

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        printf("%c%c%d\n",netif->name[0],netif->name[1],netif->num);
        fflush(stdout);
    }


    ip4_addr_t scanaddr;
    ip4_addr_t *cacheaddr;
    struct eth_addr *cachemac;

    netif=netif_find("en0");
    if (!netif) {
        printf("No en0");
    }

    unsigned char hostnum=1;
    char tmpBuff[20];
    // Arpscan network
    while (true) {

        sprintf(tmpBuff,"192.168.1.%d",hostnum);
        IP4_ADDR(&scanaddr, 192, 168 , 1, hostnum);


        //gpio_set_level(GPIO_NUM_5, level);
        if (netif)
        {
            printf("ARP request %s\n",tmpBuff);
            err_t ret=etharp_request(netif, &scanaddr);
            if (ret<0) {
                printf("Failed request %s\n",tmpBuff);
            }

            struct netif *chacheif=netif;
            for (int j=0;j<ARP_TABLE_SIZE;j++) {
                        if (1==etharp_get_entry(j, &cacheaddr, &chacheif, &cachemac))
                        {
                            printf("Found %d  %d.%d.%d.%d\n",j,IP2STR(cacheaddr));
                        }
            }

            // Periodically call etharp_cleanup_netif 	( 	struct netif *  	netif	) 	
            // To clean cache

        }

        level = !level;
	    //printf(".");
        vTaskDelay(300 / portTICK_PERIOD_MS);
        hostnum++;
    }
}



void emulated_net(void *pvParameter) {

    //int i;
    // We cant use this for emulated network, need to setup tcpip manually
    tcpip_adapter_init();
    //ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

#if 0
    for (i=0; regions[i].xSizeInBytes!=0; i++) {                                                                                                                                 <
                //if (regions[i].xTag != -1) {                                                                                                                                             <
                    printf("Region %02d: %08X len %08X tag %d", i,                                                                                                          <
                            (int)regions[i].pucStartAddress, regions[i].xSizeInBytes, regions[i].xTag);                                                                                  <
                //}                                                                                                                                                                        <
    }                         
                                                                                                                                                                                  <
    for (i=1; regions[i].xSizeInBytes!=0; i++) {
        /*                                                                                                                                 <
        if (regions[i].pucStartAddress == (regions[i-1].pucStartAddress + regions[i-1].xSizeInBytes) &&                                                                          <
                                    regions[i].xTag == regions[i-1].xTag ) {                                                                                                     <
            regions[i-1].xTag=-1;                                                                                                                                                <
            regions[i].pucStartAddress=regions[i-1].pucStartAddress;                                                                                                             <
            regions[i].xSizeInBytes+=regions[i-1].xSizeInBytes;                                                                                                                  <
        } 
        */                                                                                                                                                                       <
    }                         
#endif



    task_lwip_init(NULL);

    //gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    // Send arp request

    struct netif *netif;

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        printf("%c%c%d\n",netif->name[0],netif->name[1],netif->num);
        fflush(stdout);
    }


    ip4_addr_t scanaddr;
    ip4_addr_t *cacheaddr;
    struct eth_addr *cachemac;


    netif=netif_find("en0");
    if (!netif) {
        printf("No en0");
    }

    xTaskCreate(&echo_application_thread, "echo_thread", 2048, NULL, 15, NULL);

    unsigned char hostnum=1;
    char tmpBuff[20];
    // Arpscan network
    while (true) {
#if 1
      // ARP requests
        sprintf(tmpBuff,"192.168.1.%d",hostnum);
        IP4_ADDR(&scanaddr, 192, 168 , 4, hostnum);


        //gpio_set_level(GPIO_NUM_4, level);
        if (netif)
        {
            //printf("ARP request %s\n",tmpBuff);
            //err_t ret=etharp_request(netif, &scanaddr);
            //if (ret<0) {
            //    printf("Failed request %s\n",tmpBuff);
            //}
        }

        level = !level;
	    //printf(".");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        hostnum++;
        struct netif *chacheif=netif;
        for (int j=0;j<ARP_TABLE_SIZE;j++) {
                    if (1==etharp_get_entry(j, &cacheaddr, &chacheif, &cachemac))
                    {
                        printf("%d  %d.%d.%d.%d\n",j,IP2STR(cacheaddr));
                    }
        }
#endif
    }
}


void phy_tlk110_init(void)
{
    esp_eth_smi_write(0x1f, 0x8000);
    ets_delay_us(20000);

    while (esp_eth_smi_read(0x2) != 0x2000) {
    }

    esp_eth_smi_write(0x9, 0x7400);
    esp_eth_smi_write(0x9, 0xf400);
    ets_delay_us(20000);
}

void eth_gpio_config_rmii(void)
{
    //txd0 to gpio19 ,can not change
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO19_U, 5);
    //rx_en to gpio21 ,can not change
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO21_U, 5);
    //txd1 to gpio22 , can not change
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO22_U, 5);
    //rxd0 to gpio25 , can not change
    gpio_set_direction(25, GPIO_MODE_INPUT);
    //rxd1 to gpio26 ,can not change
    gpio_set_direction(26, GPIO_MODE_INPUT);
    //rmii clk  ,can not change
    gpio_set_direction(0, GPIO_MODE_INPUT);

    //mdc to gpio4
    gpio_matrix_out(4, EMAC_MDC_O_IDX, 0, 0);
    //mdio to gpio2
    gpio_matrix_out(2, EMAC_MDO_O_IDX, 0, 0);
    gpio_matrix_in(2, EMAC_MDI_I_IDX, 0);
}

void eth_task(void *pvParameter)
{
    tcpip_adapter_ip_info_t ip;
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    vTaskDelay(2000 / portTICK_RATE_MS);

    while (1) {

        vTaskDelay(2000 / portTICK_RATE_MS);

        if (tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip) == 0) {
            ESP_LOGI(TAG, "\n~~~~~~~~~~~\n");
            ESP_LOGI(TAG, "ETHIP:"IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "ETHPMASK:"IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "ETHPGW:"IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "\n~~~~~~~~~~~\n");
        }
    }
}


void ethernet_main()
{
    esp_err_t ret = ESP_OK;
    tcpip_adapter_init();
    esp_event_loop_init(NULL, NULL);

    eth_config_t config;
    config.phy_addr = PHY31;
    config.mac_mode = ETH_MODE_RMII;
    config.phy_init = phy_tlk110_init;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;

    ret = esp_eth_init(&config);
    if(ret == ESP_OK) {
        esp_eth_enable();
        xTaskCreate(eth_task, "eth_task", 2048, NULL, (tskIDLE_PRIORITY + 2), NULL);
    }

}


void dump_mac_with_crc(void *pvParameter)
{
      //case 0x5a004:
      //case 0x5a008:
    int *test=(int *) 0x3ff5a004;
    printf( "EFUSE MAC ,%08X=%08X\n",(unsigned int)test, *test);

    test=(int *) 0x3ff5a008;
    printf( "EFUSE MAC ,%08X=%08X\n",(unsigned int)test, *test);

}

void dump_regs(void *pvParameter)
{
    unsigned char*mem_location=(unsigned char*)0x40000000;
    unsigned int*simple_mem_location=(unsigned int*)mem_location;
    unsigned int* end=(unsigned int*)0x400C1FFF;
    int j=0;

    printf("\n");

    int *GPIO_STRAP_TEST=(int *)0x3ff44038;
    printf( "GPIO STRAP REG=%08X\n", *GPIO_STRAP_TEST);

    int *test=(int *)RTC_CNTL_RESET_STATE_REG;
    printf( "RTC_CNTL_RESET_STATE_REG,%08X=%08X\n",RTC_CNTL_RESET_STATE_REG, *test);

    test=(int *)RTC_CNTL_INT_ST_REG;
    printf( "RTC_CNTL_INT_ST_REG; %08X=%08X\n",RTC_CNTL_INT_ST_REG, *test);

#if 0
    case 0x1c018:
        //return 0;
        return 0x980020b6;
        // Some difference should land between these values
              // 0x980020c0;
              // 0x980020b0;
        //return   0x800000;
    case 0x33c00:
#endif

    for (j=0;j<4;j++) {
        test=(int *)0x6001c018;
        printf( "0x6001c018 ; %08X=%08X\n",0x6001c018, *test);

        test=(int *)0x60033c00;
        printf( "0x60033c00; %08X=%08X\n",0x60033c00, *test);
    }

}

extern uint8_t rom_i2c_readReg(uint8_t block, uint8_t host_id, uint8_t reg_add);

void dump_i2c_regs()
{
  uint8_t j;
  uint8_t ret=rom_i2c_readReg(0x62,0x01,0x06);

  printf( "rom_i2c_reg block 0x62 reg 0x6 %02x\n",ret);

  for (j=0;j<16;j++) {
    ret=rom_i2c_readReg(0x62,0x01,j);
    printf( "%02x\n",ret);
  }

  ret=rom_i2c_readReg(0x67,0x01,0x06);

  printf( "rom_i2c_reg block 0x67 reg 0x6 %02x\n",ret);

  for (j=0;j<16;j++) {
    ret=rom_i2c_readReg(0x67,0x01,j);
    printf( "%02x\n",ret);
  }


}

extern void sys_init();

void app_main()
{

    sys_init();
    esp_log_level_set("*", ESP_LOG_INFO);
    nvs_flash_init();
    dump_mac_with_crc(NULL);

    //asm("break.n 1");
    //xTaskCreate(&wifi_task,"wifi_task",2048, NULL, 5, NULL);
    // Better run in main thread for faster processing
    //xTaskCreate(&emulated_net, "emulated_net", 4096, NULL, 5, NULL);
    //emulated_net(NULL);

    //dump_i2c_regs();
    if (is_running_qemu()) {
	tcpip_adapter_init();
        xTaskCreate(&emulated_net, "emulated_net",2*4096, NULL, 20, NULL); 
	xTaskCreate(&echo_application_thread, "echo_thread", 2048, NULL, 5, NULL);
        // emulated_net(NULL);
    } else {
        xTaskCreate(&wifi_task,"wifi_task",2*2048, NULL, 5, NULL);
    }
    dump_regs(NULL);
    //ethernet_main();

    // Dumping rom is best done with the esptool.py
    //xTaskCreate(&dump_task, "dump_task", 2048, NULL, 5, NULL);
}
