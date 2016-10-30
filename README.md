 qemu_esp32
Add tensilica esp32 cpu and a board to qemu and dump the rom to learn more about esp-idf
###ESP32 in QEMU.

This documents how to add an esp32 cpu and a simple esp32 board to qemu
in order to run an app compiled with the SDK in QEMU. Esp32 is a 240 MHz dual core Tensilica LX6 microcontroller.
It might not be possible but its a good way to learn about qemu and esp32.


By following the instructions here, I added esp32 to qemu.
http://wiki.linux-xtensa.org/index.php/Xtensa_on_QEMU

prerequisites Debian/Ubuntu:
```
sudo apt-get install libpixman-1-0 libpixman-1-dev 
```

Clone qemu and qemu-esp32 and apply the patch.
```
git clone git://git.qemu.org/qemu.git
cd qemu
git submodule update --init dtc
cd ..

git clone https://github.com/Ebiroll/qemu_esp32.git qemu_esp32_patch
cd qemu_esp32_patch/qemu-patch
./maketar.sh
cp qemu-esp32.tar ../../qemu

cd ../../qemu
tar xvf qemu-esp32.tar
```

in qemu source tree, manually add to makefiles:
```
hw/xtensa/Makefile.objs
  obj-y += esp32.o

target-xtensa/Makefile.objs
  obj-y += core-esp32.o
```
###Building the patched qemu
```
mkdir ../qemu_esp32
cd ../qemu_esp32
Then run configure as,
../qemu/configure --disable-werror --prefix=`pwd`/root --target-list=xtensa-softmmu,xtensaeb-softmmu
```

The goal is to run the app-template or bootloader and connect the debugger. Connecting the debugger works quite well
but we fall back to the command line interpreter. 
UART emulation is not so good as output is only on stderr. It would be much better if we could use the qemu driver with, serial_mm_init()


#Dumping the ROM0 & ROM1 using esp-idf esptool.py
```
cd ..
git clone --recursive https://github.com/espressif/esp-idf.git
# dump_mem ROM0(776KB) to rom.bin
esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 -b 921600 -p /dev/ttyUSB0 dump_mem 0x40000000 0x000C2000 rom.bin

# dump_mem ROM1(64KB) to rom1.bin
esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 -b 921600 -p /dev/ttyUSB0 dump_mem 0x3FF90000 0x00010000 rom1.bin

```

#Dumping the ROM
set the environment properly. Build the romdump app and flash it.
Use i.e screen as serial terminal.

```
screen /dev/ttyUSB0 115200
Ctrl-A  then H   (save output)
Press and hold the boot button, then press reset. This puts the ESP to download mode.
Ctrl-A  then \   (exit, detach)
make flash
screen /dev/ttyUSB0 115200
Ctrl-A  then H   (save output)
press  reset on ESP to get start.
```
When finnished trim the capturefile (remove all before and after the dump data) and call it, test.log
Notice that there are two dumps search for ROM and ROM1
Compile the dump to rom program.
```
gcc torom.c -o torom
torom
If successfull you will have a binary rom dump, rom.bin
Then also create the file rom1.bin
If you start with second part and then do
mv rom.bin rom1.bin
Then you can do the first part
Those two files will be loaded by qemu and must be in same directory as you start qemu.
```


### Start qemu
```
  > xtensa-softmmu/qemu-system-xtensa -d guest_errors,int,mmu,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S

```

### Start the debugger
```
  > xtensa-esp32-elf-gdb build/app-template.elf
  (gdb) target remote:1234

  (gdb) x/10i $pc
  (gdb) x/10i 0x40000400
```

To disassemble a specific function you can set pc from gdb,

```
  (gdb) set $pc=0x40080804

  (gdb) set $pc=<tab>  to let gdb list available functions
  (gdb) layout asm
  (gdb) si
  (gdb) ni  (skips calls)
  (gdb) finish (run until ret)
```
Setting programcounter (pc) to a function,

```
  (gdb) set $pc=&call_start_cpu0
  (gdb) set $pc=&uart_tx_one_char 
```


```
  (gdb) x/20i $pc
  (gdb) p/x $a3          (register as hex)

```

#More useful commands
Dump mixed source/disassemply listing,
```
xtensa-esp32-elf-objdump -d -S build/bootloader/bootloader.elf
xtensa-esp32-elf-objdump -d build/main/main.o
```
Try adding simple assembly or functions and looḱ at the generated code,

```
void retint() {
    asm volatile (\
    	 "RFDE\n");
}

int test(int in) {
    return (in+1);
}

void jump() {
    asm volatile (\
    	 "jx a0\n");
    retint();
}
```


```
00000000 <retint>:
   0:	004136        	entry	a1, 32
   3:	003200        	rfde
   6:	f01d      	    retw.n

Disassembly of section .text.test:

00000000 <test>:
   0:	004136        	entry	a1, 32
   3:	221b      	addi.n	a2, a2, 1
   5:	f01d      	retw.n

Disassembly of section .text.jump:

00000000 <jump>:
   0:	004136        	entry	a1, 32
   3:	0000a0        	jx	a0
   6:	000081        	l32r	a8, fffc0008 <jump+0xfffc0008>
   9:	0008e0        	callx8	a8
   c:	f01d      	    retw.n
```


#Results
I got my ESP32-dev board from Adafruit. I have made two dumps and mapped the dumps into the files rom.bin & rom1.bin
It actually runs from the rom dump. 

However the instruction wsr.memctl does not work in QEMU but can be patched like this (in qemu tree).

```
static bool gen_check_sr(DisasContext *dc, uint32_t sr, unsigned access)
{
    if (!xtensa_option_bits_enabled(dc->config, sregnames[sr].opt_bits)) {
        if (sregnames[sr].name) {
            qemu_log_mask(LOG_GUEST_ERROR, "SR %s is not configured\n", sregnames[sr].name);
        } else {
            qemu_log_mask(LOG_UNIMP, "SR %d %s is not implemented\n", sr);
        }
        //gen_exception_cause(dc, ILLEGAL_INSTRUCTION_CAUSE);
```


We get the following utput after running, flash read err, 1000

I guess better flash emulation is necessary or we could patch the 
rom function -- ets_unpack_flash_code  located at 0x40007018
The serial device should also be emulated differently. Now serial output goes to stderr.

```
If setting breakpoint here,
b *0x40007018
finish
layout asm
set $a10=0
If we continue we pass through ets_run 0x400066bc
Patching the result of ets_unpack_flash_code and entering 
the elf load value into 3ffe0400  user_code_start gets this bootloop output,

xtensa-softmmu/qemu-system-xtensa -d guest_errors,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf  -S -s > io.txt

(gdb) b *0x40007c29
(gdb) c
// Look at Elf entry.. and set pc.
(gdb) set $pc=0x40080814       
(gdb) b start_cpu0_default
(gdb) b app_main
(gdb) c

SR 97 is not implemented
SR 97 is not implemented
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing heap allocator:
I (1) heap_alloc_caps: Region 19: 3FFB3D10 len 0002C2F0 tag 0
I (1) heap_alloc_caps: Region 25: 3FFE8000 len 00018000 tag 1
check b=0x3ffb3d1c size=180948 ok
check b=0x3ffdfff0 size=0 ok
check b=0x3ffe800c size=98276 ok
I (2) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
rtc v112 Sep 26 2016 22:32:10
XTAL 0M
TBD(pc = 40083be6): /home/olas/qemu/target-xtensa/translate.c:1354
              case 6: /*RER*/ Not implemented in qemu simulation

I (3) cpu_start: Starting scheduler on PRO CPU.
WUR 234 not implemented, TBD(pc = 400912c3): /home/olas/qemu/target-xtensa/translate.c:1887
WUR 235 not implemented, TBD(pc = 400912c8): /home/olas/qemu/target-xtensa/translate.c:1887
WUR 236 not implemented, TBD(pc = 400912cd): /home/olas/qemu/target-xtensa/translate.c:1887
RUR 234 not implemented, TBD(pc = 40091301): /home/olas/qemu/target-xtensa/translate.c:1876
RUR 235 not implemented, TBD(pc = 40091306): /home/olas/qemu/target-xtensa/translate.c:1876
RUR 236 not implemented, TBD(pc = 4009130b): /home/olas/qemu/target-xtensa/translate.c:1876

The WUR and RUR instructions are specific for the ESP32. I got to app_main but stopped in
nvs_flash_init()



Most likely the memory mapping is not correct in esp32.c.
Fixing this is saved for a rainy day. Today is rainy. Now serial output also works from qemu,


By commenting out,
    //nvs_flash_init(); & /system_init();
I was able to run the romdump.. in qemu
void app_main()
{
    //nvs_flash_init();
    //system_init();

TODO, fix so that nvs_flash_init() & system_init(); is possible to run in qemu.

```



```

make menuconfig and add FreeRTOS head debugging...

(gdb) b *0x40007c29
(gdb) c
(gdb) set $pc=0x40080814
(gdb) b start_cpu0_default
(gdb) b  xPortStartScheduler

SR 97 is not implemented
SR 97 is not implemented
ets Jun  8 2016 00:22:57
rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing heap allocator:
I (1) heap_alloc_caps: Region 19: 3FFB3D10 len 0002C2F0 tag 0
I (1) heap_alloc_caps: Region 25: 3FFE8000 len 00018000 tag 1
xtensa_cpu_do_interrupt(2) pc = 40081aa8, a0 = 00000000, ps = 00060d33, ccount = 0000ad84
xtensa_cpu_do_interrupt(4) pc = 40007d0c, a0 = 40000740, ps = 00060f30, ccount = 0000adf3
xtensa_cpu_do_interrupt(4) pc = 40009203, a0 = 80007c29, ps = 00060130, ccount = 0000ae03
check b=0x3ffb3d1c size=180948 ok
check b=0x3ffdfff0 size=0 ok
check b=0x3ffe800c size=98276 ok
xtensa_cpu_do_interrupt(5) pc = 400d05d6, a0 = 3ff40000, ps = 00060530, ccount = 0000cc89
I (2) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
------------ start_cpu0_default ---------------
xtensa_cpu_do_interrupt(4) pc = 40009203, a0 = 80007c29, ps = 00060130, ccount = 0000e8fd
rtc v112 Sep 26 2016 22:32:10
XTAL 0M
xtensa_cpu_do_interrupt(4) pc = 40082927, a0 = 800808eb, ps = 00060330, ccount = 00015448
xtensa_cpu_do_interrupt(4) pc = 40081aa5, a0 = 800807dc, ps = 00060533, ccount = 00015460
xtensa_cpu_do_interrupt(5) pc = 4000beb4, a0 = 80081b58, ps = 00060930, ccount = 0001557f
xtensa_cpu_do_interrupt(5) pc = 400d084d, a0 = 00000000, ps = 00060730, ccount = 0001558f
xtensa_cpu_do_interrupt(4) pc = 40081aa5, a0 = 800808eb, ps = 00060333, ccount = 00015604
xtensa_cpu_do_interrupt(4) pc = 40081aa5, a0 = 800807e2, ps = 00060533, ccount = 00015cfe
TBD(pc = 40083be6): /home/olas/qemu/target-xtensa/translate.c:1354
                case 6: /*RER*/ Not implemented in qemu simulation

xtensa_cpu_do_interrupt(5) pc = 40082baa, a0 = 80081b58, ps = 00060930, ccount = 00016866
xtensa_cpu_do_interrupt(5) pc = 400d06d7, a0 = 00000000, ps = 00060730, ccount = 00016874
I (3) cpu_start: Starting scheduler on PRO CPU.
-----------------  xPortStartScheduler---------------------
(gdb) where

#0  0x4000bfdf in ?? ()
#1  0x40081af0 in vPortCPUAcquireMutex (mux=0x3ffb3420 <xTaskQueueMutex>)
    at /home/olas/esp/esp-idf/components/freertos/./port.c:339
#2  0x40082736 in vTaskSwitchContext () at /home/olas/esp/esp-idf/components/freertos/./tasks.c:2300
#3  0x4008394b in _frxt_dispatch () at /home/olas/esp/esp-idf/components/freertos/./portasm.S:410


WUR 234 not implemented, TBD(pc = 400912c3): /home/olas/qemu/target-xtensa/translate.c:1887
WUR 235 not implemented, TBD(pc = 400912c8): /home/olas/qemu/target-xtensa/translate.c:1887
WUR 236 not implemented, TBD(pc = 400912cd): /home/olas/qemu/target-xtensa/translate.c:1887

We get some kind of exception and en

io read 000003f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x01
```




