/*
 * Copyright (c) 2011, Max Filippov, Open Source and Linux Lab.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Open Source and Linux Lab nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "elf.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "hw/char/serial.h"
#include "net/net.h"
#include "hw/sysbus.h"
#include "hw/block/flash.h"
#include "sysemu/block-backend.h"
#include "sysemu/char.h"
#include "sysemu/device_tree.h"
#include "qemu/error-report.h"
#include "bootparam.h"

typedef struct ESP32BoardDesc {
    hwaddr flash_base;
    size_t flash_size;
    size_t flash_boot_base;
    size_t flash_sector_size;
    size_t sram_size;
} ESP32BoardDesc;

typedef struct Lx60FpgaState {
    MemoryRegion iomem;
    uint32_t leds;
    uint32_t switches;
} Lx60FpgaState;

static void lx60_fpga_reset(void *opaque)
{
    Lx60FpgaState *s = opaque;

    s->leds = 0;
    s->switches = 0;
}

static uint64_t lx60_fpga_read(void *opaque, hwaddr addr,
        unsigned size)
{
    Lx60FpgaState *s = opaque;

    switch (addr) {
    case 0x0: /*build date code*/
        return 0x09272011;

    case 0x4: /*processor clock frequency, Hz*/
        return 10000000;

    case 0x8: /*LEDs (off = 0, on = 1)*/
        return s->leds;

    case 0xc: /*DIP switches (off = 0, on = 1)*/
        return s->switches;
    }
    return 0;
}

static void lx60_fpga_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{
    Lx60FpgaState *s = opaque;

    switch (addr) {
    case 0x8: /*LEDs (off = 0, on = 1)*/
        s->leds = val;
        break;

    case 0x10: /*board reset*/
        if (val == 0xdead) {
            qemu_system_reset_request();
        }
        break;
    }
}

