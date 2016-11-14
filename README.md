 qemu_esp32
Add tensilica esp32 cpu and a board to qemu and dump the rom to learn more about esp-idf
###ESP32 in QEMU.

This documents how to add an esp32 cpu and a simple esp32 board to qemu in order to run an app compiled with the SDK in QEMU. Esp32 is a 240 MHz dual core Tensilica LX6 microcontroller.
It is a good way to learn about qemu ,esp32 and the esp32 rom.





By following the instructions here, I added esp32 to qemu.
http://wiki.linux-xtensa.org/index.php/Xtensa_on_QEMU

prerequisites Debian/Ubuntu:
```
sudo apt-get install libpixman-1-0 libpixman-1-dev 
```


## qemu-esp32

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

#Dumping the ROM with the main/main.c program
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
When finished trim the capturefile (remove all before and after the dump data) and call it, test.log
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
  > xtensa-softmmu/qemu-system-xtensa -d guest_errors,mmu,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S > io.txt

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
  (gdb) set $pc=call_start_cpu0
```


```
  (gdb) x/20i $pc
  (gdb) p/x $a3          (register as hex)

```

#Objdump
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
   0:	004136          entry	a1, 32
   3:	003200          rfde
   6:	f01d            retw.n

Disassembly of section .text.test:

00000000 <test>:
   0:	004136      entry	a1, 32
   3:	221b      	addi.n	a2, a2, 1
   5:	f01d      	retw.n

Disassembly of section .text.jump:

00000000 <jump>:
   0:	004136        	entry	a1, 32
   3:	0000a0        	jx	a0
   6:	000081        	l32r	a8, fffc0008 <jump+0xfffc0008>
   9:	0008e0          callx8	a8
   c:	f01d            retw.n
```


#Results
I got my ESP32-dev board from Adafruit. I have made two dumps and mapped the dumps into the files rom.bin & rom1.bin
The code in esp32.c also patches the function ets_unpack_flash_code.
Instead of loading the code it sets the user_code_start variable (0x3ffe0400) This will later be used
by the rom to start the loaded elf file. The functions SPIEraseSector & SPIWrite are also patched to return 0.
The rom function -- ets_unpack_flash_code is located at 0x40007018.

The instruction wsr.memctl does not work in QEMU but can be patched like this (in qemu tree).
File translate.c
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


Better flash emulation would be better. The serial device should also be emulated differently. Now serial output goes to stderr.

To get proper serial emulation you can remove the comment around this in the file esp32.c,
```
//  serial_mm_init(system_io, 0x40000 , 2, xtensa_get_extint(env, 0),
//            115200, serial_hds[0], DEVICE_LITTLE_ENDIAN);
```
However it does not always work. You can use qemu to generate reset and it might work sometimes.

```
xtensa-softmmu/qemu-system-xtensa -d guest_errors,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf  -S -s > io.txt

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

The WUR and RUR instructions are specific for the ESP32.


When configuring, choose
  Component config  --->   
    FreeRTOS  --->
      [*] Run FreeRTOS only on first core 


```


##Before the rom patches
Before the rom patches we got this
```
xtensa-softmmu/qemu-system-xtensa -d guest_errors,int,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf    > io.txt 
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
flash read err, 1000
Falling back to built-in command interpreter.
```
The command interpreter is Basic, Here you can read about it 
    http://hackaday.com/2016/10/27/basic-interpreter-hidden-in-esp32-silicon/



#What is the problem with this code
Some i/o register name mapping in esp32.cis probably wrong.  The values returned are also many times wrong.
I did this mapping very quickly with grep to get a better understanding of what the rom was doing.
```
Terminal 1
>xtensa-softmmu/qemu-system-xtensa -d guest_errors,int,mmu,page,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf   -S -s

Terminal 2
>xtensa-esp32-elf-gdb build/app-template.elf
(gdb) target remote:1234

//Uart_Init, 0x40009120
(gdb) b *0x40009120
(gdb) layout asm

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



```




Interesting info. To be investigated. 
```
static HeapRegionTagged_t regions[]={
	{ (uint8_t *)0x3F800000, 0x20000, 15, 0}, //SPI SRAM, if available
	{ (uint8_t *)0x3FFAE000, 0x2000, 0, 0}, //pool 16 <- used for rom code
	{ (uint8_t *)0x3FFB0000, 0x8000, 0, 0}, //pool 15 <- can be used for BT
	{ (uint8_t *)0x3FFB8000, 0x8000, 0, 0}, //pool 14 <- can be used for BT
```

#Adding flash qemu emulation.
```
head -c 16M /dev/zero > esp32.flash

xtensa-softmmu/qemu-system-xtensa -d guest_errors,page,unimp  -cpu esp32 -M esp32 -m 4M -pflash esp32.flash -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S  > io.txt

This does not work at all. I dont understand qemu enough to get this working.
To add flash checkout line 502 in esp32.c
dinfo = drive_get(IF_PFLASH, 0, 0);
...
Also checkout m25p80.c driver in qemu

Dump with od,
od -t x4  partitions_singleapp.bin

xtensa-softmmu/qemu-system-xtensa -d guest_errors,page,unimp  -cpu esp32 -M esp32 -m 4M   -kernel  ~/esp/qemu_esp32/build/app-template.elf    > io.txt

```


Running two cores, most likely will not work so so well. But shows up in  (gdb) info threads
```
xtensa-softmmu/qemu-system-xtensa -d guest_errors,page,unimp  -cpu esp32 -M esp32 -m 4M -smp 2 -pflash esp32.flash -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S  > io.txt
I think that the app CPU quickly gets stuck in a loop. Also the WUR 234-236 & RUR 234-236 has something to do with processor syncronisation.
This will create problems when running 2 cores.
```

## Detailed boot analaysis
[More about the boot of the romdumps](./BOOT.md)


##Patched gdb,
In the bin directory there is an improved gdb version. The following patch was applied,
https://github.com/jcmvbkbc/xtensa-toolchain-build/blob/master/fixup-gdb.sh
To use it properly you must increase the num_regs = 104 to i.e. 211 in core-esp32.c



## Another version of qemu for xtensa
As I am not alwas sure of what I am doing, I would recomend this version of the software. Currently it is not yet finnished (11//11) but will most certainly be better if Max finds the time to work on it.

#A better version of qemu with esp32 exists here,
   
    git clone https://github.com/OSLL/qemu-xtensa
    cd qemu-xtensa
    git checkout  xtensa-esp32
    git submodule update --init dtc
       Add /hw/xtensa/esp32.c as described eariler in this doc
       Edit this row
       serial_hds[0] = qemu_chr_new("serial0", "null",NULL);
    cd ..
    mkdir build-qemu-xtensa
    cd build-qemu-xtensa
    ../qemu-xtensa/configure --disable-werror --prefix=`pwd`/root --target-list=xtensa-softmmu


## Remote debugging with gdbstub.c
It is a good idea to save the original   xtensa-esp32-elf-gdb as the one in the bin directory works best tit qemu
    
    xtensa-esp32-elf-gdb   build/app-template.elf   -b 115200 -ex 'target remote /dev/ttyUSB0'
   

## Rom symbols from rom.elf
To make debugging the rom functions there is a file rom.elf that contains debug information for the rom file. 
It was created with the help from this project, https://github.com/jcmvbkbc/esp-elf-rom
      
        xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S > io.txt
        xtensa-esp32-elf-gdb   build/app-template.elf  -ex 'target remote:1234'
       (gdb) add-symbol-file rom.elf 0x40000000
       (gdb) b start_cpu0_default
       (gdb) c
       (gdb) b app_main
	
