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

/* For testing of how code is generated  */
#if 0
/*         "wsr 	a3, ps\n"\ */
         "movi.n	a2, 0x3ffe0400\n"
         "movi.n	a3,0x400807BC\n"
        "movi.n	a2, 0x3ffe0400\n"\
        "movi.n	a3, 0x400807BC\n"\
        "s32i.n        a2, a3 , 0\n"\

#endif

	 // Load a9 with adress to user_entr  (+20)
	 // Load a8 with adress to elf_entry  (+40)
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


unsigned char test_data[]={0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                 0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                 0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                 0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                 0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                 0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                 0xFF,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};


void dump_task(void *pvParameter)
{
    unsigned char*mem_location=(unsigned char*)0x40000000;
    //unsigned char*mem_location=test_data;
    unsigned int*simple_mem_location=(unsigned int*)mem_location;
    unsigned int* end=(unsigned int*)0x400C1FFF;


    printf("\n");

    int *GPIO_STRAP_TEST=(int *)0x3ff44038;
    printf( "GPIO STRAP REG=%08X\n", *GPIO_STRAP_TEST);

    int *test=(int *)RTC_CNTL_RESET_STATE_REG;
    printf( "RTC_CNTL_RESET_STATE_REG,%08X=%08X\n",RTC_CNTL_RESET_STATE_REG, *test);

    //RTC_CNTL_RESET_STATE_REG  3ff48034

    test=(int *)RTC_CNTL_INT_ST_REG;
    printf( "RTC_CNTL_INT_ST_REG; %08X=%08X\n",RTC_CNTL_INT_ST_REG, *test);

    test=(int *)0x3FF00044;
    printf( "DPORT 44 REG; %08X=%08X\n",0x3FF00044, *test);

    test=(int *)0x3FF00040;
    printf( "DPORT 40 REG; %08X=%08X\n",0x3FF00040, *test);


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

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    int level = 0;
    while (true) {
        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
	printf("blink\n");
        printf( "DPORT RTC_CNTL_TIME_UPDATE_REG REG; %08X=%08X\n",0x3FF00038, *test);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    //system_restart();
}

void app_main()
{

    esp_log_level_set("*", ESP_LOG_INFO);
    nvs_flash_init();
    system_init();
    xTaskCreate(&dump_task, "dump_task", 2048, NULL, 5, NULL);
}