```
xtensa-softmmu/qemu-system-xtensa -d guest_errors,int,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf    > io.txt 
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
flash read err, 1000
Falling back to built-in command interpreter.
```

Some i/o register name mapping is probably wrong.  The values returned are more than likely wrong.
tlb_fill() is probably a cached read of instructions or data.
```
Terminal 1
>xtensa-softmmu/qemu-system-xtensa -d guest_errors,int,mmu,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf   -S -s

Terminal 2
>xtensa-esp32-elf-gdb build/app-template.elf
(gdb) target remote:1234

//Uart_Init, 0x40009120
(gdb) b *0x40009120
(gdb) layout asm
//uart_tx_one_char, 0x40009200
//(gdb) b *0x40009200

xtensa-softmmu/qemu-system-xtensa -d guest_errors,int,mmu,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf   -S -s 
Elf entry 400807BC
serial: event 2
tlb_fill(40000400, 2, 0) -> 40000400, ret = 0
tlb_fill(400004b3, 2, 0) -> 400004b3, ret = 0
tlb_fill(400004e3, 2, 0) -> 400004e3, ret = 0
SR 97 is not implemented
SR 97 is not implemented
tlb_fill(4000d4f8, 0, 0) -> 4000d4f8, ret = 0
tlb_fill(3ffae010, 1, 0) -> 3ffae010, ret = 0
tlb_fill(4000f0d0, 0, 0) -> 4000f0d0, ret = 0
tlb_fill(3ffe0010, 1, 0) -> 3ffe0010, ret = 0
tlb_fill(3ffe1000, 1, 0) -> 3ffe1000, ret = 0
tlb_fill(400076c4, 2, 0) -> 400076c4, ret = 0
tlb_fill(4000c2c8, 2, 0) -> 4000c2c8, ret = 0
tlb_fill(3ff9918c, 0, 0) -> 3ff9918c, ret = 0
tlb_fill(3ffe3eb0, 1, 0) -> 3ffe3eb0, ret = 0
tlb_fill(3ff00044, 0, 0) -> 3ff00044, ret = 0
io read 00000044  DPORT  3ff00044=8E6
tlb_fill(400081d4, 2, 0) -> 400081d4, ret = 0
tlb_fill(3ff48034, 0, 0) -> 3ff48034, ret = 0
io write 00000044 000008E6 io read 00048034 RTC_CNTL_RESET_STATE_REG 3ff48034=3390
io read 00048088 RTC_CNTL_DIG_ISO_REG 3ff48088=00000000
io write 00048088 00000000 RTC_CNTL_DIG_ISO_REG 3ff48088
io read 00048088 RTC_CNTL_DIG_ISO_REG 3ff48088=00000000
io write 00048088 00000400 RTC_CNTL_DIG_ISO_REG 3ff48088
tlb_fill(40009000, 2, 0) -> 40009000, ret = 0
tlb_fill(3ff40010, 1, 0) -> 3ff40010, ret = 0
serial: write addr=0x4 val=0x1
tlb_fill(3ff50010, 1, 0) -> 3ff50010, ret = 0
tlb_fill(400067fc, 2, 0) -> 400067fc, ret = 0
tlb_fill(4000bfac, 2, 0) -> 4000bfac, ret = 0
tlb_fill(3ff9c2b9, 0, 0) -> 3ff9c2b9, ret = 0
tlb_fill(3ff5f06c, 0, 0) -> 3ff5f06c, ret = 0
io write 00050010 00000001 
io read 0005f06c TIMG_RTCCALICFG1_REG 3ff5f06c=25
tlb_fill(3ff5a010, 0, 0) -> 3ff5a010, ret = 0
io read 0005a010 TIMG_T0ALARMLO_REG 3ff5a010=01
tlb_fill(40005cdc, 0, 0) -> 40005cdc, ret = 0
io read 000480b4 RTC_CNTL_STORE5_REG 3ff480b4=00000000
io read 000480b4 RTC_CNTL_STORE5_REG 3ff480b4=00000000
io write 000480b4 18CB18CB RTC_CNTL_STORE5_REG 3ff480b4

----------- break Uart_Init, 0x40009120
(gdb) finish
// Beforere add    serial_mm_init(system_io, 0x40000 , 2, xtensa_get_extint(env, 0),
//            115200, serial_hds[0], DEVICE_LITTLE_ENDIAN);

tlb_fill(4000a484, 2, 0) -> 4000a484, ret = 0
io read 00048474 
io write 00048474 00000032 
io read 00048474 
io write 00048474 00000031 
tlb_fill(3ff49088, 0, 0) -> 3ff49088, ret = 0
io read 00049088 
io write 00049088 00000033 
io read 00049088 
io write 00049088 00000033 
io write 00040014 00B000E1 UART OUTPUT 
io read 00040020 UART??=0
io write 00040020 00060000 UART OUTPUT 
io read 00040020 UART??=0
io write 00040020 00000000 UART OUTPUT 
tlb_fill(400040ac, 0, 0) -> 400040ac, ret = 0
io write 00040020 0800001C UART OUTPUT 
io read 00040020 UART??=0
io write 00040020 04060000 UART OUTPUT 
io read 00040020 UART??=0
io write 00040020 00000000 UART OUTPUT 
io write 00040024 00000001 UART OUTPUT 
io write 00040010 0000FFFF UART OUTPUT �
io read 0004000c UART READ
io write 0004000c 00000033 UART OUTPUT 3
tlb_fill(3ff0018c, 1, 0) -> 3ff0018c, ret = 0
io write 0000018c 00000005 
-----------------------------------------
// After letting serial0 be handled by qemu.
// Also set #define DEBUG_SERIAL  in hw/char/serial.c
// 
tlb_fill(4000a484, 2, 0) -> 4000a484, ret = 0
io read 00048474 
io write 00048474 00000000 
io read 00048474 
io write 00048474 00000000 
tlb_fill(3ff49088, 0, 0) -> 3ff49088, ret = 0
io read 00049088 
io write 00049088 00000000 
io read 00049088 
io write 00049088 00000000 
serial: write addr=0x5 val=0xb000e1
io read 00040020 UART??=0
io write 00040020 00060000 UART OUTPUT
io read 00040020 UART??=0
io write 00040020 00000000 UART OUTPUT
tlb_fill(400040ac, 0, 0) -> 400040ac, ret = 0
io write 00040020 0800001C UART OUTPUT
io read 00040020 UART??=0
io write 00040020 04060000 UART OUTPUT
io read 00040020 UART??=0
io write 00040020 00000000 UART OUTPUT
io write 00040024 00000001 UART OUTPUT
serial: write addr=0x4 val=0xffff
serial: read addr=0x3 val=0x00
serial: write addr=0x3 val=0x1
serial: speed=9600 parity=N data=6 stop=1
tlb_fill(3ff0018c, 1, 0) -> 3ff0018c, ret = 0
io write 0000018c 00000005 
-------------- after finish --------------
//ets_printf, 0x40007d54
(gdb) b *0x40007d54



-------------------------------

// When running we end up in this loop
// Probably waiting for input of the fallbask interpreter
(gdb) where
#0  0x4009110c in ?? ()
#1  0x4009118c in ?? ()
#2  0x40090747 in wDev_AppendRxBlocks ()
#3  0x400912ee in ?? ()
#4  0x4005a9e1 in ?? ()
#5  0x40007ba7 in ?? ()
#6  0x40000740 in ?? ()


    0x40091189      call8  0x4009110c                                                                                              
    0x4009118c      l8ui   a8, a3, 0                                                                                               
    0x4009118f      mov.n  a10, a3                                                                                                 
    0x40091191      l32r   a9, 0x400900c8 <rcGetSched+108>                                                                         
    0x40091194      bnez.n a8, 0x4009119c                                                                                          
    0x40091196      l32i   a2, a9, 0                                                                                               
    0x40091199      beqz   a2, 0x40091189                                                                                          
    0x4009119c      l32i.n a2, a9, 0                                                                                               
    0x4009119e      movi.n a3, 0                                                                                                   
    0x400911a0      s8i    a3, a10, 0                                                                                              
    0x400911a3      movi.n a3, 67                                                                                                  
    0x400911a5      moveqz a3, a8, a2                                                                                              
    0x400911a8      mov.n  a2, a3                                                                                                  
    0x400911aa      retw.n                                


```

An effort to debug the flash read error,

