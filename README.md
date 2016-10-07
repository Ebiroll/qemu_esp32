# qemu_esp32
Add tensilica esp32 cpu and a board to qemu and dump the rom to learn more about esp-idf
###ESP32 in QEMU.

This documents how the to add an esp32 cpu and a simple esp32 board 
in order to run an app compiled with the SDK in QEMU. esp32 is a 240 MHz dual core Tensilica LX6 microcontroller 


By following the instructions here, I added esp32 to qemu.
http://wiki.linux-xtensa.org/index.php/Xtensa_on_QEMU

git clone git://git.qemu.org/qemu.git
copy qemu-esp32.tar.gz to the qemu source tree and unpack (tar zxvf)

Manually add in:
hw/extensa/Makefile.objs
  obj-y += esp32.o

target-xtensa/Makefile.objs
  obj-y += core-esp32.o

mkdir ../qemu_esp32
cd ../qemu_esp32
Then run configure as,
../qemu/configure --disable-werror --prefix=`pwd`/root --target-list=xtensa-softmmu,xtensaeb-softmmu
I also did this in qemu: git submodule update --init dtc

The goal is to run the app-template or bootloader and connect the debugger.


For now, I get get double exception error.

When running with -d int, I get
xtensa_cpu_do_interrupt(11) pc = 40080828, a0 = 40080828, ps = 0000001f, ccount = 00000002



  > qemu-xtensa-softmmu/qemu-system-xtensa -d mmu  -cpu esp32 -M esp32 -m 128M  -kernel  ../esp/myapp/build/app-template.elf  -s -S


  > xtensa-esp32-elf-gdb build/app-template.elf
  (gdb) target remote 127.0.0.1:1234

  (gdb) x/10i $pc

  (gdb) x/10i 0x40000400

To disassemble you can set pc from gdb,

  (gdb) set $pc=0x40080804

  (gdb) set $pc=<tab>
  (gdb) layout asm
  (gdb) si
  (gdb) ni

  (gdb) set $pc=uart_tx_one_char 


  x/10b $pc


#More useful commands
====================
Dump mixed source/disassemply listing,
xtensa-esp32-elf-objdump -d -S build/bootloader/bootloader.elf


#Boot of ESP32
======================

When starting up the ESP32 it reads instructions from embedded memory, located at 40000400
(unless this is mapped to ram by DPORT_APP_CACHE_CTRL1_REG & DPORT_PRO_CACHE_CTRL1_REG)  

  PROVIDE ( _ResetVector = 0x40000400 );
  #define XCHAL_RESET_VECTOR1_VADDR	0x40000400


Eventually aftern initialisation the first stage bootloader will call call_start_cpu0()

The main tasks of this is:

1. cpu_configure_region_protection(), sets up the mmu

```
 /* We arrive here after the bootloader finished loading the program from flash. The hardware is mostly uninitialized,
 * and the app CPU is in reset. We do have a stack, so we can do the initialization in C.
 */

  #define IRAM_ATTR __attribute__((section(".iram1")))

void IRAM_ATTR call_start_cpu0()
{
    cpu_configure_region_protection();

    //Clear bss
    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));

    /* completely reset MMU for both CPUs
       (in case serial bootloader was running) */
    Cache_Read_Disable(0);
    Cache_Read_Disable(1);
    Cache_Flush(0);
    Cache_Flush(1);
    mmu_init(0);
    REG_SET_BIT(DPORT_APP_CACHE_CTRL1_REG, DPORT_APP_CACHE_MMU_IA_CLR);
    mmu_init(1);
    REG_CLR_BIT(DPORT_APP_CACHE_CTRL1_REG, DPORT_APP_CACHE_MMU_IA_CLR);
    /* (above steps probably unnecessary for most serial bootloader
       usage, all that's absolutely needed is that we unmask DROM0
       cache on the following two lines - normal ROM boot exits with
       DROM0 cache unmasked, but serial bootloader exits with it
       masked. However can't hurt to be thorough and reset
       everything.)

       The lines which manipulate DPORT_APP_CACHE_MMU_IA_CLR bit are
       necessary to work around a hardware bug.
    */
    REG_CLR_BIT(DPORT_PRO_CACHE_CTRL1_REG, DPORT_PRO_CACHE_MASK_DROM0);
    REG_CLR_BIT(DPORT_APP_CACHE_CTRL1_REG, DPORT_APP_CACHE_MASK_DROM0);

    bootloader_main();
}
```
Executions continues here

