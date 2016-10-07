/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "esp_system.h"
//#include "nvs_flash.h"

/* produce intel hex file output... call this routine with */
/* each byte to output and it's memory location.  The file */
/* pointer fhex must have been opened for writing.  After */
/* all data is written, call with end=1 (normally set to 0) */
/* so it will flush the data from its static buffer */

#define MAXHEXLINE 32	/* the maximum number of bytes to put in one line */

void hexout(char byte, int memory_location,int end)
{
    static int byte_buffer[MAXHEXLINE];
    static int last_mem, buffer_pos, buffer_addr;
    static int writing_in_progress=0;
    register int i, sum;

    if (!writing_in_progress) {
        /* initial condition setup */
        last_mem = memory_location-1;
        buffer_pos = 0;
        buffer_addr = memory_location;
        writing_in_progress = 1;
        }

    if ( (memory_location != (last_mem+1)) || (buffer_pos >= MAXHEXLINE) \
     || ((end) && (buffer_pos > 0)) ) {
        /* it's time to dump the buffer to a line in the file */
        printf(":%02X%04X00", buffer_pos, buffer_addr);
        sum = buffer_pos + ((buffer_addr>>8)&255) + (buffer_addr&255);
        for (i=0; i < buffer_pos; i++) {
            printf( "%02X", byte_buffer[i]&255);
            sum += byte_buffer[i]&255;
        }
        printf("%02X\n", (-sum)&255);
        buffer_addr = memory_location;
        buffer_pos = 0;
    }

    if (end) {
        printf( ":00000001FF\n");  /* end of file marker */
        fflush(stdout);
        writing_in_progress = 0;
    }

    last_mem = memory_location;
    byte_buffer[buffer_pos] = byte & 255;
    buffer_pos++;
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
    printf("Hex Dump!\n");
    //unsigned char*mem_location=0x40000000;
    //unsigned char*mem_location=hello_task;
    unsigned char*mem_location=test_data;

    //0x400C_1FFF


    //void hexout(int byte, int memory_location,int end)
    while(mem_location<1+test_data+sizeof(test_data)) {
        hexout(*mem_location,mem_location,0);
        //printf("%02X",*mem_location);
        mem_location++;
    }
    hexout(*mem_location,mem_location,1);

    printf("Done.\n");
    fflush(stdout);
    //system_restart();
}

void app_main()
{
    //nvs_flash_init();
    //system_init();
    xTaskCreate(&dump_task, "dump_task", 2048, NULL, 5, NULL);
}
