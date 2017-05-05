/* 
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "rom/tbconsole.h"

static const char *TAG = "basic";

void IRAM_ATTR start_cpu0(void)
{
      start_tb_console();
}



void app_main(void)
{

    // Workaround for qemu, esp-tool detects this and writes it automatically
    //g_rom_flashchip.chip_size=4*1024*1024; //0x3E8000;   // 4M?

    ESP_LOGI(TAG, "Starting basic");

    start_tb_console();

    ESP_LOGI(TAG, "Done");
}