```
void bootloader_main()
{
    ESP_LOGI(TAG, "Espressif ESP32 2nd stage bootloader v. %s", BOOT_VERSION);

    struct flash_hdr    fhdr;
    bootloader_state_t bs;
    SpiFlashOpResult spiRet1,spiRet2;    
    ota_select sa,sb;
    memset(&bs, 0, sizeof(bs));

    ESP_LOGI(TAG, "compile time " __TIME__ );
    /* close watch dog here */
    REG_CLR_BIT( RTC_CNTL_WDTCONFIG0_REG, RTC_CNTL_WDT_FLASHBOOT_MOD_EN );
    REG_CLR_BIT( TIMG_WDTCONFIG0_REG(0), TIMG_WDT_FLASHBOOT_MOD_EN );
    SPIUnlock();

    /*register first sector in drom0 page 0 */
    boot_cache_redirect( 0, 0x5000 );

    memcpy((unsigned int *) &fhdr, MEM_CACHE(0x1000), sizeof(struct flash_hdr) );

    print_flash_info(&fhdr);

    if (!load_partition_table(&bs, PARTITION_ADD)) {
        ESP_LOGE(TAG, "load partition table error!");
        return;
    }

    partition_pos_t load_part_pos;

    if (bs.ota_info.offset != 0) {     // check if partition table has OTA info partition
        //ESP_LOGE("OTA info sector handling is not implemented");
        boot_cache_redirect(bs.ota_info.offset, bs.ota_info.size );
        memcpy(&sa,MEM_CACHE(bs.ota_info.offset & 0x0000ffff),sizeof(sa));
        memcpy(&sb,MEM_CACHE((bs.ota_info.offset + 0x1000)&0x0000ffff) ,sizeof(sb));
        if(sa.ota_seq == 0xFFFFFFFF && sb.ota_seq == 0xFFFFFFFF) {
            // init status flash
            load_part_pos = bs.ota[0];
            sa.ota_seq = 0x01;
            sa.crc = ota_select_crc(&sa);
            sb.ota_seq = 0x00;
            sb.crc = ota_select_crc(&sb);

            Cache_Read_Disable(0);  
            spiRet1 = SPIEraseSector(bs.ota_info.offset/0x1000);       
            spiRet2 = SPIEraseSector(bs.ota_info.offset/0x1000+1);       
            if (spiRet1 != SPI_FLASH_RESULT_OK || spiRet2 != SPI_FLASH_RESULT_OK ) {  
                ESP_LOGE(TAG, SPI_ERROR_LOG);
                return;
            } 
            spiRet1 = SPIWrite(bs.ota_info.offset,(uint32_t *)&sa,sizeof(ota_select));
            spiRet2 = SPIWrite(bs.ota_info.offset + 0x1000,(uint32_t *)&sb,sizeof(ota_select));
            if (spiRet1 != SPI_FLASH_RESULT_OK || spiRet2 != SPI_FLASH_RESULT_OK ) {  
                ESP_LOGE(TAG, SPI_ERROR_LOG);
                return;
            } 
            Cache_Read_Enable(0);  
            //TODO:write data in ota info
        } else  {
            if(ota_select_valid(&sa) && ota_select_valid(&sb)) {
                load_part_pos = bs.ota[(((sa.ota_seq > sb.ota_seq)?sa.ota_seq:sb.ota_seq) - 1)%bs.app_count];
            }else if(ota_select_valid(&sa)) {
                load_part_pos = bs.ota[(sa.ota_seq - 1) % bs.app_count];
            }else if(ota_select_valid(&sb)) {
                load_part_pos = bs.ota[(sb.ota_seq - 1) % bs.app_count];
            }else {
                ESP_LOGE(TAG, "ota data partition info error");
                return;
            }
        }
    } else if (bs.factory.offset != 0) {        // otherwise, look for factory app partition
        load_part_pos = bs.factory;
    } else if (bs.test.offset != 0) {           // otherwise, look for test app parition
        load_part_pos = bs.test;
    } else {                                    // nothing to load, bail out
        ESP_LOGE(TAG, "nothing to load");
        return;
    }

    ESP_LOGI(TAG, "Loading app partition at offset %08x", load_part_pos);
    if(fhdr.secury_boot_flag == 0x01) {
        /* protect the 2nd_boot  */    
        if(false == secure_boot()){
            ESP_LOGE(TAG, "secure boot failed");
            return;
        }
    }

    if(fhdr.encrypt_flag == 0x01) {
        /* encrypt flash */            
        if (false == flash_encrypt(&bs)) {
           ESP_LOGE(TAG, "flash encrypt failed");
           return;
        }
    }

    // copy sections to RAM, set up caches, and start application
    unpack_load_app(&load_part_pos);
}
```

