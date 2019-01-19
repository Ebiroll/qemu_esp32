#include "./lights.h"

#include "esp_log.h"
//#include "../lib/ws2812.c"
extern "C" {
#include "ws2812.h"
}
  
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

int pos=0;

void lights_forward() {

    auto red = makeRGBVal(pos*2, 0, 0);
    auto green = makeRGBVal(0, pos*2, 0);
    auto blue = makeRGBVal(0, 0, pos*2);
   
    auto dark = makeRGBVal(0, 0, 0);


    int j=0;

    for (j = 0; j < pos; j++) {
        if (j % 3 == 0) pixels[j] = red;  
        if (j % 3 == 1) pixels[j] = green;  
        if (j % 3 == 2) pixels[j] = blue;  
    }

    for (auto k = pos; k < PIXEL_COUNT; k++) {
         pixels[k] = dark;  
    }


    pos++;
    if (pos>=PIXEL_COUNT) {
        pos=0;
    }
}



void lights_flush() {
    ESP_LOGI(TAG, "Show Lights...");
    ws2812_setColors(PIXEL_COUNT, pixels);
}
