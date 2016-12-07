// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file rtc.h
 * @brief Declarations of APIs provided by librtc.a
 *
 * This file is not in the include directory of esp32 component, so it is not
 * part of the public API. As the source code of librtc.a is gradually moved
 * into the ESP-IDF, some of these APIs will be exposed to applications.
 *
 * For now, only esp_deep_sleep function declared in esp_deepsleep.h
 * is part of public API.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include "soc/soc.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum{
    XTAL_40M = 40,
    XTAL_26M = 26,
    XTAL_24M = 24,
    XTAL_AUTO = 0
} xtal_freq_t;

typedef enum{
    CPU_XTAL = 0,
    CPU_80M = 1,
    CPU_160M = 2,
    CPU_240M = 3,
    CPU_2M = 4
} cpu_freq_t;

typedef enum {
    CALI_RTC_MUX = 0,
    CALI_8MD256 = 1,
    CALI_32K_XTAL = 2
} cali_clk_t;

/**
 * This function must be called to initialize RTC library
 * @param xtal_freq  Frequency of main crystal
 */
void rtc_init_lite(xtal_freq_t xtal_freq);

/**
 * Switch CPU frequency
 * @param cpu_freq  new CPU frequency
 */
void rtc_set_cpu_freq(cpu_freq_t cpu_freq);

/**
 * @brief Return RTC slow clock's period
 * @param cali_clk  clock to calibrate
 * @param slow_clk_cycles  number of slow clock cycles to average
 * @param xtal_freq  chip's main XTAL freq
 * @return average slow clock period in microseconds, Q13.19 fixed point format
 */
uint32_t rtc_slowck_cali(cali_clk_t cali_clk, uint32_t slow_clk_cycles);

/**
 * @brief Convert from microseconds to slow clock cycles
 * @param time_in_us_h Time in microseconds, higher 32 bit part
 * @param time_in_us_l Time in microseconds, lower 32 bit part
 * @param slow_clk_period  Period of slow clock in microseconds, Q13.19 fixed point format (as returned by rtc_slowck_cali).
 * @param[out] cylces_h  output, higher 32 bit part of number of slow clock cycles
 * @param[out] cycles_l  output, lower 32 bit part of number of slow clock cycles
 */
void rtc_usec2rtc(uint32_t time_in_us_h, uint32_t time_in_us_l, uint32_t slow_clk_period, uint32_t *cylces_h, uint32_t *cycles_l);


#define DEEP_SLEEP_PD_NORMAL         BIT(0)   /*!< Base deep sleep mode */
#define DEEP_SLEEP_PD_RTC_PERIPH     BIT(1)   /*!< Power down RTC peripherals */
#define DEEP_SLEEP_PD_RTC_SLOW_MEM   BIT(2)   /*!< Power down RTC SLOW memory */
#define DEEP_SLEEP_PD_RTC_FAST_MEM   BIT(3)   /*!< Power down RTC FAST memory */

/**
 * @brief Prepare for entering sleep mode
 * @param deep_slp   DEEP_SLEEP_PD_ flags combined with OR (DEEP_SLEEP_PD_NORMAL must be included)
 * @param cpu_lp_mode  for deep sleep, should be 0
 */
void rtc_slp_prep_lite(uint32_t deep_slp, uint32_t cpu_lp_mode);


#define RTC_EXT_EVENT0_TRIG     BIT(0)
#define RTC_EXT_EVENT1_TRIG     BIT(1)
#define RTC_GPIO_TRIG           BIT(2)
#define RTC_TIMER_EXPIRE        BIT(3)
#define RTC_SDIO_TRIG           BIT(4)
#define RTC_MAC_TRIG            BIT(5)
#define RTC_UART0_TRIG          BIT(6)
#define RTC_UART1_TRIG          BIT(7)
#define RTC_TOUCH_TRIG          BIT(8)
#define RTC_SAR_TRIG            BIT(9)
#define RTC_BT_TRIG             BIT(10)