```
// Is this correct?? _init_start should be interesting
void IRAM_ATTR call_start_cpu1()
{
    asm volatile (\
                  "wsr    %0, vecbase\n" \
                  ::"r"(&_init_start));

    ...
}
```


```
// This is also an important function, it prepares for start of the applicaton
void start_cpu0_default(void)
{
400807a8:	006136        	entry	a1, 48
    esp_set_cpu_freq();     // set CPU frequency configured in menuconfig
400807ab:	ff1b81        	l32r	a8, 40080418 <_init_end+0x18>
400807ae:	0008e0        	callx8	a8
    uart_div_modify(0, (APB_CLK_FREQ << 4) / 115200);
400807b1:	00a0a2        	movi	a10, 0
400807b4:	ff13b1        	l32r	a11, 40080400 <_init_end>
400807b7:	ff1981        	l32r	a8, 4008041c <_init_end+0x1c>
400807ba:	0008e0        	callx8	a8
    ets_setup_syscalls();
400807bd:	ff1881        	l32r	a8, 40080420 <_init_end+0x20>
400807c0:	0008e0        	callx8	a8
    do_global_ctors();
400807c3:	ff1881        	l32r	a8, 40080424 <_init_end+0x24>
400807c6:	0008e0        	callx8	a8
    esp_ipc_init();
400807c9:	ff1781        	l32r	a8, 40080428 <_init_end+0x28>
400807cc:	0008e0        	callx8	a8
    spi_flash_init();
400807cf:	ff1781        	l32r	a8, 4008042c <_init_end+0x2c>
400807d2:	0008e0        	callx8	a8
    xTaskCreatePinnedToCore(&main_task, "main",
400807d5:	0f0c      	movi.n	a15, 0
400807d7:	01f9      	s32i.n	a15, a1, 0
400807d9:	11f9      	s32i.n	a15, a1, 4
400807db:	21f9      	s32i.n	a15, a1, 8
400807dd:	ff09a1        	l32r	a10, 40080404 <_init_end+0x4>
400807e0:	ff0ab1        	l32r	a11, 40080408 <_init_end+0x8>
400807e3:	ff0ac1        	l32r	a12, 4008040c <_init_end+0xc>
400807e6:	0fdd      	mov.n	a13, a15
400807e8:	1e0c      	movi.n	a14, 1
400807ea:	01f1e5        	call8	40082708 <xTaskGenericCreate>
            ESP_TASK_MAIN_STACK, NULL,
            ESP_TASK_MAIN_PRIO, NULL, 0);
    ESP_LOGI(TAG, "Starting scheduler on PRO CPU.");
400807ed:	0099e5        	call8	4008118c <esp_log_timestamp>
400807f0:	0add      	mov.n	a13, a10
400807f2:	ff07e1        	l32r	a14, 40080410 <_init_end+0x10>
400807f5:	3a0c      	movi.n	a10, 3
400807f7:	0ebd      	mov.n	a11, a14
400807f9:	ff06c1        	l32r	a12, 40080414 <_init_end+0x14>
400807fc:	0085a5        	call8	40081058 <esp_log_write>
    vTaskStartScheduler();
400807ff:	020c65        	call8	400828c4 <vTaskStartScheduler>
40080802:	f01d      	retw.n

40080804 <call_start_cpu0>:
```