```
Try setting breakpoints here
spi_flash_attach, 0x40062a6c
(gdb) b  *0x40062a6c

    0x40062a6c      entry  a1, 32                                               
    0x40062a6f      or     a10, a2, a2                                          
    0x40062a72      call8  0x40061f84                   // SelectSpiFunction                                  
    0x40062a75      l32r   a8, 0x400626bc               // 0a8 contains 0x3ff43034                        
    0x40062a78      movi.n a9, -2                                               
    0x40062a7a      memw                                                        
    0x40062a7d      l32i.n a10, a8, 0                   //    io read 00043034  , SPI_PIN_REG                  
    0x40062a7f      movi.n a11, 4                                               
    0x40062a81      and    a9, a10, a9                                          
    0x40062a84      memw                                                        
    0x40062a87      s32i.n a9, a8, 0                                            
    0x40062a89      memw                                                        
    0x40062a8c      l32i.n a10, a8, 0    
    0x40062a8e      movi.n a9, 2                                                
    0x40062a90      or     a9, a10, a9                                          
    0x40062a93      memw                                                        
    0x40062a96      s32i.n a9, a8, 0                                            
    0x40062a98      movi   a10, 0                                               
    0x40062a9b      call8  0x40061d90                  //  spi_dummy_len_fix                                         
    0x40062a9e      movi.n a10, 1                                               
    0x40062aa0      movi.n a11, 4                                               
    0x40062aa2      call8  0x40061d90                 //  spi_dummy_len_fix                          
    0x40062aa5      l32r   a8, 0x40062a44             //  a8=3043                        
    0x40062aa8      l32r   a9, 0x40062a40             //  a9=0x3ff42018
..

    io write 00042018 00003043     // SPI_CLOCK_REG 
    io write 00043018 00003043 

..
    0x40062b00      l32r   a9, 0x400626dc                                       
    0x40062b03      memw                                                        
    0x40062b06      l32i.n a10, a8, 0                                           
    0x40062b08      and    a9, a10, a9                                          
    0x40062b0b      l32r   a10, 0x40061b64                                      
    0x40062b0e      or     a9, a9, a10                                          
    0x40062b11      memw                                                        
    0x40062b14      s32i.n a9, a8, 0                                            
    0x40062b16      movi   a10, 0                                               
    0x40062b19      call8  0x40061b88             // 
    0x40062b1c      l32r   a8, 0x40062a5c                                       
    0x40062b1f      movi.n a9, 3                                                
    0x40062b21      memw               

 .. 

    0x40062b3b      l32r   a8, 0x400621ac                                       
    0x40062b3e      memw                                                        
    0x40062b41      s32i   a10, a8, 0                                           
    0x40062b44      l32r   a10, 0x40062a68                                      
    0x40062b47      memw                                                        
    0x40062b4a      s32i   a9, a10, 0                                           
    0x40062b4d      memw                                                        
    0x40062b50      l32i.n a9, a8, 0                                            
    0x40062b52      bnez   a9, 0x40062b4d                                       
    0x40062b55      mov.n  a11, a3                                              
    0x40062b57      movi.n a10, 5                                               
    0x40062b59      call8  0x40062944                   // SPIReadModeCnfig                                    
    0x40062b5c      retw.n              

...

SPIReadModeCnfig...
Both SPI0 & SPI1 registers are written/read in parallell.
This is th I/O activity of SPIReadModeCnfig
io read 000420f8 SPI_EXT2_REG 3ff420f8=0
io read 000430f8 ????? 3ff430f8=0
io read 00042008 
io write 00042008 00000000 
io read 00043008 
io write 00043008 00000000 
io read 00042008 
io write 00042008 00000000 
io read 00043008 
io write 00043008 00000000 
io read 0004301c 
io write 0004301c 00000000 
io read 0004301c 
io write 0004301c 00000000 
io read 0004301c 
io write 0004301c 50000000 
io read 00043020 
io write 00043020 5C000000 
io read 00043024 
io write 00043024 00000003 


After return from spi_flash_attach..

    0x400079c4      l32r   a4, 0x40006878           //        0x3ffe0400                     
    0x400079c7      s32i.n a2, a4, 0                                            
    0x400079c9      j      0x40007a8d                                           
    0x400079cc      or     a12, a0, a0                                          
    0x400079cf      lsi    f0, a2, 160                                          
    0x400079d2      lsi    f0, a12, 0x3e0                                       
    0x400079d5      extui  a9, a9, 0, 5                                         
    0x400079d8      bne    a9, a8, 0x400079ed                                   
    0x400079db      mov.n  a11, a2                                              
    0x400079dd      or     a10, a4, a4                                          
    0x400079e0      call8  0x40009034                                           
    0x400079e3      extui  a11, a10, 0, 16                                      
    0x400079e6      mov.n  a10, a4   

// Also we can break at mmu_init, 0x400095a4
(gdb) b *0x400095a4
(gdb) finish
I heavliy use the reset function of the qemu console.

Sowhere around here, the rom code decides that we have 
  flash read err, 1000

    0x40007b45      l32r   a2, 0x400068ac                                       
    0x40007b48      movi.n a10, 0                                               
    0x40007b4a      mov.n  a12, a2                                              
    0x40007b4c      mov.n  a13, a10                                             
    0x40007b4e      movi.n a14, 64                                              
    0x40007b50      mov.n  a11, a10                                             
    0x40007b52      movi.n a15, 1                                               
    0x40007b54      call8  0x400095e0             // cache_flash_mmu_set
// cache_flash_mmu_set results in this activity..
io read 00000044  DPORT  3ff00044=8E6
io read 0000005c 
io read 0000005c 
io write 0000005c 0000003F 
io read 00000044  DPORT  3ff00044=8E6
io write 00000044 000008FF 
io write 00010000 00000000 
io write 00000044 000008E6 
io write 0000005c 00000000 
io read 00000044  DPORT  3ff00044=8E6
io write 00000044 000008E6 

    0x40007b57      l32r   a3, 0x40007000                                      
    0x40007b5a      movi.n a4, -17                                              
    0x40007b5c      memw                                                        
    0x40007b5f      l32i.n a5, a3, 0                                            
    0x40007b61      movi.n a10, 0            
    0x40007b63      and    a4, a5, a4                                           
    0x40007b66      memw                                                        
    0x40007b69      s32i   a4, a3, 0                                            
    0x40007b6c      call8  0x40009950     //  Cache_Read_Init   
// Cache read init results in this activity
io read 00000040  DPORT  3ff00040=28
io write 00000040 00000020 
io read 00043050 
io write 00043050 00000000 
io read 00000040  DPORT  3ff00040=28
io write 00000040 00000028 
io read 00000040  DPORT  3ff00040=28
io write 00000040 00000038 
io read 00000040  DPORT  3ff00040=28
io read 00000040  DPORT  3ff00040=28
io write 00000040 00000028 
io read 00043050 
io write 00043050 00000001 
io read 00000040  DPORT  3ff00040=28
io write 00000040 00000028 

    0x40007b6f      mov.n  a11, a2                                              
    0x40007b71      movi   a10, 0                                               
    0x40007b74      call8  0x4005cbcc          // ets_secure_boot_check_start 
io read 0005a018, gpio...
    
    0x40007b77      extui  a13, a10, 0, 8                                       
    0x40007b7a      l32r   a11, 0x40006878                                      
    0x40007b7d      l32r   a10, 0x4000062c                                      
    0x40007b80      movi.n a12, 0                                               
    0x40007b82      movi.n a14, 1                                               
    0x40007b84      call8  0x40007018         // ets_unpack_flash_code                                  
    0x40007b87      beqz   a10, 0x40007bf5                                      
    0x40007b8a      l32r   a2, 0x400076b8                                       
    0x40007b8d      memw                                                        
    0x40007b90      l32i.n a2, a2, 0                                            
    0x40007b92      bbci   a2, 2, 0x40007ba4                                    
    0x40007b95      l32r   a10, 0x40007684            



Detailed analysis of Cache_Read_Init
Cache_Read_Init
    0x40009950      entry  a1, 32                                               
    0x40009953      movi.n a9, -9                                               
    0x40009955      l32r   a8, 0x40009944        // 0x3ff00040                               
    0x40009958      beqz.n a2, 0x4000995d        // a2=0 (xp /x 0x4000995d in qemu)  a80020c0                             
    0x4000995a      l32r   a8, 0x40009948        // 0x3ff00058                               
    0x4000995d      memw                                                        
    0x40009960      l32i.n a10, a8, 0                                           
    0x40009962      and    a9, a10, a9                                          
    0x40009965      memw                                                        
    0x40009968      s32i.n a9, a8, 0            // a9=                                 
    0x4000996a      l32r   a8, 0x4000994c       // 0x3ff43050                                
    0x4000996d      movi.n a9, -2                                               
    0x4000996f      memw
    0x40009972      l32i.n a10, a8, 0                                           
    0x40009974      and    a9, a10, a9                                          
    0x40009977      memw                                                        
    0x4000997a      s32i.n a9, a8, 0                                            
    0x4000997c      movi.n a9, -17                                              
    0x4000997e      bnez.n a2, 0x400099ac                                       
    0x40009980      l32r   a8, 0x40009944       //  0x3ff00040
    0x40009983      memw                                                        
    0x40009986      l32i.n a10, a8, 0           // io read 00000040  DPORT  3ff00040=28
    0x40009988      and    a9, a10, a9                                          
    0x4000998b      memw                               
    0x4000998e      s32i.n a9, a8, 0           // io write (PRO_CACHE_CTRL_REG) 00000040 00000028                                  
    0x40009990      memw                                                        
    0x40009993      l32i.n a10, a8, 0          // io read 00000040 00000028                                   
    0x40009995      movi.n a9, 16                                               
    0x40009997      or     a9, a10, a9         // 0x38                                 
    0x4000999a      memw                                                        
    0x4000999d      s32i.n a9, a8, 0           // io write 00000040 00000038                                 
    0x4000999f      movi.n a9, 32                                               
    0x400099a1      memw                                                        
    0x400099a4      l32i.n a10, a8, 0               //  io read 00000040                           
    0x400099a6      bnone  a10, a9, 0x400099a1     //   0x80020c0                   
    0x400099a9      j      0x400099d5                                           
    0x400099ac      l32r   a8, 0x40009948                                       
    0x400099af      memw                                                        
    0x400099b2      l32i.n a10, a8, 0                                           
    0x400099b4      and    a9, a10, a9                                          
    0x400099b7      memw                                                        
    0x400099ba      s32i.n a9, a8, 0                                            
    0x400099bc      memw                                                        
    0x400099bf      l32i.n a10, a8, 0                                           
    0x400099c1      movi.n a9, 16                                               
    0x400099c3      or     a9, a10, a9                



    0x400099d5      memw                                                        
    0x400099d8      l32i.n a10, a8, 0                                           
    0x400099da      movi.n a9, -17                                              
    0x400099dc      and    a9, a10, a9                                          
    0x400099df      memw                                                        
    0x400099e2      s32i.n a9, a8, 0                                            
    0x400099e4      l32r   a8, 0x4000994c                                       
    0x400099e7      movi.n a9, 1                                                
    0x400099e9      memw                                                        
    0x400099ec      l32i.n a10, a8, 0                                           
    0x400099ee      or     a9, a10, a9                                          
    0x400099f1      memw                                                        
    0x400099f4      s32i.n a9, a8, 0          



//  cache_flash_mmu_set, 0x400095e0

#0  0x400095e0 in ?? ()    cache_flash_mmu_set
#1  0x400070b7 in ?? ()    in ets_unpack_flash_code
#2  0x40007b87 in ?? ()    
#3  0x40000740 in ?? ()





//When we get here 
0x40007ba4      call8  0x4005a980   
start_tb_console, 0x4005a980


Probably missing ram.. 
static HeapRegionTagged_t regions[]={
	{ (uint8_t *)0x3F800000, 0x20000, 15, 0}, //SPI SRAM, if available
	{ (uint8_t *)0x3FFAE000, 0x2000, 0, 0}, //pool 16 <- used for rom code
	{ (uint8_t *)0x3FFB0000, 0x8000, 0, 0}, //pool 15 <- can be used for BT
	{ (uint8_t *)0x3FFB8000, 0x8000, 0, 0}, //pool 14 <- can be used for BT




```

## _ResetVector  Start of execution (0x40000400)
```
If booting from the romdump we start here, _ResetVector, 0x40000400
0x40000400      j      0x40000450                    // _ResetHandler, 0x40000450
```

