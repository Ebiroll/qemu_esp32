#include "./lights.h"

#include "esp_log.h"
#include "../lib/ws2812.c"

static rgbVal * pixels;
static const char * TAG = "Lights!";

#define WS2812_PIN 22
#define PIXEL_COUNT 22

void lights_init() {
    ws2812_init(WS2812_PIN);

    auto color = makeRGBVal(10, 5, 0);
    auto dark = makeRGBVal(0, 0, 0);

    pixels = (rgbVal *)malloc(sizeof(rgbVal) * PIXEL_COUNT);
    for (auto k = 0; k < PIXEL_COUNT; k++) {
        ESP_LOGI(TAG, "Set lights for pixel %d", k);
        pixels[k] = (k % 2 == 0) ? color : dark;
        ESP_LOGI(TAG, "light %d set up now", k);
    }
}

void lights_flush() {
    ESP_LOGI(TAG, "Show Lights...");
    ws2812_setColors(PIXEL_COUNT, pixels);
}