#include "./card.h"

#include "esp_log.h"
#include "sdmmc_cmd.h"
#include <esp_system.h>
#include "esp_vfs_fat.h"
#include <stdio.h>
#include <dirent.h>

static sdmmc_card_t *card;
static const char * TAG = "Sd Card";

bool initCardReaderSdMmc() {
    ESP_LOGI(TAG, "Setup GPIO pull-up for SD/MMC");

    // DAT0
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);
    // DAT1
    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);
    // DAT2
    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);
    // DAT3
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);
    // CLK
    gpio_set_pull_mode(GPIO_NUM_14, GPIO_PULLUP_ONLY);
    // CMD
    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.slot = SDMMC_HOST_SLOT_1;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    esp_err_t err = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount the filesystem - might be able to format");
        } else {
            ESP_LOGE(TAG, "FAILED to initialize the card (%d). Make sure SD hardware is configured properly", err);
        }
        return false;
    }

    return true;
}

void cardInfo() {
    sdmmc_card_print_info(stdout, card);
}

void cardPrintFiles() {
    DIR * dr = opendir("/sdcard");

    if (dr == NULL) {
        ESP_LOGE(TAG, "Could not open directory /sdcard");
    } else {
        struct dirent * de;

        while ((de = readdir(dr)) != NULL) {
            ESP_LOGI(TAG, "file %s", de->d_name);
        }

        closedir(dr);
    }
}