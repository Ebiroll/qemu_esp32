/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PLATFORM_H
#define __PLATFORM_H

#define NO_USB_PLEASE

#define SET_RUN_STATE(state)
#define SET_IDLE_STATE(state)
#define SET_ERROR_STATE(state)
#define DEBUG(x, ...) do { ; } while (0)

//#include "timing.h"
//#define LWIP_OPEN_SRC
//#include <espressif/esp_common.h>
//#include <esp32.h>

#define TMS_SET_MODE() do { } while (0)

// no-connects on ESP-01: 12,13,14,15
#define TMS_PIN (2) // no-connects on ESP-01
#define TDI_PIN (13) // "
#define TDO_PIN (14) // "
#define TCK_PIN (3) // "
// 2 is GPIO2, broken out
// 3 is RXD

#define SWDIO_PIN TMS_PIN
#define SWCLK_PIN TCK_PIN

#define gpio_set_val(port, pin, value) do {	\
		gpio_write(pin, value);		\
		/*sdk_os_delay_us(2);	*/	\
	} while (0);
#define gpio_set(port, pin) gpio_set_val(port, pin, 1)
#define gpio_clear(port, pin) gpio_set_val(port, pin, 0)
#define gpio_get(port, pin) gpio_read(pin)

#define SWDIO_MODE_FLOAT() do {			\
		gpio_enable(SWDIO_PIN, GPIO_INPUT);	\
	} while (0)

#define SWDIO_MODE_DRIVE() do {				\
		gpio_enable(SWDIO_PIN, GPIO_OUTPUT);	\
	} while (0)

#define PLATFORM_HAS_DEBUG // do we?

#endif
