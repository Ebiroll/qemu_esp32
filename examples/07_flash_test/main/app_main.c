#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
#include "rom/spi_flash.h"
#include <string.h>
#include <stdlib.h>


static uint32_t g_wbuf[SPI_FLASH_SEC_SIZE / 4];
static uint32_t g_rbuf[SPI_FLASH_SEC_SIZE / 4];

void readWriteTask(void *pvParameters)
{
    //srand(0);

    // This is just a qemu trick to restore original rom content 
    int *unpatch=(int *) 0x3ff00088;
    *unpatch=0x42;


    for (uint32_t base_addr = 0x200000;
         base_addr < 0x300000;
         base_addr += SPI_FLASH_SEC_SIZE)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("erasing sector %x\n", base_addr / SPI_FLASH_SEC_SIZE);
        spi_flash_erase_sector(base_addr / SPI_FLASH_SEC_SIZE);

        for (int i = 0; i < sizeof(g_wbuf)/sizeof(g_wbuf[0]); ++i) {
            g_wbuf[i] = i; //rand();
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


    vTaskDelete(NULL);
}


void app_main()
{
    // workaround: configure SPI flash size manually (2nd argument)
    SPIParamCfg(0x1540ef, 4*1024*1024, 64*1024, 4096, 256, 0xffff);

    xTaskCreatePinnedToCore(&readWriteTask, "readWriteTask", 2048, NULL, 5, NULL, 0);
}
