#ifndef __ESP32_ANALOG
#define __ESP32_ANALOG

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SAMPLING_DONE_BIT   1

#define NUM_SAMPLES 2048

uint8_t* get_values();


// Param is task handle of task to notify
void sample_thread(void *param);

void start_sampling();

bool samples_finnished();


extern TaskHandle_t xHandlingTask;

#endif