static const MemoryRegionOps lx60_fpga_ops = {
    .read = lx60_fpga_read,
    .write = lx60_fpga_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static Lx60FpgaState *lx60_fpga_init(MemoryRegion *address_space,
        hwaddr base)
{
    Lx60FpgaState *s = g_malloc(sizeof(Lx60FpgaState));

    memory_region_init_io(&s->iomem, NULL, &lx60_fpga_ops, s,
            "lx60.fpga", 0x10000);
    memory_region_add_subregion(address_space, base, &s->iomem);
    lx60_fpga_reset(s);
    qemu_register_reset(lx60_fpga_reset, s);
    return s;
}

static void lx60_net_init(MemoryRegion *address_space,
        hwaddr base,
        hwaddr descriptors,
        hwaddr buffers,
        qemu_irq irq, NICInfo *nd)
{
    DeviceState *dev;
    SysBusDevice *s;
    MemoryRegion *ram;

    dev = qdev_create(NULL, "open_eth");
    qdev_set_nic_properties(dev, nd);
    qdev_init_nofail(dev);

    s = SYS_BUS_DEVICE(dev);
    sysbus_connect_irq(s, 0, irq);
    memory_region_add_subregion(address_space, base,
            sysbus_mmio_get_region(s, 0));
    memory_region_add_subregion(address_space, descriptors,
            sysbus_mmio_get_region(s, 1));

    ram = g_malloc(sizeof(*ram));
    memory_region_init_ram(ram, OBJECT(s), "open_eth.ram", 16384, &error_abort);
    vmstate_register_ram_global(ram);
    memory_region_add_subregion(address_space, buffers, ram);
}

static uint64_t translate_phys_addr(void *opaque, uint64_t addr)
{
    XtensaCPU *cpu = opaque;

    return cpu_get_phys_page_debug(CPU(cpu), addr);
}

static void lx60_reset(void *opaque)
{
    XtensaCPU *cpu = opaque;

    cpu_reset(CPU(cpu));
}

static uint64_t esp_io_read(void *opaque, hwaddr addr,
        unsigned size)
{
    return 0;
}

static void esp_io_write(void *opaque, hwaddr addr,
        uint64_t val, unsigned size)
{
    printf("io write %08x\n");
}

static const MemoryRegionOps esp_io_ops = {
    .read = esp_io_read,
    .write = esp_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};


// MMU not setup? Or maybe cpu_get_phys_page_debug returns wrong adress.
// We try this mapping instead to get us started
static uint64_t translate_esp32_address(void *opaque, uint64_t addr)
{
    return addr;
}

static void esp32_init(const ESP32BoardDesc *board, MachineState *machine)
{
    int be = 0;

    MemoryRegion *system_memory = get_system_memory();
    XtensaCPU *cpu = NULL;
    CPUXtensaState *env = NULL;
    MemoryRegion *ram, *rom, *system_io;
    DriveInfo *dinfo;
    pflash_t *flash = NULL;
    QemuOpts *machine_opts = qemu_get_machine_opts();
    const char *cpu_model = machine->cpu_model;
    const char *kernel_filename = qemu_opt_get(machine_opts, "kernel");
    const char *kernel_cmdline = qemu_opt_get(machine_opts, "append");
    const char *dtb_filename = qemu_opt_get(machine_opts, "dtb");
    const char *initrd_filename = qemu_opt_get(machine_opts, "initrd");
    int n;

    if (!cpu_model) {
        cpu_model = XTENSA_DEFAULT_CPU_MODEL;
    }

    for (n = 0; n < smp_cpus; n++) {
        cpu = cpu_xtensa_init(cpu_model);
        if (cpu == NULL) {
            error_report("unable to find CPU definition '%s'",
                         cpu_model);
            exit(EXIT_FAILURE);
        }
        env = &cpu->env;

        env->sregs[PRID] = n;
        qemu_register_reset(lx60_reset, cpu);
        /* Need MMU initialized prior to ELF loading,
         * so that ELF gets loaded into virtual addresses
         */
        cpu_reset(CPU(cpu));
    }

    // Internal rom, 0x4000_0000 ~ 0x4005_FFFF


    // Map all as ram 
    ram = g_malloc(sizeof(*ram));
    memory_region_init_ram(ram, NULL, "iram0", 0x20000000,
                           &error_abort);

    vmstate_register_ram_global(ram);
    memory_region_add_subregion(system_memory, 0x30000000, ram);

#if 0
    ram = g_malloc(sizeof(*ram));
    memory_region_init_ram(ram, NULL, "iram0", 0x20002000,  // 0x1a0000
                           &error_abort);

    vmstate_register_ram_global(ram);
    memory_region_add_subregion(system_memory, 0x30000000, ram);
#endif
    // iram0  -- 0x4008_0000, len = 0x20000


    // Sram1 can be remapped by 
    // setting bit 0 of register DPORT_PRO_BOOT_REMAP_CTRL_REG or
    // DPORT_APP_BOOT_REMAP_CTRL_REG


    // sram1 -- 0x400B_0000 ~ 0x400B_7FFF


// dram0 3ffc0000 

    system_io = g_malloc(sizeof(*system_io));
    memory_region_init_io(system_io, NULL, &esp_io_ops, NULL, "esp32.io",
                          0x80000);

    memory_region_add_subregion(system_memory, 0x3ff00000, system_io);
    lx60_fpga_init(system_io, 0x0d020000);


    if (!serial_hds[0]) {
        serial_hds[0] = qemu_chr_new("serial0", "null", NULL);
    }
                            //0x0d050020
    serial_mm_init(system_io, 0x3FF40020 , 2, xtensa_get_extint(env, 0),
            115200, serial_hds[0], DEVICE_NATIVE_ENDIAN);

    dinfo = drive_get(IF_PFLASH, 0, 0);
    if (dinfo) {
        flash = pflash_cfi01_register(board->flash_base,
                NULL, "esp32.io.flash", board->flash_size,
                blk_by_legacy_dinfo(dinfo),
                board->flash_sector_size,
                board->flash_size / board->flash_sector_size,
                4, 0x0000, 0x0000, 0x0000, 0x0000, be);
        if (flash == NULL) {
            error_report("unable to mount pflash");
            exit(EXIT_FAILURE);
        }
    }

    /* Use presence of kernel file name as 'boot from SRAM' switch. */
    if (kernel_filename) {
        uint32_t entry_point = env->pc;
        size_t bp_size = 3 * get_tag_size(0); /* first/last and memory tags */
        uint32_t tagptr = 0xfe000000 + board->sram_size;
        uint32_t cur_tagptr;
        BpMemInfo memory_location = {
            .type = tswap32(MEMORY_TYPE_CONVENTIONAL),
            .start = tswap32(0),
            .end = tswap32(machine->ram_size),
        };
        uint32_t lowmem_end = machine->ram_size < 0x08000000 ?
            machine->ram_size : 0x08000000;
        uint32_t cur_lowmem = QEMU_ALIGN_UP(lowmem_end / 2, 4096);

        rom = g_malloc(sizeof(*rom));
        memory_region_init_ram(rom, NULL, "lx60.sram", 0x20000,
                               &error_abort);
        vmstate_register_ram_global(rom);
        memory_region_add_subregion(system_memory, 0x40000000, rom);

        if (kernel_cmdline) {
            bp_size += get_tag_size(strlen(kernel_cmdline) + 1);
        }
        if (dtb_filename) {
            bp_size += get_tag_size(sizeof(uint32_t));
        }
        if (initrd_filename) {
            bp_size += get_tag_size(sizeof(BpMemInfo));
        }

        /* Put kernel bootparameters to the end of that SRAM */
        #if 0
        tagptr = (tagptr - bp_size) & ~0xff;
        cur_tagptr = put_tag(tagptr, BP_TAG_FIRST, 0, NULL);
        cur_tagptr = put_tag(cur_tagptr, BP_TAG_MEMORY,
                             sizeof(memory_location), &memory_location);

        if (kernel_cmdline) {
            cur_tagptr = put_tag(cur_tagptr, BP_TAG_COMMAND_LINE,
                                 strlen(kernel_cmdline) + 1, kernel_cmdline);
        }
        #endif

        if (dtb_filename) {
            int fdt_size;
            void *fdt = load_device_tree(dtb_filename, &fdt_size);
            uint32_t dtb_addr = tswap32(cur_lowmem);

            if (!fdt) {
                error_report("could not load DTB '%s'", dtb_filename);
                exit(EXIT_FAILURE);
            }

            cpu_physical_memory_write(cur_lowmem, fdt, fdt_size);
            cur_tagptr = put_tag(cur_tagptr, BP_TAG_FDT,
                                 sizeof(dtb_addr), &dtb_addr);
            cur_lowmem = QEMU_ALIGN_UP(cur_lowmem + fdt_size, 4096);
        }
        if (initrd_filename) {
            BpMemInfo initrd_location = { 0 };
            int initrd_size = load_ramdisk(initrd_filename, cur_lowmem,
                                           lowmem_end - cur_lowmem);

            if (initrd_size < 0) {
                initrd_size = load_image_targphys(initrd_filename,
                                                  cur_lowmem,
                                                  lowmem_end - cur_lowmem);
            }
            if (initrd_size < 0) {
                error_report("could not load initrd '%s'", initrd_filename);
                exit(EXIT_FAILURE);
            }
            initrd_location.start = tswap32(cur_lowmem);
            initrd_location.end = tswap32(cur_lowmem + initrd_size);
            cur_tagptr = put_tag(cur_tagptr, BP_TAG_INITRD,
                                 sizeof(initrd_location), &initrd_location);
            cur_lowmem = QEMU_ALIGN_UP(cur_lowmem + initrd_size, 4096);
        }
        // a2. Stack??
        cur_tagptr = put_tag(cur_tagptr, BP_TAG_LAST, 0, NULL);
        env->regs[2] = tagptr;

        uint64_t elf_entry;
        uint64_t elf_lowaddr;
        int success = load_elf(kernel_filename, translate_esp32_address, cpu,
                &elf_entry, &elf_lowaddr, NULL, be, EM_XTENSA, 0,0);
        if (success > 0) {
            entry_point = elf_entry;
        } else {
            hwaddr ep;
            int is_linux;
            success = load_uimage(kernel_filename, &ep, NULL, &is_linux,
                                  translate_esp32_address, cpu);
            if (success > 0 && is_linux) {
                entry_point = ep;
            } else {
                error_report("could not load kernel '%s'",
                             kernel_filename);
                exit(EXIT_FAILURE);
            }
        }
        if (entry_point != env->pc) {
            // elf entypoint differs, set up 
            printf("Set jx a0");
            static const uint8_t jx_a0[] = {
                0xa0, 0, 0,
            };
            env->regs[0] = entry_point;
            // Stack ? a1
            env->regs[1]=0x40081000;

            // ??? 
            //env->regs[193]=0x00000020;


            cpu_physical_memory_write(env->pc, jx_a0, sizeof(jx_a0));
        }
    } else {
        // No elf, try flash...
        if (flash) {
            MemoryRegion *flash_mr = pflash_cfi01_get_memory(flash);
            MemoryRegion *flash_io = g_malloc(sizeof(*flash_io));

            memory_region_init_alias(flash_io, NULL, "esp32.flash",
                    flash_mr, board->flash_boot_base,
                    board->flash_size - board->flash_boot_base < 0x02000000 ?
                    board->flash_size - board->flash_boot_base : 0x02000000);
            memory_region_add_subregion(system_memory, 0xfe000000,
                    flash_io);
        }
    }
}

static void xtensa_esp32_init(MachineState *machine)
{
    static const ESP32BoardDesc esp32_board = {
        .flash_base = 0xf0000000,
        .flash_size = 0x08000000,
        .flash_boot_base = 0x06000000,
        .flash_sector_size = 0x20000,
        .sram_size = 0x2000000,
    };
    esp32_init(&esp32_board, machine);
}

/*
*/

static void xtensa_esp32_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "esp32 DEV (esp32)";
    mc->init = xtensa_esp32_init;
    mc->max_cpus = 4;

}

static const TypeInfo xtensa_esp32_machine = {
    .name = MACHINE_TYPE_NAME("esp32"),
    .parent = TYPE_MACHINE,
    .class_init = xtensa_esp32_class_init,
};


static void xtensa_esp32_machines_init(void)
{
    type_register_static(&xtensa_esp32_machine);
}

type_init(xtensa_esp32_machines_init);
