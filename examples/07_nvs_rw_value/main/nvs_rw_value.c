/* Non-Volatile Storage (NVS) Read and Write a Value - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

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
#include "nvs.h"
#include "rom/spi_flash.h"


void app_main()
{
    uint32_t before_buf[10];
    uint32_t after_buf[10];
    nvs_flash_init();

#if 0
    // for debugging of qemu,  esp_rom_spiflash functions
    esp_rom_spiflash_result_t res;

    res = esp_rom_spiflash_read(0x100, before_buf, 8);

    for (int j=0;j<8;j++) {
        before_buf[j]=j;
    }
    before_buf[0]=0xaaaaaaaa;
    before_buf[1]=0xbbbbbbbb;
    before_buf[7]=0x88888888;

    //spi_flash_guard_start();
    res = esp_rom_spiflash_write(0x100, before_buf, 8);
    //spi_flash_guard_end();
#endif

    nvs_handle my_handle;
    esp_err_t err;

    printf("\n");

    // Open
    printf("Opening Non-Volatile Storage (NVS) ... ");
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%d) opening NVS!\n", err);
    } else {
        printf("Done\n");

        // Read
        printf("Reading restart counter from NVS ... ");
        int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_i32(my_handle, "restart_conter", &restart_counter);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Restart counter = %d\n", restart_counter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%d) reading!\n", err);
        }

        // Write
        printf("Updating restart counter in NVS ... ");
        restart_counter++;
        //restart_counter=0xaabbccdd;
        err = nvs_set_i32(my_handle, "restart_conter", restart_counter);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }

    printf("\n");

    // Restart module
    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
