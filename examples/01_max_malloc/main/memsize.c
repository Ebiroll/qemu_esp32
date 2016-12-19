#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"

#define MAX_ALLOCS 20

void app_main()
{
    void *ptr[MAX_ALLOCS];
    size_t idx = 0, size = 1024*1024, sum = 0;

    nvs_flash_init();
    
    printf("starting\n");
    uint32_t test=esp_get_free_heap_size();
    printf("Free heap size %d\n",test);


    while (idx < MAX_ALLOCS && size > 64) {
        ptr[idx] = malloc(size);
        if (ptr[idx] == NULL) {
            size /= 2;
            continue;
        }
        idx++;
        sum += size;
        printf("ALLOCS: %d/%d, SIZE %d/%d\n", idx, MAX_ALLOCS, size, sum);
    }
    test=esp_get_free_heap_size();
    printf("Free heap size %d\n",test);

    while (idx--) {
        printf("free idx %d\n", idx);
        free(ptr[idx]);
    }
    
    test=esp_get_free_heap_size();
    printf("Free heap size %d\n",test);
}