static void main_task(void* args)
{
    app_main();
    vTaskDelete(NULL);
}





#The ESP32 memory layout
======================


```
Bus Type   |  Boundary Address     |     Size | Target
            Low Address | High Add 
            0x0000_0000  0x3F3F_FFFF             Reserved 
Data        0x3F40_0000  0x3F7F_FFFF   4 MB      External Memory
Data        0x3F80_0000  0x3FBF_FFFF   4 MB      External Memory
            0x3FC0_0000  0x3FEF_FFFF   3 MB      Reserved
Data        0x3FF0_0000  0x3FF7_FFFF  512 KB     Peripheral
Data        0x3FF8_0000  0x3FFF_FFFF  512 KB     Embedded Memory
Instruction 0x4000_0000  0x400C_1FFF  776 KB     Embedded Memory
Instruction 0x400C_2000  0x40BF_FFFF  11512 KB   External Memory
            0x40C0_0000  0x4FFF_FFFF  244 MB     Reserved
            0x5000_0000 0x5000_1FFF   8 KB       Embedded Memory
            0x5000_2000 0xFFFF_FFFF              Reserved

```


When compiling the application an elf file, bin file and map file is generated.

The start adress of the example-app elf file is..... 0x40080804


this is not all that should be loaded loaded. Also partitiontable and bootloader should be loaded.

The linker scripts contains information of how to layout the code and variables.
componenets/esp32/ld


```
  /* IRAM for PRO cpu. Not sure if happy with this, this is MMU area... */
  iram0_0_seg (RX) :                 org = 0x40080000, len = 0x20000


  /* Even though the segment name is iram, it is actually mapped to flash */
  iram0_2_seg (RX) :                 org = 0x400D0018, len = 0x330000

  /* Shared data RAM, excluding memory reserved for ROM bss/data/stack.
     Enabling Bluetooth & Trace Memory features in menuconfig will decrease
     the amount of RAM available.
  */

  dram0_0_seg (RW) :                 org = 0x3FFB0000 + CONFIG_BT_RESERVE_DRAM,
                                     len = 0x50000 - CONFIG_TRACEMEM_RESERVE_DRAM - CONFIG_BT_RESERVE_DRAM

  /* Flash mapped constant data */
  drom0_0_seg (R) :                  org = 0x3F400010, len = 0x800000

  /* RTC fast memory (executable). Persists over deep sleep. */
  rtc_iram_seg(RWX) :                org = 0x400C0000, len = 0x2000

  /* RTC slow memory (data accessible). Persists over deep sleep.
     Start of RTC slow memory is reserved for ULP co-processor code + data, if enabled.
  */
  rtc_slow_seg(RW)  :                org = 0x50000000 + CONFIG_ULP_COPROC_RESERVE_MEM,
                                     len = 0x1000 - CONFIG_ULP_COPROC_RESERVE_MEM
}

/* Heap ends at top of dram0_0_seg */
_heap_end = 0x40000000 - CONFIG_TRACEMEM_RESERVE_DRAM;
```


Detailed layout of iram0
========================

