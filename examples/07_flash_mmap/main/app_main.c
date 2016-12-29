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
#include <rom/spi_flash.h>
#include <rom/cache.h>


#define DPORT_PRO_FLASH_MMU_TABLE ((volatile uint32_t*) 0x3FF10000)

/* Flash MMU table for APP CPU */
#define DPORT_APP_FLASH_MMU_TABLE ((volatile uint32_t*) 0x3FF12000)


static uint32_t g_wbuf[SPI_FLASH_SEC_SIZE / 4];
static uint32_t g_rbuf[SPI_FLASH_SEC_SIZE / 4];

#define ESP_PARTITION_TABLE_ADDR 0x8000


int printable(char c)

{
  return ((c >= ' ' && c <= '~') ? 1 : 0);
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

#define REGIONS_COUNT 4
#define PAGES_PER_REGION 64

void mmu_table_dump()
{
    uint32_t test;
    int i;    
    for (i = 0; i < REGIONS_COUNT * PAGES_PER_REGION; ++i) {
        test=DPORT_PRO_FLASH_MMU_TABLE[i];
        if (test!=0) {
            printf("pro page %d: paddr=%08X\n",i, test);
        }
    }
    // DPORT_APP_FLASH_MMU_TABLE
    for (i = 0; i < REGIONS_COUNT * PAGES_PER_REGION; ++i) {
        test=DPORT_APP_FLASH_MMU_TABLE[i];
        if (test!=0) {
            printf("app page %d: paddr=%08X\n",i, test);
        }
    }


}



void mmapPartitionData() {
    void *ptr;
    spi_flash_mmap_handle_t handle;
    spi_flash_mmap_handle_t handle2;

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
    printf( "ptr @%p\n",ptr);

    const esp_partition_info_t* it = (const esp_partition_info_t*)
            (ptr + (ESP_PARTITION_TABLE_ADDR & 0xffff) / sizeof(*ptr));
    const esp_partition_info_t* end = it + SPI_FLASH_SEC_SIZE / sizeof(*it);

    unsigned int *iptr=(unsigned int *) it;

    printf( "iptr @%p\n",iptr);

    iptr=ptr;

    printf( "iptr @%p\n",iptr);

    //((ptr + ESP_PARTITION_TABLE_ADDR)/ sizeof(*ptr));
    // Dump entire sector
    int q=0;
    int j=0;
    for(q=0;q<1*1024;q++) 
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
                } else
                {
                    printf( ".");
                }
                text++;
            }            
            printf("\n");
        }
    }
    mmu_table_dump();

    // Map another 64K section way above tha app at 0x10000
    // App is 264K   
     esp_err_t high_map = spi_flash_mmap(0x70000 & 0xffff0000,
            SPI_FLASH_SEC_SIZE, SPI_FLASH_MMAP_DATA, (const void**) &ptr, &handle);

    if (high_map != ESP_OK) {
      printf("high mmap fail\n");
      return err;
    }

    printf("high mmap success\n");

    printf( "ptr @%p\n",ptr);

    iptr=(unsigned int *) ptr;

    printf( "iptr @%p\n",iptr);

    for(q=0;q<1*1024;q++) 
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
                } else {
                    printf( ".");
                }
                text++;
            }            
            printf("\n");
        }
    }

    mmu_table_dump();


    esp_err_t derr = spi_flash_mmap(ESP_PARTITION_TABLE_ADDR & 0xffff0000,
            SPI_FLASH_SEC_SIZE, SPI_FLASH_MMAP_DATA, (const void**) &ptr, &handle);

    if (derr != ESP_OK) {
      printf("double mmap fail\n");
      //return err;
    }

    printf("mmap success\n");

    printf( "ptr @%p\n",ptr);

    iptr=(unsigned int *) ptr;

    printf( "iptr @%p\n",iptr);

    for(q=0;q<1*1024;q++) 
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
                } else {
                    printf( ".");
                }
                text++;
            }            
            printf("\n");
        }
    }

    mmu_table_dump();



    iptr=(unsigned int *) ptr;

    printf( "iptr @%p\n",iptr);

    iptr=(unsigned int *)0x3f400000;

    while (iptr<48000000) {
        if (*iptr==0xbada) {
            printf( "found bada @%p",iptr);
            if (*(iptr+1)==0x55e5) {
                printf (" yes\n");
            }
        }
        iptr++;
    }

    printf ("Done\n");
}


void tagFlash()
{

     for (uint32_t base_addr = 0x000000;
         base_addr < 0x4000;
         base_addr += SPI_FLASH_SEC_SIZE)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        int i;

        printf("erasing sector %x\n", base_addr / SPI_FLASH_SEC_SIZE);
        spi_flash_erase_sector(base_addr / SPI_FLASH_SEC_SIZE);

        for (i = 0; i < 64; ++i) {
            g_wbuf[i] = 0; //rand();
        }

        int idx=0;
        char *string_data=(char *)&g_wbuf[8];
        string_data[idx++] = 'E'; //rand();
        string_data[idx++] = 'b'; //rand();
        string_data[idx++] = 'i'; //rand();
        string_data[idx++] = 'r'; //rand();
        string_data[idx++] = 'o'; //rand();
        string_data[idx++] = 'l'; //rand();
        string_data[idx++] = 'l'; //rand();
        string_data[idx++] = ' '; //rand();
        string_data[idx++] = 'w'; //rand();
        string_data[idx++] = 'a'; //rand();
        string_data[idx++] = 's'; //rand();
        string_data[idx++] = ' '; //rand();
        string_data[idx++] = 'h'; //rand();
        string_data[idx++] = 'e'; //rand();
        string_data[idx++] = 'r'; //rand();
        string_data[idx++] = 'e'; //rand();
        string_data[idx++] = 0; //rand();


        for (i = 64; i < sizeof(g_wbuf)/sizeof(g_wbuf[0]); ++i) {
            g_wbuf[i] = base_addr; //rand();
        }

        g_wbuf[0]=0xbada;
        g_wbuf[1]=0x55e5;

        printf("writing at %x\n", base_addr);
        spi_flash_write(base_addr, g_wbuf, sizeof(g_wbuf));

        base_addr += SPI_FLASH_SEC_SIZE;
    }

    printf("tag flash done\n");


}




void app_main()
{
    // workaround: configure SPI flash size manually (2nd argument)
    //SPIParamCfg(0x1540ef, 4*1024*1024, 64*1024, 4096, 256, 0xffff);

    // This is just a qemu trick to restore original rom content 
    //int *unpatch=(int *) 0x3ff005F0;
    //*unpatch=0x42;
    mmu_table_dump();
    nvs_flash_init();

    tagFlash();

    spi_flash_mmap_dump();
    mmu_table_dump();


    mmapPartitionData();

    spi_flash_mmap_dump();
    mmu_table_dump();

}