## _ResetHandler
```
    0x40000450      movi   a0, 0 
    0x40000453      wsr.intenable  a0                // Turn of interrupts
    0x40000456      rsr.prid       a2                // Check processor ID. 0xABAB on app-cpu and 0xCDCD on pro-cpu 
    0x40000459      l32r   a3, 0x40000404            // a3 contains ABAB                                                             
    0x4000045c      bne    a2, a3, 0x40000471        // a2 should contain CDCD=52685 fot PRO cpu                                                                
    0x4000045f      l32r   a3, 0x40000408            |                                                               
    0x40000462      l32i   a3, a3, 0                 |                                                               
    0x40000465      bbci   a3, 31, 0x40000471        |                                                               
    0x40000468      l32r   a2, 0x4000040c            |                                                               
    0x4000046b      and    a3, a3, a2                |                                                               
    0x4000046e      callx0 a3                        v                                                               
    0x40000471      l32r   a2, 0x40000410                                                                           
    0x40000474      rsr.prid       a3                                                                               
    0x40000477      extui  a3, a3, 0, 8              // a3=CD Extract unsigned immediate , shift ,mask                                                                
    0x4000047a      beqz.n a2, 0x40000480            |   branch if zero                                                           
    0x4000047c      bnez.n a3, 0x40000480            |                                                               
    0x4000047e      s32i.n a0, a2, 0                 v                                                               
    0x40000480      l32r   a2, 0x40000414            // a2 contains   0x40000000                                                            
    0x40000483      wsr.vecbase    a2                // Write special register vecbase        
    0x40000486      movi.n a3, 21                    // load register a3 with 21=0x15
    0x40000486      movi.n a3, 21                    // Twice??      RCW

    RCW Transaction — Execute the S32C1I instruction by sending an RCW transaction on the PIF bus. 
    External logic must then implement the atomic read-compare-write on the memory location. 
    If the Data Cache Option is configured and the memory region is cacheable, 
    any corresponding cache line will be flushed out of the cache by the S32C1I instruction 
    using the equivalent of a DHWBI instruction before the RCW transaction is sent. 
    RCW Transaction may be the only method available for certain memory types 
    or it may be directed by the ATOMCTLregister.


If the address of the RCW transaction targets the Inbound PIF port of another Xtensa processor, 
the targeted Xtensa processor has the Conditional Store Option and the Data RAM Option configured, and the RCW address targets the DataRAM,
the RCW will be performed atomically on the target processor’s DataRAM. No external logic other than PIF bus interconnects 
is necessary to allow an Xtensa processor to atomically access a DataRAM location in another Xtensa processor in this way.

    0x40000488      wsr.atomctl    a3                // Atomic CTL reg 99,                                                                
    0x4000048b      rsil   a2, 1                     // a2 contains 0x1f                               
    0x4000048e      l32r   a2, 0x40000418            // a2 contains    0x2222211f                                                            
    0x40000491      l32r   a5, 0x4000041c            // a5 contains    0xe0000000                                                              
    0x40000494      l32r   a6, 0x40000420            // a6 contains    0x400004c3                                                               
    0x40000497      movi.n a3, 0                                                                                    
    0x40000499      mov.n  a7, a2                    // a7 contains    0x2222211f
    0x4000049b      and    a6, a6, a5                // a6 contains    0x40000000
    0x4000049e      j      0x400004c3      
    0x400004a1      ill                          |                                                                  
    0x400004a4      ill                          |                                                                   
    0x400004a7      ill                          |                                                                   
    0x400004aa      ill                          |                                                                   
    0x400004ad      ill                          |          

// configure_region_protection    

    0x400004b0      witlb  a4, a3                |                                                                   
    0x400004b3      isync                        |                                                                   
    0x400004b6      nop.n                        |                                                                   
    0x400004b8      nop.n                        |                                                                   
    0x400004ba      sub    a3, a3, a5            |                                                                   
    0x400004bd      bltui  a3, 16, 0x400004d5    |                                                                   
^   0x400004c0      srli   a7, a7, 4             v                                                                   
|   0x400004c3      extui  a4, a7, 0, 4          // a4 contains 0xf                                                                   
|   0x400004c6      beq    a3, a6, 0x400004b0    // a6 starts at 0x40000000                                                                    
|   0x400004c9      witlb  a4, a3                                                                                   
|   0x400004cc      sub    a3, a3, a5                                                                               
 _  0x400004cf      bgeui  a3, 16, 0x400004c0                                                                       
    0x400004d2      isync                             //  a5   0xe0000000                                                           
    0x400004d5      l32r   a5, 0x4000041c 
    0x400004d8      movi.n a3, 0                                                                                    
    0x400004da      or     a7, a2, a2                                                                               
^   0x400004dd      extui  a4, a7, 0, 4                                                                             
|   0x400004e0      wdtlb  a4, a3                                                                                   
|   0x400004e3      sub    a3, a3, a5                                                                               
|   0x400004e6      srli   a7, a7, 4                                                                                
| _ 0x400004e9      bgeui  a3, 16, 0x400004dd                                                                       
    0x400004ec      dsync                                                                                           
    0x400004ef      movi   a3, 1                                                                                    
    0x400004f2      rsr.memctl     a2                // Exception here, (patch qemu) a2 contains..0x2222211f, read from 0x40000418 (_ResetVector + 18)
                                                     // I guess qemu has no support for this.. patch qemu or set $pc=0x400004fb to continue.
                                                // a3 contains 1.   
    0x400004f5      or     a2, a2, a3                                                                               
    0x400004f8      wsr.memctl     a2                                                                               
    0x400004fb      l32r   a4, 0x40000424            // a4 contains   0x4000d4f8                                                                
    0x400004fe      l32r   a5, 0x40000428            // a5 contains   0x4000d5c8
    0x40000501      l32i.n a6, a4, 0                 //                                                                 
    0x40000503      l32i.n a7, a4, 4                 // a7 contains      0x3ffae010                                                         
    0x40000505      l32i.n a8, a4, 8                 // a8 contains      0x4000d670                                                             
    0x40000507      l32i.n a2, a4, 12                                                                               
    0x40000509      beqi   a2, 1, 0x40000515  
    0x4000050c      rsr.prid       a2                // Read process id                                                                                                <
    0x4000050f      l32r   a3, 0x40000404            // a3 contains 0xabab                                                                                                 <
    0x40000512      beq    a2, a3, 0x40000520                                                                                                             
 ^   0x40000515      l32i.n a2, a8, 0                                       
 |   0x40000517      s32i.n a2, a6, 0                                                                                                                  
 |   0x40000519      addi.n a6, a6, 4                                                                                                                  
 |   0x4000051b      addi.n a8, a8, 4                                                                                                                  
 |  0x4000051d       bltu   a6, a7, 0x40000515                                                                                                         
 |   0x40000520      addi   a4, a4, 16                                                                                                                  
 |_  0x40000523      bltu   a4, a5, 0x40000501 
    0x40000526      isync

// Initiate registers

    0x40000529      movi.n a1, 1                                                                   
    0x4000052b      wsr.windowstart        a1                                                      
    0x4000052e      wsr.windowbase a0                                                              
    0x40000531      rsync                                                                          
    0x40000534      movi.n a0, 0                                                                   
    0x40000536      l32r   a4, 0x4000042c    // a4= 0x40000954                                                  
    0x40000539      wsr.excsave2   a4                                                              
    0x4000053c      l32r   a4, 0x40000430    // a4=0x40000a28                                                  
    0x4000053f      wsr.excsave3   a4                                                              
    0x40000542      l32r   a4, 0x40000434                                                          
    0x40000545      wsr.excsave4   a4                                                              
    0x40000548      l32r   a5, 0x40000438                                                          
    0x4000054b      s32i   a4, a5, 0                                                               
    0x4000054e      l32r   a4, 0x4000043c                                                          
    0x40000551      wsr.excsave5   a4        // a4=0x40000c68                     
    0x40000554      l32r   a5, 0x40000440    // a5=0x3ffe064c                                                   
    0x40000557      s32i   a4, a5, 0         // a4=0x40000c68                                                     
    0x4000055a      call0  0x40000704      // call _start
```
## _start
```
   0x40000704       movi.n a0, 0                                                                   
    0x40000706      l32r   a1, 0x40000560   // a1=0x3ffe3f20                                                       
    0x40000709      l32r   a3, 0x40000564   // a3= 0x40020                                                   
    0x4000070c      wsr.ps a3               // UM=1 , WOE=1       
       // UM 1 → user vector mode — exceptions need to switch stacks
       // Overflow detection                                                 
    0x4000070f      rsync                                                                          
    0x40000712      l32r   a6, 0x40000568    //  a6=0x4000d5d0                                                   
    0x40000715      l32r   a7, 0x4000056c    // a7=0x4000d66c                                                      
    0x40000718      movi.n a0, 0                                                                   
    0x4000071a      l32i.n a4, a6, 0                                                               
    0x4000071c      l32i.n a5, a6, 4                                                               
    0x4000071e      l32i.n a3, a6, 8                                                               
    0x40000720      beqi   a3, 1, 0x40000735                                                       
    0x40000723      rsr.prid       a2                                                              
    0x40000726      l32r   a3, 0x40000570                                                          
    0x40000729      beq    a2, a3, 0x40000738                                                      
    0x4000072c      j      0x40000735          

    // loop         a4 goes from ?? -   0x3ffae020 -
    0x40000731      s32i.n a0, a4, 0                                                               
    0x40000733      addi.n a4, a4, 4                                                               
    0x40000735      bltu   a4, a5, 0x40000731  
    0x40000738      addi.n a6, a6, 12                                                              
    0x4000073a      bltu   a6, a7, 0x4000071a                                                      
    0x4000073d      call4  0x400076c4    // call rom_main                                                    
    0x40000740      movi.n a2, 1                                                                   
    0x40000742      simcall             // ?? simcall
    0x40000745      break  1, 15            
```



## rom_main
```
   0x400076c4      entry  a1, 112                                                                                                                    <
   0x400076c7      l32r   a11, 0x40007674        // a11=0x3ff9918c                                                                                                   <
   0x400076ca      mov.n  a10, a1                // a10=0x3ffe3eb0                                                                                                  <
   0x400076cc      movi.n a12, 68                // a12=0x44                                                                                                    <
   0x400076ce      call8  0x4000c2c8             // call memcpy                                                                                                     <
   0x400076d1      l32r   a2, 0x40000570         // a2=abab                                                                                                    <
   0x400076d4      rsr.prid       a3                                                                                                                 <
   0x400076d7      bne    a3, a2, 0x400076e9                                                                                                         <
   0x400076da      l32r   a3, 0x40006898                                                                                                             <
   0x400076dd      memw                                                                                                                              <
   0x400076e0      l32i.n a2, a3, 0                                                                                                                  <
   0x400076e2      beqz   a2, 0x400076dd                                                                                                             <
   0x400076e5      j      0x40007bf0                                                                                                                 <

...

...
    0x400076e9      l32r   a2, 0x40007000         // a2=  0x3ff00044                                               
    0x400076ec      movi.n a3, 6                                                                   
    0x400076ee      memw                                                                           
    0x400076f1      l32i.n a4, a2, 0                                                               
    0x400076f3      movi   a10, 0                                                                  
    0x400076f6      or     a3, a4, a3                                                              
    0x400076f9      memw                                                                           
    0x400076fc      s32i   a3, a2, 0                                                               
    0x400076ff      call8  0x400081d4   // rtc_get_reset_reason .. a2=0x3ff00044 , a3=0x26 a4=0x22                                                           
    0x40007702      l32r   a2, 0x40007678                                                          
    0x40007705      l32r   a4, 0x4000767c                                                          
    0x40007708      memw                                                                           
    0x4000770b      l32i.n a5, a2, 0                                                               
    0x4000770d      mov.n  a3, a10                
...


// io read 00048034
// rtc reset reason
## rtc_get_reset_reason
...
    0x400081d4      entry  a1, 32                                                                  
    0x400081d7      l32r   a8, 0x400081d0            // a8=0x3ff48034                                              
    0x400081da      bnez.n a2, 0x400081e8                                                          
    0x400081dc      memw                                                                           
    0x400081df      l32i.n a2, a8, 0                                                               
    0x400081e1      extui  a2, a2, 0, 6                                                            
    0x400081e4      retw.n      
...



// Here we get an io read register 0x38...

   <0x400076d4      rsr.prid       a3                                                                                                                 <
   <0x400076d7      bne    a3, a2, 0x400076e9                                                                                                         <
   <0x400076da      l32r   a3, 0x40006898                                                                                                             <
   <0x400076dd      memw                                                                                                                              <
   <0x400076e0      l32i.n a2, a3, 0                                                                                                                  <
   <0x400076e2      beqz   a2, 0x400076dd  
   <0x400076e5      j      0x40007bf0                                                                                                                 <
   <0x400076e8      extui  a2, a0, 17, 5                                                                                                              <
   <0x400076eb      .byte 0xfe            


// Lots of calls to... xtos_set_exception_handler
// We also have already called user code done..

   0x40007bf0      l32r   a3, 0x40006878                                                                                                             <
   <0x40007bf3      s32i.n a2, a3, 0                                                                                                                  <
   <0x40007bf5      l32r   a2, 0x400076bc                                                                                                             <
   <0x40007bf8      movi.n a10, 9                                                                                                                     <
   <0x40007bfa      mov.n  a11, a2                                                                                                                    <
   <0x40007bfc      call8  0x4000074c                                                                                                                 <
   <0x40007bff      mov.n  a11, a2                                                                                                                    <
   <0x40007c01      movi.n a10, 0                                                                                                                     <
   <0x40007c03      call8  0x4000074c                                                                                                                 <
   <0x40007c06      mov.n  a11, a2                                                                                                                    <
   <0x40007c08      movi.n a10, 2                                                                                                                     <
   <0x40007c0a      call8  0x4000074c                                                                                                                 <
   <0x40007c0d      mov.n  a11, a2                                                                                                                    <
   <0x40007c0f      movi.n a10, 3                                                                                                                     <
   <0x40007c11      call8  0x4000074c                                                                                                                 <
   <0x40007c14      mov.n  a11, a2                                                                                                                    <
   <0x40007c16      movi.n a10, 28                                                                                                                    <
   <0x40007c18      call8  0x4000074c                                                                                                                 <
   <0x40007c1b      mov.n  a11, a2                                                                                                                    <
   <0x40007c1d      movi.n a10, 29                                                                                                                    <
   <0x40007c1f      call8  0x4000074c                                                                                                                 <
   <0x40007c22      mov.n  a11, a2                                                                                                                    <
   <0x40007c24      movi.n a10, 8                  
    0x40007c26      call8  0x4000074c                                           
    0x40007c29      l32r   a2, 0x40006878             // 3ffe0400  user_code_start !!                         
    0x40007c2c      l32i.n a2, a2, 0                  //                          
    0x40007c2e      beqz   a2, 0x40007c34                                       
    0x40007c31      callx8 a2                         // CALL user_code_start
// I guess this is where we enter user_code, that would be the 
// bootloader. The bootloader reads the partition table and 
// Sets up stacks bss and such for the application
// To start the loaded elf file I guess we could mimic
// unpack_load_app from the bootloader
    0x40007c34      l32r   a10, 0x400076c0                                      
    0x40007c37      call8  0x40007d54            // prints user code done 
    0x40007c3a      l32r   a2, 0x40006888                                       
    0x40007c3d      l32i.n a2, a2, 0                                            
    0x40007c3f      beqz   a2, 0x40007c48                                       
    0x40007c42      callx8 a2                                                   
    0x40007c45      j      0x40007c4b                                           
    0x40007c48      call8  0x400066bc      // ets_run  
    0x40007c4b      movi.n a2, 0           
    0x40007c4d      retw.n    

When entering ets_run we already have the printout... user code done

ets_run

    0x400066bc      entry  a1, 32                                               
    0x400066bf      l32r   a3, 0x400066b8                                       
    0x400066c2      call8  0x400067b0          //  ets_intr_lock                                  
    0x400066c5      l32i   a8, a3, 0                                            
    0x400066c8      movi   a4, 32                                               
    0x400066cb      nsau   a2, a8                                               
    0x400066ce      sub    a2, a4, a2                                           
    0x400066d1      extui  a2, a2, 0, 8                                         
    0x400066d4      beqz   a2, 0x40006725      //                                 
    0x400066d7      l32r   a4, 0x40006684                                       
    0x400066da      addi   a2, a2, -1                                           
    0x400066dd      slli   a2, a2, 4                                            
    0x400066e0      add.n  a2, a2, a4       
    0x400066e2      l8ui   a9, a2, 10                                           
    0x400066e5      l32i.n a4, a2, 4                                            
    0x400066e7      l8ui   a10, a2, 8                                           
    0x400066ea      addx8  a4, a9, a4                                           
    0x400066ed      addi.n a9, a9, 1                                            
    0x400066ef      extui  a9, a9, 0, 8                                         
    0x400066f2      s8i    a9, a2, 10                                           
    0x400066f5      bne    a10, a9, 0x400066fd          
    0x400066f8      movi.n a9, 0                                                
    0x400066fa      s8i    a9, a2, 10                                           
    0x400066fd      l8ui   a9, a2, 11                                           
    0x40006700      addi.n a9, a9, -1                                           
    0x40006702      extui  a9, a9, 0, 8                                         
    0x40006705      s8i    a9, a2, 11                                           
    0x40006708      bnez.n a9, 0x40006718                                       
    0x4000670a      l32i   a9, a2, 12                                           
    0x4000670d      movi   a10, -1                                              
    0x40006710      xor    a9, a10, a9                                          
    0x40006713      and    a8, a9, a8                                           
    0x40006716      s32i.n a8, a3, 0                         





    // check translate.c for simcall..
    qemu_log_mask(LOG_GUEST_ERROR, "SIMCALL but semihosting is disabled\n");
    //gen_exception_cause(dc, ILLEGAL_INSTRUCTION_CAUSE);
// We get, this printout..
io read 00000038

// We endup here. Dead end 
   <0x40000280      wsr.excsave6   a2                                                                                                                 <
   <0x40000283      movi.n a2, -8                                                                                                                     <
   <0x40000285      simcall                                                                                                                           <
  ><0x40000288      j      0x40000288  


```

