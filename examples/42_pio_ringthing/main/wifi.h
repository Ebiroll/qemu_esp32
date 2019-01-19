#include "cstring"
#include <functional>
#include <memory>

#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


using onConnectedCallback = void();

void connect_wifi(std::function<onConnectedCallback> onConnected);