#define RTC_EXT_EVENT0_TRIG_EN  RTC_EXT_EVENT0_TRIG
#define RTC_EXT_EVENT1_TRIG_EN  RTC_EXT_EVENT1_TRIG
#define RTC_GPIO_TRIG_EN        RTC_GPIO_TRIG
#define RTC_TIMER_EXPIRE_EN     RTC_TIMER_EXPIRE
#define RTC_SDIO_TRIG_EN        RTC_SDIO_TRIG
#define RTC_MAC_TRIG_EN         RTC_MAC_TRIG
#define RTC_UART0_TRIG_EN       RTC_UART0_TRIG
#define RTC_UART1_TRIG_EN       RTC_UART1_TRIG
#define RTC_TOUCH_TRIG_EN       RTC_TOUCH_TRIG
#define RTC_SAR_TRIG_EN         RTC_SAR_TRIG
#define RTC_BT_TRIG_EN          RTC_BT_TRIG

/**
 * @brief Enter sleep mode for given number of cycles
 * @param cycles_h  higher 32 bit part of number of slow clock cycles
 * @param cycles_l  lower 32 bit part of number of slow clock cycles
 * @param wakeup_opt  wake up reason to enable (RTC_xxx_EN flags combined with OR)
 * @param reject_opt  reserved, should be 0
 * @return TBD
 */
uint32_t rtc_sleep(uint32_t cycles_h, uint32_t cycles_l, uint32_t wakeup_opt, uint32_t reject_opt);

typedef enum {
	RTC_GPIO0_SEL = BIT(0),
	RTC_GPIO1_SEL = BIT(1),
	RTC_GPIO2_SEL = BIT(2),
	RTC_GPIO3_SEL = BIT(3),
	RTC_GPIO4_SEL = BIT(4),
	RTC_GPIO5_SEL = BIT(5),
	RTC_GPIO6_SEL = BIT(6),
	RTC_GPIO7_SEL = BIT(7),
	RTC_GPIO8_SEL = BIT(8),
	RTC_GPIO9_SEL = BIT(9),
	RTC_GPIO10_SEL = BIT(10),
	RTC_GPIO11_SEL = BIT(11),
	RTC_GPIO12_SEL = BIT(12),
	RTC_GPIO13_SEL = BIT(13),
	RTC_GPIO14_SEL = BIT(14),
	RTC_GPIO15_SEL = BIT(15),
	RTC_GPIO16_SEL = BIT(16),
	RTC_GPIO17_SEL = BIT(17)
} rtc_gpio_sel_t;

void rtc_pads_muxsel(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
void rtc_pads_funsel(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
void rtc_pads_slpsel(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
 
void rtc_pads_hold(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
void rtc_pads_pu(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
void rtc_pads_pd(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
void rtc_pads_slpie(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
void rtc_pads_slpoe(rtc_gpio_sel_t rtc_io_sel, uint8_t set);
void rtc_pads_funie(rtc_gpio_sel_t rtc_io_sel, uint8_t set);

uint32_t rtc_get_xtal();

/*
 * Using the EXT_WAKEUP0 slot. So you need to enable the RTC_EXT_EVENT0_TRIG
 * in rtc_sleep()
 *
 * rtc_io_num is the number of rtc_pad. e.g. The number of RTC_GPIO5 is 5
 * wakeup_level = 0: external wakeup at low level
 * wakeup_level = 1: external wakeup at high level
 */
void rtc_pad_ext_wakeup(rtc_gpio_sel_t rtc_io_sel, uint8_t rtc_io_num, uint8_t wakeup_level);

/*
 * Using the EXT_WAKEUP0 slot. So you need to enable the RTC_EXT_EVENT0_TRIG
 * in rtc_sleep()
 *
 * External wakeup at high level
 *
 * rtc_io_num is the number of rtc_pad. e.g. The number of RTC_GPIO5 is 5
 *
 */
void rtc_cmd_ext_wakeup(rtc_gpio_sel_t rtc_io_sel, uint8_t rtc_io_num);

/*
 * Only available in light sleep mode
 *
 * int_type:
 *  0: GPIO interrupt disable
 *  1: rising edge
 *  2: falling edge trigger
 *  3: any edge trigger
 *  4: low level trigger
 *  5: high level trigger
 *
 */
void rtc_pad_gpio_wakeup(rtc_gpio_sel_t rtc_io_sel, uint8_t int_type);

#ifdef __cplusplus
}
#endif

