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

/* This file implements TI/LMI LM3S target specific functions providing
 * the XML memory map and Flash memory programming.
 *
 * According to: TivaTM TM4C123GH6PM Microcontroller Datasheet
 */

#include "general.h"
#include "target.h"
#include "target_internal.h"

//MEMORY {
//  iram : org = 0x40090000, len = 0x10000
//  dram : org = 0x3FFC0000, len = 0x20000
//}

#define IRAM_BASE            0x40090000
#define IRAM_SIZE            0x10000

#define DRAM_BASE            0x3FFC0000
#define DRAM_SIZE            0x20000


#define BLOCK_SIZE           0x1000



static int esp32_flash_erase(struct target_flash *f, target_addr addr, size_t len);
static int esp32_flash_write(struct target_flash *f,
                           target_addr dest, const void *src, size_t len);

static const char esp32_driver_str[] = "OTA esp32";

//static const uint16_t esp32_flash_write_stub[] = {
//#include "flashstub/lmi.stub"
//};

static void esp32_add_flash(target *t, size_t length)
{
	struct target_flash *f = calloc(1, sizeof(*f));
	f->start = 0;
	f->length = length;
	f->blocksize = BLOCK_SIZE;
	f->erase = esp32_flash_erase;
	f->write = esp32_flash_write;
	f->align = 4;
	f->erased = 0xff;
	target_add_flash(t, f);
}

bool esp32_otaflash_probe(target *t)
{
	t->driver = esp32_driver_str;
	target_add_ram(t, IRAM_BASE, 0x8000);
	esp32_add_flash(t, 0x400000);

	return true;
}

int esp32_flash_erase(struct target_flash *f, target_addr addr, size_t len)
{
	target  *t = f->t;

	printf("esp32_flash_erase\n");

	target_check_error(t);

	while(len) {

		if (target_check_error(t))
			return -1;

		len -= BLOCK_SIZE;
		addr += BLOCK_SIZE;
	}
	return 0;
}

int esp32_flash_write(struct target_flash *f,
                    target_addr dest, const void *src, size_t len)
{
	target  *t = f->t;

	target_check_error(t);

	printf("esp32_flash_write\n");


/* start stub
	target_mem_write(t, SRAM_BASE, esp32_flash_write_stub,
	                 sizeof(esp32_flash_write_stub));
	target_mem_write(t, STUB_BUFFER_BASE, src, len);

	if (target_check_error(t))
		return -1;

	return cortexm_run_stub(t, SRAM_BASE, dest, STUB_BUFFER_BASE, len, 0);
*/
 return 0;
}