from components/freertos/include/freertos/xtensa_context.h

```
/*
 Macro to get the current core ID. Only uses the reg given as an argument.
 Reading PRID on the ESP108 architecture gives us 0xCDCD on the PRO processor
 and 0xABAB on the APP CPU. We distinguish between the two by simply checking 
 bit 1: it's 1 on the APP and 0 on the PRO processor.
*/

#ifdef __ASSEMBLER__
	.macro getcoreid reg
	rsr.prid \reg
	bbci \reg,1,1f
	movi \reg,1
	j 2f
1:
	movi \reg,0
2:
	.endm
#endif
```






If running from the loaded elf, we get:
x400807e0 <call_start_cpu0>    entry  a1, 64                                                                   
    0x400807e3 <call_start_cpu0+3>  l32r   a2, 0x4008041c                                                           
    0x400807e6 <call_start_cpu0+6>  memw                                                                            
    0x400807e9 <call_start_cpu0+9>  l32i.n a9, a2, 0                                                                
    0x400807eb <call_start_cpu0+11> movi   a8, 0xfffffbff                                                           
    0x400807ee <call_start_cpu0+14> and    a8, a9, a8                                                               
    0x400807f1 <call_start_cpu0+17> memw                                                                            
    0x400807f4 <call_start_cpu0+20> s32i.n a8, a2, 0                                                                
    0x400807f6 <call_start_cpu0+22> l32r   a2, 0x40080420                                                           
    0x400807f9 <call_start_cpu0+25> memw                                                                            
    0x400807fc <call_start_cpu0+28> l32i.n a9, a2, 0                                                                
    0x400807fe <call_start_cpu0+30> l32r   a8, 0x40080424                                                           
    0x40080801 <call_start_cpu0+33> and    a8, a9, a8                                                               
    0x40080804 <call_start_cpu0+36> memw                                                                            
    0x40080807 <call_start_cpu0+39> s32i.n a8, a2, 0                                                                
  > 0x40080809 <call_start_cpu0+41> mov.n  a10, a1                                                                  
    0x4008080b <call_start_cpu0+43> l32r   a11, 0x40080428                                                          
    0x4008080e <call_start_cpu0+46> movi.n a12, 20                                                                  
    0x40080810 <call_start_cpu0+48> l32r   a8, 0x4008044c     //    a8     0x4000c2c8,memcpy       1073791688                                                    
    0x40080813 <call_start_cpu0+51> callx8 a8                        



  cpu_configure_region_protection () at /home/olas/esp/esp-idf/components/esp32/include/soc/cpu.h:61


## memcpy.
Currently we get an exception here, probably as we are missing qme memory somewhere..
```
    0x4000c2c8      entry  a1, 16                                                                                   
    0x4000c2cb      or     a5, a2, a2                                                                               
    0x4000c2ce      bbsi   a2, 0, 0x4000c298       // Branch if bit set
    0x4000c2d1      bbsi   a2, 1, 0x4000c2ac    
    0x4000c2d4      srli   a7, a4, 4                                                                                
    0x4000c2d7      slli   a8, a3, 30                                                                               
    0x4000c2da      bnez   a8, 0x4000c338                                                                           
    0x4000c2dd      loopnez        a7, 0x4000c2f6                                                                   
    0x4000c2e0      l32i.n a6, a3, 0                                                                                
    0x4000c2e2      l32i.n a7, a3, 4                                                                                
    0x4000c2e4      s32i.n a6, a5, 0   
    0x4000c2e6      l32i.n a6, a3, 8                                                                                
    0x4000c2e8      s32i.n a7, a5, 4                                                                                
    0x4000c2ea      l32i.n a7, a3, 12                                                                               
    0x4000c2ec      s32i.n a6, a5, 8                                                                                
    0x4000c2ee      addi   a3, a3, 16                                                                                
    0x4000c2f1      s32i.n a7, a5, 12                                                                               
    0x4000c2f3      addi   a5, a5, 16                                                                               
    0x4000c2f6      bbci   a4, 3, 0x4000c305                   
    0x4000c2f9      l32i.n a6, a3, 0                                                                                
    0x4000c2fb      l32i.n a7, a3, 4                                                                                
    0x4000c2fd      addi.n a3, a3, 8                                                                                
    0x4000c2ff      s32i.n a6, a5, 0                                                                                
    0x4000c301      s32i.n a7, a5, 4                                                                                
    0x4000c303      addi.n a5, a5, 8                                                                                
    0x4000c305      bbsi   a4, 2, 0x4000c310                                                                        
    0x4000c308      bbsi   a4, 1, 0x4000c320                                                                        
    0x4000c30b      bbsi   a4, 0, 0x4000c330                                                                        
    0x4000c30e      retw.n                        // return 
    0x4000c310      l32i.n a6, a3, 0                                                                                
    0x4000c312      addi.n a3, a3, 4                                                                                
    0x4000c314      s32i.n a6, a5, 0                                                                                
    0x4000c316      addi.n a5, a5, 4                                                                                
    0x4000c318      bbsi   a4, 1, 0x4000c320                                                                        
    0x4000c31b      bbsi   a4, 0, 0x4000c330                                                                        
    0x4000c31e      retw.n                        // return
```



#Useful qemu patch but should be done differently.
```
void HELPER(entry)(CPUXtensaState *env, uint32_t pc, uint32_t s, uint32_t imm)
{
    int callinc = (env->sregs[PS] & PS_CALLINC) >> PS_CALLINC_SHIFT;
    /* if (s > 3 || ((env->sregs[PS] & (PS_WOE | PS_EXCM)) ^ PS_WOE) != 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "Illegal entry instruction(pc = %08x), PS = %08x\n s=%d",
                      pc, env->sregs[PS],s);
        HELPER(exception_cause)(env, pc, ILLEGAL_INSTRUCTION_CAUSE);
    } else */ 
    {
```



#Setting up visual studio code to compile
```
Visual studio code works fine for navigating both qemu and esp32 sources
Press ctrl-shift-P 
type tasks, select configure task runner, select other
{
    "version": "0.1.0",
    "command": "make",
    "isShellCommand": true,
    "options": {
        "cwd": "${workspaceRoot}/../qemu_esp32"
    },
    "tasks": [
        {
            "taskName": "Makefile",
            // Make this the default build command.
            "isBuildCommand": true,
            // Show the output window only if unrecognized errors occur.
            "showOutput": "always",
            // No args
            "args": ["all"],
            // Use the standard less compilation problem matcher.
            "problemMatcher": {
                "owner": "cpp",
                "fileLocation": ["relative", "${workspaceRoot}"],
                "pattern": {
                    "regexp": "^(.*):(\\d+):(\\d+):\\s+(warning|error):\\s+(.*)$",
                    "file": 1,
                    "line": 2,
                    "column": 3,
                    "severity": 4,
                    "message": 5
                }
            }
        }
    ]
}

Now go to File->Preferences->Keyboard Shortcuts and add the following key binding for the build task:

// Place your key bindings in this file to overwrite the defaults
[
    { "key": "f8",          "command": "workbench.action.tasks.build" }
]
```


##Applied patch
It mentions that gdb can only access unprivilidge registers.
This patch was applied to make all registers unpriveledged.
When running gdb from within qemu, this was not necessary.

https://github.com/jcmvbkbc/xtensa-toolchain-build/blob/master/fixup-gdb.sh
```
#! /bin/bash -ex
#
# Fix xtensa registers definitions so that gdb can work with QEMU
#

. `dirname "$0"`/config

sed -i $GDB/gdb/xtensa-config.c -e 's/\(XTREG([^,]\+,[^,]\+,[^,]\+,[^,]\+,[^,]\+,[^,]\+,[^,]\+\)/\1 \& ~1/'
```

##Another discared patch
We could probably write an RFDE instruction in the DoubleException isr routine.

PC is saved in the DEPC register.

EXCSAVE ,on isr entry to get scratch
RFDE , on isr return.

The taking of an exceptionon has the following semantics:
```
procedure Exception(cause)
   if (PS.EXCM & NDEPC=1) then
     DEPC ← PC
     nextPC ← DoubleExceptionVector
  elseif PS.EXCM then
     EPC[1] ← PC
     nextPC ← DoubleExceptionVector
  elseif PS.UM then
     EPC[1] ← PC
     nextPC ← UserExceptionVector
  else
     EPC[1] ← PC
     nextPC ← KernelExceptionVector
  endif
EXCCAUSE ← cause
PS.EXCM ← 1
endprocedure Exception
```

### PS register

The PS register contains miscellaneous fields that are grouped together primarily so that
they can be saved and restored easily for interrupts and context switching. Figure 4–8
shows its layout and Table 4–63 describes its fields. Section 5.3.5 “Processor Status
Special Register” describes the fields of this register in greater detail. The processor ini-
tializes these fields on processor reset: PS.INTLEVEL is set to 15, if it exists and
PS.EXCM is set to 1, and the other fields are set to zero.