```
  /* Send .iram0 code to iram */
  .iram0.vectors : 
  {
    /* Vectors go to IRAM */
    _init_start = ABSOLUTE(.);
    /* Vectors according to builds/RF-2015.2-win32/esp108_v1_2_s5_512int_2/config.html */
    . = 0x0;
    KEEP(*(.WindowVectors.text));
    . = 0x180;
    KEEP(*(.Level2InterruptVector.text));
    . = 0x1c0;
    KEEP(*(.Level3InterruptVector.text));
    . = 0x200;
    KEEP(*(.Level4InterruptVector.text));
    . = 0x240;
    KEEP(*(.Level5InterruptVector.text));
    . = 0x280;
    KEEP(*(.DebugExceptionVector.text));
    . = 0x2c0;
    KEEP(*(.NMIExceptionVector.text));
    . = 0x300;
    KEEP(*(.KernelExceptionVector.text));
    . = 0x340;
    KEEP(*(.UserExceptionVector.text));
    . = 0x3C0;
    KEEP(*(.DoubleExceptionVector.text));
    . = 0x400;
    *(.*Vector.literal)

    *(.UserEnter.literal);
    *(.UserEnter.text);
    . = ALIGN (16);
    *(.entry.text)
    *(.init.literal)
    *(.init)
    _init_end = ABSOLUTE(.);
  } > iram0_0_seg
```


###Test with loader scripts 


Tried to add a new section in loader script to place a function at reset vector,

```
#define RESET_ATTR __attribute__((section(".boot")))


void RESET_ATTR boot_func()
{
    asm volatile (\
    	 "movi.n	a8, 40080804\n"
         "jx a8");
}

Then we must add this to the linker script.

  _boot_start = 0x40000400;
  .boot_func _boot_start :
  {
    KEEP(*(.boot)) ;
  }
```


Try compile with,

  make VERBOSE=1 then replace the -T esp32.common.ld with your modified ld file.

However when we try this we get,
  In function `boot_func':
  main.c:13:(.boot+0x3): dangerous relocation: l32r: literal target out of range (try using text-section-literals): (*UND*+0x26395a4)

# Calling convention 

Xtensa ESP8266 calling convention, not same as ESP32? 


The lx106 used in the ESP8266 implements the CALL0 ABI offering a 16-entry register file (see Diamond Standard 106Micro Controller brochure). By this we can apply the calling convention outlined in the Xtensa ISA reference manual, chapter 8.1.2 CALL0 Register Usage and Stack Layout:

a0 - return address
a1 - stack pointer
a2..a7 - arguments (foo(int16 a,long long b) -> a2 = a, a4/5 = b), if sizefof(args) > 6 words -> use stack
a8 - static chain (for nested functions: contains the ptr to the stack frame of the caller)
a12..a15 - callee saved (registers containing values that must be preserved for the caller)
a15 - optional stack frame ptr

Return values are placed in AR2..AR5. If the offered space (four words) does not meet the required amount of memory to return the result, the stack is used.
disabling interrupts

According to the Xtensa ISA (Instruction Set Architecture) manual the Xtensa processor cores supporting up to 15 interrupt levels – but the used lx106 core only supports two of them (level 1 and 2). The current interrupt level is stored in CINTLEVEL (4 bit, part of the PS register, see page 88). Only interrupts at levels above CINTLEVEL are enabled.

```
In esp_iot_sdk_v1.0.0/include/osapi.h the macros os_intr_lock() and os_intr_unlock() are defined to use the ets_intr_lock() and ets_intr_unlock() functions offered by the ROM. The disassembly reveals nothing special:

disassembly – ets_intr_lock() and ets_intr_unlock():

ets_intr_lock():
40000f74:  006320      rsil  a2, 3           // a2 = old level, set CINTLEVEL to 3 -> disable all interrupt levels supported by the lx106
40000f77:  fffe31      l32r  a3, 0x40000f70  // a3 = *mem(0x40000f70) = 0x3fffcc0
40000f7a:  0329        s32i.n  a2, a3, 0       // mem(a3) = a2 -> mem(0x3fffdcc0) = old level -> saved for what?
40000f7c:  f00d        ret.n

