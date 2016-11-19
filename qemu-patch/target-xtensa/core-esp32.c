#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/gdbstub.h"
#include "qemu-common.h"
#include "qemu/host-utils.h"

#include "core-esp32/core-isa.h"
#include "overlay_tool.h"


static XtensaConfig esp32 __attribute__((unused)) = {
        .name = "esp32",
        .gdb_regmap = {
        .num_regs = 104,  // 172
        .num_core_regs = 52,   /* PC + a + ar + sr + ur */
        .reg = {
         #include "core-esp32/gdb-config.c"
        }
    },
    .clock_freq_khz = 240000,  // 160 Mhz
    DEFAULT_SECTIONS
};
REGISTER_CORE(esp32)
