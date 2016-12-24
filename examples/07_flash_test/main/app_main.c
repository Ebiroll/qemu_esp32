#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
#include "rom/spi_flash.h"
#include <string.h>
#include <stdlib.h>
#include "esp_partition.h"
#include "esp_flash_data_types.h"
#include "nvs_flash.h"

static uint32_t g_wbuf[SPI_FLASH_SEC_SIZE / 4];
static uint32_t g_rbuf[SPI_FLASH_SEC_SIZE / 4];

#define ESP_PARTITION_TABLE_ADDR 0x8000


int printable(char c)

{
  return ((c >= ' ' && c <= '~') ? 1 : 0);
}

void dumpPartitionData() {

    spi_flash_read(ESP_PARTITION_TABLE_ADDR , g_rbuf, sizeof(g_rbuf));
    //esp_partition_t* p = (esp_partition_t*) g_rbuf;

    const esp_partition_info_t* it = (const esp_partition_info_t*) g_rbuf;

    printf("Partition data\n");

    while(it+sizeof(esp_partition_info_t)<g_rbuf+sizeof(g_rbuf)) {
        if (it->magic==0x50aa) {
            if (it->type==0) {
                printf("Type APP, subtype %d  %s\n",it->subtype,it->label);
            } else if (it->type==1) {
                printf("Type DATA, subtype %d  %s\n",it->subtype,it->label);
            } else {
                  printf("Type %d , subtype %d  %s\n",it->type,it->subtype,it->label);
            }
            printf("Offset %08X, sz %08X\n",it->pos.offset,it->pos.size);
        }
        it++;
    }

    // Dump entire sector
    int q=0;
    int j=0;
    for(q=0;q<1024;q++) 
    {
        printf( "%08X", g_rbuf[q]);
        if (q%8==7) {
            printf("  ");
            int letters[16];
            // Only 4 byte access?
            //memcpy(letters,g_rbuf[q-6],8*4);
            letters[0]=g_rbuf[q-7];
            letters[1]=g_rbuf[q-6];
            letters[2]=g_rbuf[q-5];
            letters[3]=g_rbuf[q-4];
            letters[4]=g_rbuf[q-3];
            letters[5]=g_rbuf[q-2];
            letters[6]=g_rbuf[q-1];
            letters[7]=g_rbuf[q];

            char *text=(char *)letters;
            for(j=0;j<8*4;j++) {
                if (printable(*text)) {
                     printf( "%c", *text);
                }
                text++;
            }            
            printf("\n");
        }
    }



/* WTF?
    esp_err_t err = spi_flash_mmap(ESP_PARTITION_TABLE_ADDR & 0xffff0000,
            SPI_FLASH_SEC_SIZE, SPI_FLASH_MMAP_DATA, (const void**) &ptr, &handle);
    if (err != ESP_OK) {
        return err;
    }
    // calculate partition address within mmap-ed region
    const esp_partition_info_t* it = (const esp_partition_info_t*)
            (ptr + (ESP_PARTITION_TABLE_ADDR & 0xffff) / sizeof(*ptr));
    const esp_partition_info_t* end = it + SPI_FLASH_SEC_SIZE / sizeof(*it);

*/


}


void mmapPartitionData() {
    void *ptr;
    spi_flash_mmap_handle_t handle;

    printf("mmap partition data\n");

    esp_err_t err = spi_flash_mmap(ESP_PARTITION_TABLE_ADDR & 0xffff0000,
            SPI_FLASH_SEC_SIZE, SPI_FLASH_MMAP_DATA, (const void**) &ptr, &handle);

    //esp_err_t err = spi_flash_mmap(ESP_PARTITION_TABLE_ADDR,
    //        SPI_FLASH_SEC_SIZE, SPI_FLASH_MMAP_DATA, (const void**) &ptr, &handle);
    if (err != ESP_OK) {
      printf("mmap fail\n");
      //return err;
    }

    printf("mmap success\n");

    //int *test=(int *)*ptr;
    //printf( "%08X\n",(int )test);
    unsigned int *iptr=(unsigned int *)ptr;
    // Dump entire sector
    int q=0;
    int j=0;
    for(q=0;q<1024;q++) 
    {
        g_rbuf[q]=*iptr++;
        printf( "%08X", g_rbuf[q]);
        if (q%8==7) {
            printf("  ");
            int letters[16];
            // Only 4 byte access?
            //memcpy(letters,g_rbuf[q-6],8*4);
            letters[0]=g_rbuf[q-7];
            letters[1]=g_rbuf[q-6];
            letters[2]=g_rbuf[q-5];
            letters[3]=g_rbuf[q-4];
            letters[4]=g_rbuf[q-3];
            letters[5]=g_rbuf[q-2];
            letters[6]=g_rbuf[q-1];
            letters[7]=g_rbuf[q];

            char *text=(char *)letters;
            for(j=0;j<8*4;j++) {
                if (printable(*text)) {
                     printf( "%c", *text);
                }
                text++;
            }            
            printf("\n");
        }
    }


}


void readWriteTask(void *pvParameters)
{
    //srand(0);



    //    for (uint32_t base_addr = 0x200000;
    //     base_addr < 0x300000;
    //     base_addr += SPI_FLASH_SEC_SIZE)
        for (uint32_t base_addr = 0x000000;
         base_addr < 0x100000;
         base_addr += SPI_FLASH_SEC_SIZE)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("erasing sector %x\n", base_addr / SPI_FLASH_SEC_SIZE);
        spi_flash_erase_sector(base_addr / SPI_FLASH_SEC_SIZE);

        for (int i = 0; i < sizeof(g_wbuf)/sizeof(g_wbuf[0]); ++i) {
            g_wbuf[i] = i%16; //rand();
        }

        printf("writing at %x\n", base_addr);
        spi_flash_write(base_addr, g_wbuf, sizeof(g_wbuf));

        memset(g_rbuf, 0, sizeof(g_rbuf));
        printf("reading at %x\n", base_addr);
        spi_flash_read(base_addr, g_rbuf, sizeof(g_rbuf));
        for (int i = 0; i < sizeof(g_rbuf)/sizeof(g_rbuf[0]); ++i) {
            if (g_rbuf[i] != g_wbuf[i]) {
                printf("failed writing or reading at 0x%08x\n", base_addr + i * 4);
                printf("got %08x, expected %08x\n", g_rbuf[i], g_wbuf[i]);
                //return;
            }
        }

        printf("done at %x\n", base_addr);
        base_addr += SPI_FLASH_SEC_SIZE;
    }

    printf("read/write/erase test done\n");

    // TODO flash map and check data.
    //spi_flash_mmap

    spi_flash_mmap_dump();



    vTaskDelete(NULL);
}


void app_main()
{
    // workaround: configure SPI flash size manually (2nd argument)
    //SPIParamCfg(0x1540ef, 4*1024*1024, 64*1024, 4096, 256, 0xffff);
    nvs_flash_init();

    // This is just a qemu trick to restore original rom content 
    //int *unpatch=(int *) 0x3ff00088;
    //*unpatch=0x42;

    spi_flash_mmap_dump();

    dumpPartitionData();

    mmapPartitionData();

    spi_flash_mmap_dump();

    //xTaskCreatePinnedToCore(&readWriteTask, "readWriteTask", 2048, NULL, 5, NULL, 0);
}