ets_intr_unlock():
40000f80:  006020      rsil  a2, 0           //enable all interrupt levels
40000f83:  f00d        ret.n
```

To avoid the overhead of the function call and the unneeded store operation the following macros to enable / disable interrupts can be used:

macros for disabling/enabling interrupts:

```
#define XT_CLI __asm__("rsil a2, 3");
#define XT_STI __asm__("rsil a2, 0");
```

NOTE: the ability to run rsil from user code without triggering a PriviledgedInstruction exception implies that all code is run on ring0. This matches the information given here https://github.com/esp8266/esp8266-wiki/wiki/Boot-Process
the watchdog

Just keep it disabled. I run into a lot of trouble with it – seemed the wdt_feed() didn’t work for me.
…and its not (well) documented at all.

Some pieces of information I found in the net:

    reverse-C of wdt_feet/task/init: http://esp8266.ru/forum/threads/interesnoe-obsuzhdenie-licenzirovanija-espressif-sdk.52/page-2#post-3505
    about the RTC memory: http://bbs.espressif.com/viewtopic.php?f=7&t=68&p=303&hilit=LIGHT_SLEEP_T#p291

gpio_output_set(uint32 set_mask, uint32 clear_mask, uint32 enable_mask, uint32 disable_mask)

declared in: esp_iot_sdk_v1.0.0/include/gpio.h
defined in: ROM (eagle.rom.addr.v6.ld -> PROVIDE ( gpio_output_set = 0x40004cd0 ))

C example:

gpio_output_set(BIT2, 0, BIT2, 0);  // HIGH
gpio_output_set(0, BIT2, BIT2, 0);  // LOW
gpio_output_set(BIT2, 0, BIT2, 0);  // HIGH
gpio_output_set(0, BIT2, BIT2, 0);  // LOW

disassembly – call to gpio_output_set(BIT2, 0, BIT2, 0):

```
40243247:       420c            movi.n  a2, 4                                     // a2 = 4
40243249:       030c            movi.n  a3, 0                                     // a3 = 0
4024324b:       024d            mov.n   a4, a2                                    // a4 = 4
4024324d:       205330          or      a5, a3, a3                                // a5 = 0
40243250:       f79001          l32r    a0, 40241090 <system_relative_time+0x18>  // a0 = *mem(40241090) = 0x40004cd0
40243253:       0000c0          callx0  a0                                        // call 0x40004cd0 - gpio_output_set

disassembly – gpio_output_set (thanks to By0ff for the ROM dump):

> xtensa-lx106-elf-objdump -m xtensa -EL  -b binary --adjust-vma=0x40000000 --start-address=0x40004cd0 --stop-address=0x40004ced -D 0x4000000-0x4011000/0x4000000-0x4011000.bin

0x4000000-0x4011000/0x4000000-0x4011000.bin:     file format binary

Disassembly of the .data section:

40004cd0 <.data+0x4cd0>:
40004cd0:       f0bd61          l32r    a6, 0x40000fc4  // a6 = *mem(0x40000fc4) = 0x60000200
40004cd3:       0020c0          memw                    // finish all mem operations before next op
40004cd6:       416622          s32i    a2, a6, 0x104   // mem(a6 + 0x104) = a2 -> mem(0x60000304) = 4 (SET)
40004cd9:       0020c0          memw
40004cdc:       426632          s32i    a3, a6, 0x108   // mem(a6 + 0x108) = a3 -> mem(0x60000308) = 0 (CLR)
40004cdf:       0020c0          memw
40004ce2:       446642          s32i    a4, a6, 0x110   // mem(a6 + 0x110) = a4 -> mem(0x60000310) = 4 (DIR -> OUTPUT)
40004ce5:       0020c0          memw
40004ce8:       456652          s32i    a5, a6, 0x114   // mem(a6 + 0x114) = a5 -> mem(0x60000314) = 0 (DIR -> INPUT)
40004ceb:       f00d            ret.n                   // return to the caller

> od -A x -j 0xfc4 -N 4 -x 0x4000000-0x4011000/0x4000000-0x4011000.bin
000fc4 0200 6000            // *mem(0x40000fc4) = 0x60000200
000fc8
```