```
31                      19  18  17  16  15        12 11 8 7 6 5 4    3 0
                                                                      Intevel
                                                                  Excm
                                                               UM 
                                                          RING
                                CALLINC
                              WOE
```


EXCM 
Exception mode 
0 → normal operation
1 → exception mode
Overrides the values of certain other PS fields (Section 4.4.1.4

UM
User vector mode 
0 → kernel vector mode — exceptions do not need to switch stacks
1 → user vector mode — exceptions need to switch stacks
This bit does not affect protection. It is modified by software and affects the vector
used for a general exception.

CALLINC
Call increment 
Set to window increment by CALL instructions. Used by ENTRY to rotate window.

WOE
Window overflow-detection enable
0 → overflow detection disabled
1 → overflow detection enabled
Used to compute the current window overflow enable

###Windowed call mechanism

The register window operation of the {CALL, CALLX}{4,8,12}, ENTRY, and {RETW,RETW.N} instructions are:

```
CALLn/CALLXn
    WindowCheck (2'b00, 2'b00, n)
    PS.CALLINC ← n
    AR[n || 2'b00] ← n || (PC + 3) 29..0

ENTRY s, imm12
     AR[PS.CALLINC || s 1..0 ] ← AR[s] − (0 17 || imm12 || 0 3 )
     WindowBase ← WindowBase + (0 2 || PS.CALLINC)
     WindowStart WindowBase ← 1
```
In the definition of ENTRY above, the AR read and the AR write refer to different registers.
```
RETW/RETW.N
    n ← AR[0] 31..30
    nextPC ← PC 31..30 || AR[0] 29..0
    owb ← WindowBase
    m ← if WindowStart WindowBase-4’b0001 then 2’b01
    elsif WindowStart  WindowBase-4’b0010 then 2’b10
    elsif WindowStart  WindowBase-4’b0011 then 2’b11
    else 2’b00
  if n = 2’b00 | (m ≠ 2’b00 & m ≠ n) | PS.WOE=0 | PS.EXCM=1 then
     -- undefined operation
    -- may raise illegal instruction exception
  else
     WindowBase ← WindowBase − (0^2 || n)
     if WindowStart WindowBase ≠ 0 then
       WindowStart owb ← 0
     else
       -- Underflow exception
     PS.EXCM ← 1
     EPC[1] ← PC
     PS.OWB ← owb
    nextPC ← if n = 2'b01 then WindowUnderflow4
             if n = 2'b10 then WindowUnderflow8
             WindowUnderflow12
   endif
 endif
```

The RETW opcode assignment is such that the s and t fields are both zero, so that the
hardware may use either AR[s] or AR[t] in place of AR[0] above. Underflow is de-
tected by the caller’s window’s WindowStart bit being clear (that is, not valid)
### Double exception info

###RFDE

Description

RFDE returns from an exception that went to the double exception vector (that is, an ex-
ception raised while the processor was executing with PS.EXCM set). It is similar to RFE, but PS.EXCM is not cleared, and DEPC, if it exists, is used instead of EPC[1]. RFDE sim-
ply jumps to the exception PC. PS.UM and PS.WOE are left unchanged.

RFDE is a privileged instruction.

Operation
```
   if CRING ≠ 0 then
      Exception (PrivilegedInstructionCause)
   elsif NDEPC=1 then
    nextPC ← DEPC
   else
     nextPC ← EPC[1]
   endif
```


###The Double Exception Program Counter (DEPC) 

The double exception program counter (DEPC) register contains the virtual byte ad-
dress of the instruction that caused the most recent double exception. A double excep-
tion is one that is raised when PS.EXCM is set. This instruction has not been executed.

Many double exceptions cannot be restarted, but those that can may be restarted at this address by using an RFDE instruction after fixing the cause of the exception.

The DEPC register exists only if the configuration parameter NDEPC=1. 
If DEPC does not exist, the EPC register is used in its place when a double exception is taken and when the RFDE instruction is executed. The consequence is that it is not possible to recover from most double exceptions. NDEPC=1 is required if both the Windowed Register

Option and the MMU Option are configured. DEPC is undefined after processor reset.


### The Exception Save Register (EXCSAVE) 

The exception save register (EXCSAVE[1]) is simply a read/write 32-bit register intend-
ed for saving one AR register in the exception vector software. This register is undefined after processor reset and there are many software reasons its value might change

whenever PS.EXCM is 0.

The Exception Option defines only one exception save register (EXCSAVE[1]). The High-Priority Interrupt Option extends this concept by adding one EXCSAVE register per

high-priority interrupt level (EXCSAVE[2..NLEVEL+NNMI]). 



####Handling of Exceptional Conditions 

Exceptional conditions are handled by saving some state and redirecting execution to one of a set of exception vector locations. 


The three exception vectors that use EXCCAUSE for the primary cause information form a set called the “general vector.” If PS.EXCM is set when one of the exceptional condi-
tions is raised, then the processor is already handling an exceptional condition and the exception goes to the DoubleExceptionVector. Only a few double exceptions are recoverable, including a TLB miss during a register window overflow or underflow ex-
ception. For these, EXCCAUSE (and EXCSAVE in Table 4–66) must be well enough understood not to need duplication. Otherwise (PS.EXCM clear), if PS.UM is set the exception goes to the UserExceptionVector, and if not the exception goes to the

KernelExceptionVector. The Exception Option effectively defines two operating modes: user vector mode and kernel vector mode, controlled by the PS.UM bit. The combination of user vector mode and kernel vector mode is provided so that the user
 vector exception handler can switch to an exception stack before processing the exception, whereas the kernel vector exception handler can continue using the kernel stack. Single or multiple high-priority interrupts can be configured for any hardware prioritized levels 2..6. These will redirect to the InterruptVector[i] where “i” is the level. One of those levels, often the highest one, can be chosen as the debug level and will redirect execution to InterruptVector[d] where “d” is the debug level. The level one higher than the highest high-priority interrupt can be chosen as an NMI, which will redirect execution to InterruptVector[n] where “n” is the NMI level (2..7).

```
The MEMW instruction causes all memory and cache accesses
 (loads, stores, acquires, releases, prefetches, and cache operations, but not instruction fetches) before itself 
 in program order to access memory before all memory and cache accesses (but not instruction fetches) after. 
 At least one MEMW should be executed in between every load or store to a volatile variable. 
The Multiprocessor Synchronization Option provides some additional instructions that also affect memory ordering in a more focused fashion.

MEMW has broader applications than these other instructions (for example, when reading and writing device registers), 
but it also may affect performance more than the synchronization instructions.
```


#Boot of ESP32

When starting up the ESP32 it reads instructions from embedded memory, located at 40000400
(unless this is mapped to ram by DPORT_APP_CACHE_CTRL1_REG & DPORT_PRO_CACHE_CTRL1_REG)  

```
  PROVIDE ( _ResetVector = 0x40000400 );
  #define XCHAL_RESET_VECTOR1_VADDR	0x40000400
```


Eventually after initialisation the first stage bootloader will call call_start_cpu0()

The main tasks of this is:

1. cpu_configure_region_protection(), mmu_init(1); sets up the mmu

2.  boot_cache_redirect( 0, 0x5000 );  /*register boot sector in drom0 page 0 */
    boot from ram at next reset.

3. load_partition_table(), Load partition table and take apropiate action.    

4.  unpack_load_app(&load_part_pos); To be investigated.

## BOOT/Startup in single core mode
```
40080828 <call_start_cpu0>:
 * We arrive here after the bootloader finished loading the program from flash. The hardware is mostly uninitialized,
 * and the app CPU is in reset. We do have a stack, so we can do the initialization in C.
 */

void IRAM_ATTR call_start_cpu0()
{
40080828:	008136        	entry	a1, 64
    //Kill wdt
    REG_CLR_BIT(RTC_CNTL_WDTCONFIG0_REG, RTC_CNTL_WDT_FLASHBOOT_MOD_EN);
4008082b:	ff0121        	l32r	a2, 40080430 <_init_end+0x30>               //  Loads a2 with  0x3ff4808c, RTC_CNTL_WDTCONFIG0_REG
4008082e:	0020c0        	memw
40080831:	0298      	    l32i.n	a9, a2, 0                                   
40080833:	ffab82        	movi	a8, 0xfffffbff                              // Bit(10)  description: enable WDT in flash boo
40080836:	108980        	and	a8, a9, a8
40080839:	0020c0        	memw
4008083c:	0289      	    s32i.n	a8, a2, 0                                   // a2 contains     0x6001f048
    REG_CLR_BIT(0x6001f048, BIT(14)); //DR_REG_BB_BASE+48
4008083e:	fefd21        	l32r	a2, 40080434 <_init_end+0x34>
40080841:	0020c0        	memw
40080844:	0298      	    l32i.n	a9, a2, 0
40080846:	fefc81        	l32r	a8, 40080438 <_init_end+0x38>
40080849:	108980        	and	a8, a9, a8
4008084c:	0020c0        	memw
4008084f:	0289      	s32i.n	a8, a2, 0                                       // a2 still contains 0x6001f048
 * 15 — no access, raise exception
 */



static inline void cpu_configure_region_protection()
{
    const uint32_t pages_to_protect[] = {0x00000000, 0x80000000, 0xa0000000, 0xc0000000, 0xe0000000};
40080851:	01ad      	    mov.n	a10, a1
40080853:	fefab1        	l32r	a11, 4008043c <_init_end+0x3c>
40080856:	4c1c      	    movi.n	a12, 20
40080858:	ff0281        	l32r	a8, 40080460 <_init_end+0x60>
4008085b:	0008e0        	callx8	a8                          // a8 contains 0x4000c2c8, memcpy()...
    for (int i = 0; i < sizeof(pages_to_protect)/sizeof(pages_to_protect[0]); ++i) {
4008085e:	080c      	movi.n	a8, 0
 * See Xtensa ISA Reference manual for explanation of arguments (section 4.6.3.2).
 */

static inline void cpu_write_dtlb(uint32_t vpn, unsigned attr)
{
    asm volatile ("wdtlb  %1, %0; dsync\n" :: "r" (vpn), "r" (attr));
40080860:	fa0c      	movi.n	a10, 15
40080862:	0004c6        	j	40080879 <call_start_cpu0+0x51>

static inline void cpu_configure_region_protection()
{
    const uint32_t pages_to_protect[] = {0x00000000, 0x80000000, 0xa0000000, 0xc0000000, 0xe0000000};
    for (int i = 0; i < sizeof(pages_to_protect)/sizeof(pages_to_protect[0]); ++i) {
        cpu_write_dtlb(pages_to_protect[i], 0xf);
40080865:	a09810        	addx4	a9, a8, a1
40080868:	0998      	l32i.n	a9, a9, 0
 * See Xtensa ISA Reference manual for explanation of arguments (section 4.6.3.2).
 */

static inline void cpu_write_dtlb(uint32_t vpn, unsigned attr)
{
    asm volatile ("wdtlb  %1, %0; dsync\n" :: "r" (vpn), "r" (attr));
4008086a:	50e9a0        	wdtlb	a10, a9
4008086d:	002030        	dsync
}


static inline void cpu_write_itlb(unsigned vpn, unsigned attr)
{
    asm volatile ("witlb  %1, %0; isync\n" :: "r" (vpn), "r" (attr));
40080870:	5069a0        	witlb	a10, a9
40080873:	002000        	isync
 */

static inline void cpu_configure_region_protection()
{
    const uint32_t pages_to_protect[] = {0x00000000, 0x80000000, 0xa0000000, 0xc0000000, 0xe0000000};
    for (int i = 0; i < sizeof(pages_to_protect)/sizeof(pages_to_protect[0]); ++i) {
40080876:	01c882        	addi	a8, a8, 1
40080879:	e858b6        	bltui	a8, 5, 40080865 <call_start_cpu0+0x3d>
 * See Xtensa ISA Reference manual for explanation of arguments (section 4.6.3.2).
 */

static inline void cpu_write_dtlb(uint32_t vpn, unsigned attr)
{
    asm volatile ("wdtlb  %1, %0; dsync\n" :: "r" (vpn), "r" (attr));
4008087c:	080c      	movi.n	a8, 0
4008087e:	fef021        	l32r	a2, 40080440 <_init_end+0x40>
40080881:	50e280        	wdtlb	a8, a2
40080884:	002030        	dsync
}


static inline void cpu_write_itlb(unsigned vpn, unsigned attr)
{
    asm volatile ("witlb  %1, %0; isync\n" :: "r" (vpn), "r" (attr));
40080887:	feee21        	l32r	a2, 40080440 <_init_end+0x40>
4008088a:	506280        	witlb	a8, a2
4008088d:	002000        	isync

    cpu_configure_region_protection();

    //Move exception vectors to IRAM
    asm volatile (\
40080890:	feed21        	l32r	a2, 40080444 <_init_end+0x44>
40080893:	13e720        	wsr.vecbase	a2
                  "wsr    %0, vecbase\n" \
                  ::"r"(&_init_start));

    uartAttach();
40080896:	fef381        	l32r	a8, 40080464 <_init_end+0x64>
40080899:	0008e0        	callx8	a8
    ets_install_uart_printf();
4008089c:	fef381        	l32r	a8, 40080468 <_init_end+0x68>
4008089f:	0008e0        	callx8	a8

    memset(&_bss_start, 0, (&_bss_end - &_bss_start) * sizeof(_bss_start));
400808a2:	feea21        	l32r	a2, 4008044c <_init_end+0x4c>
400808a5:	02ad      	    mov.n	a10, a2
400808a7:	0b0c      	    movi.n	a11, 0
400808a9:	fee7c1        	l32r	a12, 40080448 <_init_end+0x48>
400808ac:	c0cc20        	sub	a12, a12, a2
400808af:	feef81        	l32r	a8, 4008046c <_init_end+0x6c>
400808b2:	0008e0        	callx8	a8

    // Initialize heap allocator
    heap_alloc_caps_init();
400808b5:	feee81        	l32r	a8, 40080470 <_init_end+0x70>
400808b8:	0008e0        	callx8	a8

    ESP_EARLY_LOGI(TAG, "Pro cpu up.");
400808bb:	008f65        	call8	400811b0 <esp_log_timestamp>
400808be:	0abd      	    mov.n	a11, a10
400808c0:	fed421        	l32r	a2, 40080410 <_init_end+0x10>
400808c3:	fee3a1        	l32r	a10, 40080450 <_init_end+0x50>
400808c6:	02cd      	    mov.n	a12, a2
400808c8:	feeb81        	l32r	a8, 40080474 <_init_end+0x74>
400808cb:	0008e0        	callx8	a8

    while (!app_cpu_started) {
        ets_delay_us(100);
    }
#else
    ESP_EARLY_LOGI(TAG, "Single core mode");
400808ce:	008e25        	call8	400811b0 <esp_log_timestamp>
400808d1:	0abd      	    mov.n	a11, a10
400808d3:	fee0a1        	l32r	a10, 40080454 <_init_end+0x54>
400808d6:	02cd      	    mov.n	a12, a2
400808d8:	fee781        	l32r	a8, 40080474 <_init_end+0x74>
400808db:	0008e0        	callx8	a8
    CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);
400808de:	fede81        	l32r	a8, 40080458 <_init_end+0x58>
400808e1:	0020c0        	memw
400808e4:	08a8      	    l32i.n	a10, a8, 0
400808e6:	e97c      	    movi.n	a9, -2
400808e8:	109a90        	and	a9, a10, a9
400808eb:	0020c0        	memw
400808ee:	0899      	    s32i.n	a9, a8, 0
#endif
    ESP_EARLY_LOGI(TAG, "Pro cpu start user code");
400808f0:	008be5        	call8	400811b0 <esp_log_timestamp>
400808f3:	0abd      	    mov.n	a11, a10
400808f5:	fed9a1        	l32r	a10, 4008045c <_init_end+0x5c>
400808f8:	02cd      	    mov.n	a12, a2
400808fa:	fede81        	l32r	a8, 40080474 <_init_end+0x74>
400808fd:	0008e0        	callx8	a8
    start_cpu0();
40080900:	ffeca5        	call8	400807cc <start_cpu0_default>
40080903:	f01d      	retw.n
40080905:	000000        	ill

40080908 <ipc_task>:
    s_ipc_wait = IPC_WAIT_FOR_START;
    xSemaphoreGive(s_ipc_sem[cpu_id]);
    xSemaphoreTake(s_ipc_ack, portMAX_DELAY);
    xSemaphoreGive(s_ipc_mutex);
    return ESP_OK;
}
40080908:	004136        	entry	a1, 32
4008090b:	02c725        	call8	4008357c <xPortGetCoreID>
4008090e:	101a27        	beq	a10, a2, 40080922 <ipc_task+0x1a>
40080911:	fed9a1        	l32r	a10, 40080478 <_init_end+0x78>
40080914:	fb2c      	movi.n	a11, 47
40080916:	fed9c1        	l32r	a12, 4008047c <_init_end+0x7c>
40080919:	fed9d1        	l32r	a13, 40080480 <_init_end+0x80>
4008091c:	fedf81        	l32r	a8, 40080498 <_init_end+0x98>
4008091f:	0008e0        	callx8	a8
40080922:	fed831        	l32r	a3, 40080484 <_init_end+0x84>
40080925:	a02230        	addx4	a2, a2, a3
40080928:	fed871        	l32r	a7, 40080488 <_init_end+0x88>
4008092b:	fed861        	l32r	a6, 4008048c <_init_end+0x8c>
4008092e:	fed851        	l32r	a5, 40080490 <_init_end+0x90>
40080931:	02a8      	l32i.n	a10, a2, 0
40080933:	0b0c      	movi.n	a11, 0
40080935:	fc7c      	movi.n	a12, -1
40080937:	0bdd      	mov.n	a13, a11
40080939:	0155a5        	call8	40081e94 <xQueueGenericReceive>
4008093c:	051a26        	beqi	a10, 1, 40080945 <ipc_task+0x3d>
4008093f:	fed781        	l32r	a8, 4008049c <_init_end+0x9c>
40080942:	0008e0        	callx8	a8
40080945:	0020c0        	memw
40080948:	0738      	l32i.n	a3, a7, 0
4008094a:	0020c0        	memw
4008094d:	0648      	l32i.n	a4, a6, 0
4008094f:	0020c0        	memw
40080952:	0588      	l32i.n	a8, a5, 0
40080954:	d8cc      	bnez.n	a8, 40080965 <ipc_task+0x5d>
40080956:	fecf81        	l32r	a8, 40080494 <_init_end+0x94>
40080959:	08a8      	l32i.n	a10, a8, 0
4008095b:	0b0c      	movi.n	a11, 0
4008095d:	20cbb0        	or	a12, a11, a11
40080960:	0bdd      	mov.n	a13, a11
40080962:	0116e5        	call8	40081ad0 <xQueueGenericSend>
40080965:	04ad      	mov.n	a10, a4
40080967:	0003e0        	callx8	a3
4008096a:	fec931        	l32r	a3, 40080490 <_init_end+0x90>
4008096d:	0020c0        	memw
40080970:	0338      	l32i.n	a3, a3, 0
40080972:	bb1366        	bnei	a3, 1, 40080931 <ipc_task+0x29>
40080975:	fec731        	l32r	a3, 40080494 <_init_end+0x94>
40080978:	03a8      	l32i.n	a10, a3, 0
4008097a:	0b0c      	movi.n	a11, 0
4008097c:	0bcd      	mov.n	a12, a11
4008097e:	20dbb0        	or	a13, a11, a11
40080981:	0114e5        	call8	40081ad0 <xQueueGenericSend>
40080984:	ffea46        	j	40080931 <ipc_task+0x29>
	...

40080988 <lock_init_generic>:
static portMUX_TYPE lock_init_spinlock = portMUX_INITIALIZER_UNLOCKED;

/* Initialise the given lock by allocating a new mutex semaphore
   as the _lock_t value.
*/
static void IRAM_ATTR lock_init_generic(_lock_t *lock, uint8_t mutex_type) {
40080988:	004136        	entry	a1, 32
    portENTER_CRITICAL(&lock_init_spinlock);
4008098b:	fec5a1        	l32r	a10, 400804a0 <_init_end+0xa0>
4008098e:	01cba5        	call8	40082648 <vTaskEnterCritical>
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
40080991:	01c865        	call8	40082618 <xTaskGetSchedulerState>
40080994:	0e1a66        	bnei	a10, 1, 400809a6 <lock_init_generic+0x1e>
        /* nothing to do until the scheduler is running */
        *lock = 0; /* ensure lock is zeroed out, in case it's an automatic variable */
40080997:	00a032        	movi	a3, 0
4008099a:	0239      	s32i.n	a3, a2, 0
        portEXIT_CRITICAL(&lock_init_spinlock);
4008099c:	fec1a1        	l32r	a10, 400804a0 <_init_end+0xa0>
4008099f:	01d165        	call8	400826b4 <vTaskExitCritical>
        return;
400809a2:	f01d      	retw.n
400809a4:	820000        	mull	a0, a0, a0
    }

    if (*lock) {
400809a7:	560022        	l8ui	a2, a0, 86
400809aa:	00f8      	l32i.n	a15, a0, 0
           implements these as macros instead of inline functions
           (*party like it's 1998!*) it's not possible to do this
           without writing wrappers. Doing it this way seems much less
           spaghetti-like.
        */
        xSemaphoreHandle new_sem = xQueueCreateMutex(mutex_type);
400809ac:	20a330        	or	a10, a3, a3
400809af:	0129a5        	call8	40081c48 <xQueueCreateMutex>
        if (!new_sem) {
400809b2:	4acc      	bnez.n	a10, 400809ba <lock_init_generic+0x32>
            abort(); /* No more semaphores available or OOM */
400809b4:	feba81        	l32r	a8, 4008049c <_init_end+0x9c>
400809b7:	0008e0        	callx8	a8
        }
        *lock = (_lock_t)new_sem;
400809ba:	02a9      	s32i.n	a10, a2, 0
    }
    portEXIT_CRITICAL(&lock_init_spinlock);
400809bc:	feb9a1        	l32r	a10, 400804a0 <_init_end+0xa0>
400809bf:	01cf65        	call8	400826b4 <vTaskExitCritical>
400809c2:	f01d      	retw.n

400809c4 <_lock_init>:
}

void IRAM_ATTR _lock_init(_lock_t *lock) {
400809c4:	004136        	entry	a1, 32
    lock_init_generic(lock, queueQUEUE_TYPE_MUTEX);
400809c7:	02ad      	mov.n	a10, a2
400809c9:	1b0c      	movi.n	a11, 1
400809cb:	fffbe5        	call8	40080988 <lock_init_generic>
400809ce:	f01d      	retw.n

400809d0 <_lock_init_recursive>:
}

void IRAM_ATTR _lock_init_recursive(_lock_t *lock) {
400809d0:	004136        	entry	a1, 32
    lock_init_generic(lock, queueQUEUE_TYPE_RECURSIVE_MUTEX);
400809d3:	02ad      	mov.n	a10, a2
400809d5:	4b0c      	movi.n	a11, 4
400809d7:	fffb25        	call8	40080988 <lock_init_generic>
400809da:	f01d      	retw.n

400809dc <_lock_close>:

   Note that FreeRTOS doesn't account for deleting mutexes while they
   are held, and neither do we... so take care not to delete newlib
   locks while they may be held by other tasks!
*/
void IRAM_ATTR _lock_close(_lock_t *lock) {
400809dc:	004136        	entry	a1, 32
    portENTER_CRITICAL(&lock_init_spinlock);
400809df:	feb0a1        	l32r	a10, 400804a0 <_init_end+0xa0>
400809e2:	01c665        	call8	40082648 <vTaskEnterCritical>
    if (*lock) {
400809e5:	002232        	l32i	a3, a2, 0
400809e8:	027316        	beqz	a3, 40080a13 <_lock_close+0x37>
        xSemaphoreHandle h = (xSemaphoreHandle)(*lock);
#if (INCLUDE_xSemaphoreGetMutexHolder == 1)
        configASSERT(xSemaphoreGetMutexHolder(h) == NULL); /* mutex should not be held */
400809eb:	03ad      	mov.n	a10, a3
400809ed:	010be5        	call8	40081aac <xQueueGetMutexHolder>
400809f0:	6a9c      	beqz.n	a10, 40080a0a <_lock_close+0x2e>
400809f2:	feaca1        	l32r	a10, 400804a4 <_init_end+0xa4>
400809f5:	feacb1        	l32r	a11, 400804a8 <_init_end+0xa8>
400809f8:	09a1c2        	movi	a12, 0x109
400809fb:	feacd1        	l32r	a13, 400804ac <_init_end+0xac>
400809fe:	fe9d81        	l32r	a8, 40080474 <_init_end+0x74>
40080a01:	0008e0        	callx8	a8
40080a04:	fea681        	l32r	a8, 4008049c <_init_end+0x9c>
40080a07:	0008e0        	callx8	a8
#endif
        vSemaphoreDelete(h);
40080a0a:	03ad      	mov.n	a10, a3
40080a0c:	017125        	call8	40082120 <vQueueDelete>
        *lock = 0;
40080a0f:	030c      	movi.n	a3, 0
40080a11:	0239      	s32i.n	a3, a2, 0
    }
    portEXIT_CRITICAL(&lock_init_spinlock);
40080a13:	fea3a1        	l32r	a10, 400804a0 <_init_end+0xa0>
40080a16:	01c9e5        	call8	400826b4 <vTaskExitCritical>
40080a19:	f01d      	retw.n
	...

40080a1c <lock_acquire_generic>:
}
```



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

start_cpu0_default() is also an important function, it prepares for start of the applicaton.
Depending on how the application is configured. In this function app_main() is called.

```
  xTaskCreatePinnedToCore(&main_task, "main",...)


static void main_task(void* args)
{
    app_main();
    vTaskDelete(NULL);
}
```


```
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


```
// Is this correct?? _init_start should be interesting.
void IRAM_ATTR call_start_cpu1()
{
    asm volatile (\
                  "wsr    %0, vecbase\n" \
                  ::"r"(&_init_start));

    ...
}
```






#The ESP32 memory layout


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


When compiling the application an elf file, bin file and map file is generated. this is not all that should be loaded loaded. Also partitiontable and the bootloader should be loaded.

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


###Tests with loader scripts 


I tried to add a new section in loader script to place a function at reset vector,

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


When compileing tike this,

  make VERBOSE=1 
  
  We see how the linker is called. Then replace the -T esp32.common.ld with your modified ld file.

However when we try this we get,
  In function `boot_func':
  main.c:13:(.boot+0x3): dangerous relocation: l32r: literal target out of range (try using text-section-literals): (*UND*+0x26395a4)

This turned out to not be necessary. qemu inserts a jump instruction after loading the elf file (esp32.c)
entry_point is the adress ot the entry point received by load_elf()

```
        static const uint8_t jx_a0[] = {
            0xa0, 0, 0,
        };
        // a0
        env->regs[0] = entry_point;


        cpu_physical_memory_write(env->pc, jx_a0, sizeof(jx_a0));
```



# Calling convention 


The  Xtensa  windowed  register  calling  convention  is  designed  to  efficiently  pass  arguments  and  return  values  in  
AR registers  

The  register  windows  for  the caller and the callee are not the same, but they partially overlap. As many as six words
of arguments can be passed from the caller to the callee in these overlapping registers, and as many as four words of a return value can be returned in the same registers. If all
the arguments do not fit in registers, the rest are passed on the stack. Similarly, if the return value needs more than four words, the value is returned on the stack instead of the AR  registers.

The Windowed Register Option replaces the simple 16-entry AR register file with a larger register file from which a window of 16 entries is visible at any given time. The window
is rotated on subroutine entry and exit, automatically saving and restoring some registers. When the window is rotated far enough to require registers to be saved to or re-
stored from the program stack, an exception is raised to move some of the register values between the register file and the program stack. The option reduces code size and
increases performance of programs by eliminating register saves and restores at procedure entry and exit, and by reducing argument-shuffling at calls. It allows more local
variables to live permanently in registers, reducing the need for stack-frame maintenance in non-leaf routines.
Xtensa ISA register windows are different from register windows in other instruction sets. Xtensa register increments are 4, 8, and 12 on a per-call basis, not a fixed incre-
ment as in other instruction sets. Also, Xtensa processors have no global address registers. The caller specifies the increment amount, while the callee performs the actual in-
crement by the ENTRY instruction. 

The compiler uses an increment sufficient to hide the registers that are live at the point of the call (which the compiler can pack into the fewest
possible at the low end of the register-number space). The number of physical registers is 32 or 64, which makes this a more economical configuration. Sixteen registers are vis-
ible at one time. Assuming that the average number of live registers at the point of call is

6.5 (return address, stack pointer, and 4.5 local variables), and that the last routine uses
12 registers at its peak, this allows nine call levels to live in 64 registers (8×6.5+12=64).
As an example, an average of 6.5 live registers might represent 50% of the calls using
an increment of 4, 38% using an increment of 8, and 12% using an increment of 12.


The rotation of the 16-entry visible window within the larger register file is controlled by the WindowBase Special Register added by the option. The rotation always occurs in
units of four registers, causing the number of bits in WindowBase to be log 2 (NAREG/4). Rotation at the time of a call can instantly save some registers and provide new regis-
ters for the called routine. Each saved register has a reserved location on the stack, to which it may be saved if the call stack extends enough farther to need to re-use the
physical registers. The WindowStart Special Register, which is also added by the option and consists of NAREG/4 bits, indicates which four register units are currently cached in
the physical register file instead of residing in their stack locations. An attempt to use registers live with values from a parent routine raises an Overflow Exception which saves those values and frees the registers for use. A return to a calling routine whose
registers have been previously saved to the stack raises an Underflow Exception which restores those values. Programs without wide swings in the depth of the call stack save and restore values only occasionally.

### General-Purpose Register Use
```
Register       Use
a0             Return Address
a1 (sp)        Stack Pointer
a2 – a7        Incoming Arguments
a7             Optional Frame Point
```


```
int proc1 (int x, short y, char z) {
// a2 = x
// a3 = y
// a4 = z
// return value goes in a2
}
```

The registers that the caller uses for arguments and return values are determined by the
size  of  the  register  window.  The  window  size  must  be  added  to  the  register  numbers
seen by the callee. For example, if the caller uses a  CALL8  instruction, the window size is 8 and “

```
x = proc1 (1, 2, 3)
translates to:
movi.n    a10, 1
movi.n    a11, 2
movi.n    a12, 3
call8     proc1
s32i      a10, sp, x_offset
```

Double-precision floating-point values and  long long  integers require two registers for storage. The calling convention requires that these registers be even/odd register pairs.
```
double proc2 (int x, long long y) {
// a2 = x
// a3 = unused
// a4,a5 = y
// return value goes in a2/a3
}
```

##
Stack Frame Layout
The  stack  grows  down  from  high  to  low  addresses.  The  stack  pointer  must  always  be
aligned to a 16-byte boundary. Every stack frame contains at least 16 bytes of register
window save area. If a function call has arguments that do not fit in registers, they are
passed  on  the  stack  beginning  at  the  stack-pointer  address. 


## Exception-Processing States 
The Xtensa LX processor implements basic functions needed to manage both 
synchronous exceptions  and  asynchronous exceptions  (interrupts). However, the baseline pro-
cessor configuration only supports synchronous exceptions, for example, those caused
by illegal instructions, system calls, instruction-fetch errors, and load/store errors. Asyn-
chronous exceptions are supported by processor configurations built with the interrupt,
high-priority interrupt, and timer options.
The Xtensa LX interrupt option implements Level-1 interrupts, which are asynchronous
exceptions  triggered  by  processor  input  signals  or  software  exceptions.  Level-1  interrupts have the lowest priority of all interrupts and are handled differently than high-prior-
ity  interrupts,  which  occur  at  priority  levels  2  through  15.  The  interrupt  option  is  a
prerequisite for the high-priority-interrupt, peripheral-timer, and debug options. 

The  high-priority-interrupt  option  implements  a  configurable  number  of  interrupt  levels
(2 through  15).  In  addition,  an  optional  non-maskable  interrupt  (NMI)  has  an  implicit
infinite  priority  level.  Like  Level-1  interrupts,  high-priority  interrupts  can  be  external  or
software  exceptions.  Unlike  Level-1  interrupts,  each  high-priority  interrupt  level  has  its
own interrupt vector and dedicated registers for saving processor state. These separate
interrupt vectors and special registers permit the creation of very efficient handler mech-
anisms  that  may  need  only  a  few  lines  of  assembler  code,  avoiding  the  need  for  a
subroutine call.

###Exception Architecture
The Xtensa LX processor supports one exception model, Xtensa Exception Architecture 2 (XEA2).
XEA2 Exceptions XEA2 exceptions share the use of two exception vectors. These two vector addresses,
UserExceptionVector  and   KernelExceptionVector are  both  configuration  options. The exception-handling process saves the address of the instruction causing the
exception into special register  EPC[1]  and the cause code for the exception into special
register  EXCCAUSE

Interrupts and exceptions are precise, so that on returning from the exception  handler,  program  execution  can  continue  exactly  where  it  left  off.  
(Note:  Of necessity, write bus-error exceptions are not precise.)

###Unaligned Exception
This option allows an exception to be raised upon any unaligned memory access wheth-
er it is generated by core processor memory instructions, by optional instructions, or by
designer-defined TIE instructions. With the cooperation of system software, occasional unaligned accesses can be handled correctly.
Note  that  instructions  that  deal  with  the  cache  lines  such  as  prefetch  and  cache-
management instructions will not cause unaligned exceptions.

###Interrupt
The Xtensa LX exception option implements Level-1 interrupts. The processor configu-
ration process allows for a variable number of interrupts to be defined. External interrupt
inputs  can  be  level-sensitive  or  edge-triggered.  The  processor  can  test  the  state of these interrupt inputs at any time by reading the 
INTERRUPT register. An arbitrary number of software interrupts (up to a total of 32 for all types of interrupts) that are not tied to an   external   signal   can   also   be   configured.   These   interrupts   use   the   general
exception/interrupt  handler.  Software  can  then  manipulate  the  bits  of  interrupt  enable fields   to   prioritize   the   exceptions.   The   processor   accepts   an   interrupt   when   an
interrupt   signal  is  asserted  and  the  proper  interrupt  enable  bits  are  set  in  the

INTENABLE
 register and in the 
INTLEVEL
 field of the PS register.


###High-Priority Interrupt Option
A configured Xtensa LX processor can have as many as 6 level-sensitive or edge-trig-
gered  high-priority  interrupts.  One  NMI  or  non-maskable  interrupt  can  also  be  config-
ured.  The  processor  can  have  as  many  as  six  high-priority  interrupt  levels  and  each
high-priority interrupt level has its own interrupt vector. Each high-priority interrupt level
also has its own dedicated set of three special registers (EPC, EPS, and EXCSAVE) used
to save the processor state.


###Timer Interrupt Option
The  Timer  Interrupt  Option  creates  one  to  three  hardware  timers  that  can  be  used  to
generate periodic interrupts. The timers can be configured to generate either level-one
or high-level interrupts. Software manages the timers through one special register that
contains  the  processor  core’s  current  clock  count  (which  increments  each  clock  cycle)
and  additional  special  registers  (one  for  each  of  the  three  optional  timers)  for  setting
match-count values that determine when the next timer interrupt


### Inter processor instructions

L32AI       RRI8
Load 32-bit Acquire (8-bit shifted offset)
This non-speculative load will perform before any subsequent loads, stores, or 
acquires are performed. It is typically used to test the synchronization variable 
protecting a mutual-exclusion region (for example, to acquire a lock)

S32RI       RRI8
Store 32-bit Release (8-bit shifted offset)
All prior loads, stores, acquires, and releases will be performed before this 
store is performed. It is typically used to write a synchronization variable to 
indicate that this processor is no longer in a mutual-exclusion region (for 
example, to release a lock).


## Xtensa ESP8266
Xtensa ESP8266 calling convention, not same as ESP32? ESP uses Windowed register ABI. 


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
