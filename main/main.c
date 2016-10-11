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

/* For testing of how code is generated  */
#if 0
/*         "wsr 	a3, ps\n"\ */

void retint() {
    asm volatile (\
        "wsr 	a3 , ps\n"\
        "movi.n	a8, -9\n"\
        "and	a2, a2, a8\n"\
        "RFDE\n");
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
#endif

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
    //unsigned char*mem_location=dump_task;
    //unsigned char*mem_location=test_data;
    unsigned int*simple_mem_location=(unsigned int*)mem_location;
    unsigned int* end=(unsigned int*)0x400C1FFF;

    printf("\n");
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
    printf("Done.\n");
    fflush(stdout);
    //system_restart();
}

void app_main()
{
    nvs_flash_init();
    system_init();
    xTaskCreate(&dump_task, "dump_task", 2048, NULL, 5, NULL);
}
