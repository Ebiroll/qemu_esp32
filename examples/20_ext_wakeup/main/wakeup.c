/*
 * MaiKe Labs (2016 - 2026)
 *
 * Written by Jack Tan <jiankemeng@gmail.com>
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include "xtensa/specreg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include <sys/lock.h>
#include "rom/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/dport_reg.h"
#include "esp_attr.h"
#include "esp_deepsleep.h"
#include "librtc.h"

#define BLINK_GPIO 27

void deep_sleep_ext_wakeup(uint8_t rtc_gpio_num)
{
	if (esp_get_deep_sleep_wake_stub() == NULL) {
		esp_set_deep_sleep_wake_stub(esp_wake_deep_sleep);
	}

	rtc_pad_ext_wakeup(1 << rtc_gpio_num, rtc_gpio_num, 1);

	/* fixed bug of rtc_pad_slpie() for RTC_GPIO4 */
	//REG_SET_BIT(RTC_IO_ADC_PAD_REG, RTC_IO_ADC1_SLP_IE);

	/* fixed bug of rtc_pad_slpie() for RTC_GPIO5 */
	REG_SET_BIT(RTC_IO_ADC_PAD_REG, RTC_IO_ADC2_SLP_IE);

	rtc_slp_prep_lite(DEEP_SLEEP_PD_NORMAL, 0);

	rtc_sleep(0, 0, RTC_EXT_EVENT0_TRIG, 0);

	while (1) {
		;
	}
}

void study_task(void *pvParameters)
{
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

	while (1) {
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_RATE_MS);

		/* pull up the RTC_GPIO5 (GPIO35) to wakeup */
		deep_sleep_ext_wakeup(5);
	}
}

void app_main()
{
	printf("Welcome to Noduino Quantum\r\n");
	printf("Try to investigate the ULP/RTC of ESP32 ... \r\n");
	xTaskCreatePinnedToCore(&study_task, "study_task", 1024, NULL, 5,
				NULL, 0);
}

/* use the rtc_gpio5 (pull up) to wakeup */
void ext_wakeup_rtc_io5_setup()
{
	rtc_pads_muxsel(1<<5, 1);
	rtc_pads_funsel(1<<5, 0);
	rtc_pads_slpsel(1<<5, 1);

	REG_SET_BIT(RTC_IO_ADC_PAD_REG, RTC_IO_ADC2_SLP_IE);

#if USE_EXT_WAKEUP0
	REG_SET_BIT(RTC_CNTL_EXT_WAKEUP_CONF_REG, RTC_CNTL_EXT_WAKEUP0_LV);
	REG_WRITE(RTC_IO_EXT_WAKEUP0_REG, 5<<RTC_IO_EXT_WAKEUP0_SEL_S);
#else
	REG_SET_BIT(RTC_CNTL_EXT_WAKEUP_CONF_REG, RTC_CNTL_EXT_WAKEUP1_LV);
	REG_SET_BIT(RTC_CNTL_EXT_WAKEUP1_REG, 1<<5);
#endif
}
