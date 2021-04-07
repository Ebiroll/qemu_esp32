 qemu_esp32
Add tensilica esp32 cpu and a board to qemu and dump the rom to learn more about esp-idf
###   ESP32 in QEMU.

This documents how to add an esp32 cpu and a simple esp32 board to qemu in order to run an app compiled with the SDK (esp-idf) in QEMU. Esp32 is a 240 MHz dual core Tensilica LX6 microcontroller.
It is a good way to learn about qemu , esp32 and the esp32 rom.



## Quick Start

```
1. Build qemu
git clone git://github.com/Ebiroll/qemu-xtensa-esp32
mkdir qemu_esp32
cd qemu_esp32
../qemu-xtensa-esp32/configure --disable-werror --prefix=`pwd`/root --target-list=xtensa-softmmu,xtensaeb-softmmu
make

2. Dump rom1 and rom.bin
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 -b 921600 -p /dev/ttyUSB0 dump_mem 0x40000000 0x000C2000 rom.bin
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 -b 921600 -p /dev/ttyUSB0 dump_mem 0x3FF90000 0x00010000 rom1.bin
If you have rev3 of the rom, then it is more safe to use espressifs rom0 dump
wget https://github.com/espressif/qemu/raw/esp-develop/pc-bios/esp32-r0-rom.bin
mv esp32-r0-rom.bin rom.bin

3. Build an esp32 application
Open a new window to build example app, and set env variables
export PATH=$PATH:$HOME/esp/xtensa-esp32-elf/bin
export IDF_PATH=~/esp/esp-idf
cd esp   
git clone git://github.com/Ebiroll/qemu_esp32
cd qemu_esp32
make menuconfig
> Set [*] Run FreeRTOS only on first core      
>     [ ] Make exception and panic handlers JTAG/OCD aware 
make

4. Build the qemu_flash app to create the qemu flash image
Dont forget to check the code in toflash.c as it uses hardcoded paths.
gcc toflash.c -o qemu_flash

5. Flash the esp32flash.bin file
> ./qemu_flash build/app-template.bin

Note that this will create an 4MB empty file filled with 0xff and write
bootloader, partition table and application to this file called esp32flash.bin
This is then copied to  /home/user/qemu_esp32, this file will be read by qemu at upstart.

6. In the first window, start qemu
  > xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M  -s  > io.txt
Add -S if you want halt execution until debugger is attached.

7. Optional step (start debugger)
cp qemu_esp32/bin/xtensa-esp32-elf-gdb    $HOME/esp/xtensa-esp32-elf/bin/xtensa-esp32-elf-gdb.qemu   
  > xtensa-esp32-elf-gdb.qemu   build/app-template.elf -ex 'target remote:1234'
    (gdb) b bootloader_main
    (gdb) b app_main
    (gdb) c
When breakpoint is hit try Ctrl-X then O, also consider starting with data directory to where you cloned this
   xtensa-esp32-elf-gdb.qemu --data-directory ~/esp/qemu_esp32/gdb

Or to debug the bootloader,
    xtensa-esp32-elf-gdb.qemu   build/bootloader/bootloader.elf -ex 'target remote:1234'
    (gdb) b bootloader_main
Remember to start qemu with -S (capital S) this will halt qemu execusion until debugger is attached.

A fun example with an emulated 1306 display
cd examples/06_1306_interactive
ln -s ../../components/ components
make
gcc ../toflash.c -o qemu_flash
./qemu_flash build/app-template.bin
Start qemu
Connect to emulated UART1
nc 127.0.0.1 8881
Press return
However the latest changes in esp-idf makes i2c emulation very slow.
A much faster but non-interactive example is the 06_duino example.
It runs fine with the latest version of esp-idf

```

## Docker run
This uses the espressif version under windows, rom provided by espressif
```
docker build -t qemu_esp32/ubuntu -f .\Dockerfile .
docker run -v C:\work\qemu_esp32\:/home/src -p 1234:1234 -ti qemu_esp32/ubuntu  /bin/bash
Inside the container
> chmod a+x qemu-system-xtensa/qemu-system-xtensa
You must have built a runnable flash file.
> cp /home/src/esp32flash.bin .
> qemu-system-xtensa/qemu-system-xtensa  -nographic -M esp32 -drive file=esp32flash.bin,if=mtd,format=raw -s -S
Outside container
xtensa-esp32-elf-gdb   build/app-template.elf -ex 'target remote:1234'
(gdb) b app_main
(gdb) c
(gdb) monitor system_reset
(gdb) monitor info mtree
(gdb) monitor quit
To attach and kill qemu
docker ps
docker exec -ti [container]  bash  
Ctrl-p Ctrl-q
```

## Windows 10

Enable the windows subsystem for linux on Windows 10, 
https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux
As admin in power shell, 
```
Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux
```
Install ubuntu (i.e. 18.04 LTS) from the app store.
```
sudo apt-get update && sudo apt-get upgrade
sudo  apt-get install build-essential
sudo apt-get install libpixman-1-0 libpixman-1-dev 
apt-get install libglib2.0-dev zlib1g-dev
```
It should also be possible to build with MSYS2 but performance is not so good.
As an X-server you can use this one,
https://mobaxterm.mobatek.net/

## Build with cmake
Latest version of esp-idf recomends
    idf.py menuconfig
    idf.py build

As location of partition is different for cmake
    gcc ../../toflash-cmake.c -o qemu_flash
##  April 2021
  Updated original esp32-qemu to use espressifs sha calculations due to changes in latest versions of esp-idf.
  
  Also no need to dump or use rom1.bin as they point to the same data.
  The reason you might want to use the original rathere than espressifs
  more complete implementation is: 
  
    1. that it boots much faster on slower computer
    2. Works much better with the xtensa-esp32-elf-gdb.qemu debugger
    3. Boots from you own romdump

##  March 2021
  Updated latest version with espressifs latest qemu changes, it now runs qemu 5.2 (New build system)
  S2 emulation has regressed, but probably good enough to step into the rom dump.
  I also added the ssd1306 device on the i2c bus, on address 0x3C, not fully implemented but examples should work.
  https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
    
    git clone https://github.com/Ebiroll/qemu-xtensa-esp32s2
    mkdir qemu_esp32s2 ;cd qemu_esp32s2
    ../qemu-xtensa-esp32s2/configure --target-list=xtensa-softmmu,riscv32-softmmu     --enable-debug --enable-sanitizers     --disable-strip     --disable-capstone --disable-vnc --enable-gcrypt --disable-seccomp        
    qemu-system-xtensa -M esp32s2 -s   -d unimp,guest_errors,page -nographic
    xtensa-esp32s2-elf-gdb build/led.elf  -ex 'target remote:1234'
    (gdb) monitor quit
    Remember to use toflash-s2.c as sha calculations is not implemented.
# esp32
    The esp32 emulation has gotten better with i2c support. I also added an ssd1306 to the i2c bus 0. adress 0x3c
    cp ../qemu-xtensa-esp32s2/pc-bios/esp32-v3-rom.bin .
    xtensa-softmmu/qemu-system-xtensa  -M esp32 -drive file=esp32flash.bin,if=mtd,format=raw -s  -serial stdio -global driver=timer.esp32.timg,property=wdt_disable,value=true


##  June 2020
esp32s2 emulation. Starting to work,  SHA256 does not work properly, so checksums are patched to 0 byt the toflash-s2.c file.

     Rom dumps
       (gdb) dump binary memory drom0.bin 0x3ffa0000 0x3ffaffff
       (gdb) dump binary memory s2rom.bin 0x40000000 0x4001ffff 

     The data rom is equal to the second half of s2rom.bin but can be convenient to have

    The s2rom.bin requires some  patching as done by s2rompatch.c
    Compile and run it to patch s2rom.bin


    git clone https://github.com/Ebiroll/qemu-xtensa-esp32s2
    ../qemu-xtensa-esp32s2/configure --target-list=xtensa-softmmu     --enable-debug --enable-sanitizers     --disable-strip     --disable-capstone --disable-vnc    
    qemu-system-xtensa -M esp32s2 -s   -d unimp,guest_errors,page -nographic
    xtensa-esp32s2-elf-gdb build/led.elf  -ex 'target remote:1234'
    

    0x4000f254 in ?? ()
    (gdb) load
    Loading section .flash.rodata, size 0x576c lma 0x3f000020
    Loading section .dram0.data, size 0x1e74 lma 0x3ffbe150
    Loading section .iram0.vectors, size 0x403 lma 0x40024000
    Loading section .iram0.text, size 0x9d40 lma 0x40024404
    Loading section .flash.text, size 0x147f7 lma 0x40080020
    Start address 0x40025528, load size 155418



```
ESP-ROM:esp32s2-rc4-20191025
Build:Oct 25 2019
rst:0x1 (POWERON),boot:0xa (SPI_FAST_FLASH_BOOT)
MMU Map flash 00000000 to 3F000000
SPIWP:0xee
mode:DIO, clock div:2
load:0x3ffe8100,len:0x4
load:0x3ffe8104,len:0x17d4
load:0x40050000,len:0x14b0
load:0x40054000,len:0x210c
SHA-256 comparison failed:
Calculated: 0000000000000000000000000000000000000000000000000000000000000000
Expected: 4944baaa4edc3b31e938597047c92c74b9ba1454c182196b0147204d7c2e1cdb
Attempting to boot anyway...
(229) boot: ESP-IDF v4.2-dev-1660-g7d7521367 2nd stage bootloader[0m
(239) boot: compile time 22:45:23
unimp write  000012FC,00008000
MMU Map flash 00000000 to 3FFC0000
FFFFFFFF,FFFFFFFF,b 100204E9,p 020150AA
(245) boot: chip revision: 0
(2500) boot.esp32s2: SPI Speed      : 40MHz
(2501) boot.esp32s2: SPI Mode       : DIO
(2502) boot.esp32s2: SPI Flash Size : 2MB
unimp read  00000040
unimp write  00000040,00080001
read RTC_CNTL_RESET_STATE_REG
(2508) boot: Enabling RNG early entropy source...
unimp write  00001200,00008000
MMU Map flash 00000000 to 3F000000
FFFFFFFF,FFFFFFFF,b 100204E9,p 020150AA
unimp read  00000040
unimp write  00000040,00080001
(2513) boot: Partition Table:
(2514) boot: ## Label            Usage          Type ST Offset   Length
(2516) boot:  0 nvs              WiFi data        01 02 00009000 00006000
(2517) boot:  1 phy_init         RF data          01 01 0000f000 00001000
(2519) boot:  2 factory          factory app      00 00 00010000 00100000
(2521) boot: End of partition table
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
read RTC_CNTL_RESET_STATE_REG
read RTC_CNTL_RESET_STATE_REG
unimp write  000012FC,00008001
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
read RTC_CNTL_RESET_STATE_REG
(2187) esp_image: segment 0: paddr=0x00010020 vaddr=0x3f000020 size=0x0576c ( 22380) map
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
read RTC_CNTL_RESET_STATE_REG
(2208) esp_image: segment 1: paddr=0x00015794 vaddr=0x3ffbe150 size=0x01e74 (  7796) load
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
read RTC_CNTL_RESET_STATE_REG
(2218) esp_image: segment 2: paddr=0x00017610 vaddr=0x40024000 size=0x00404 (  1028) load
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
read RTC_CNTL_RESET_STATE_REG
(2220) esp_image: segment 3: paddr=0x00017a1c vaddr=0x40024404 size=0x085fc ( 34300) load
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
MMU Map flash 00020000 to 3F010000
3350FFAD,0020C010,b 8A88000A,p 23571A62
MMU Map flash 00020000 to 3F3F0000
3350FFAD,0020C010,b 8A88000A,p 23571A62
read RTC_CNTL_RESET_STATE_REG
[0;32mI (2246) esp_image: segment 4: paddr=0x00020020 vaddr=0x40080020 size=0x147f8 ( 83960) map
MMU Map flash 00020000 to 3F000000
3350FFAD,0020C010,b 8A88000A,p 23571A62
MMU Map flash 00030000 to 3F010000
BF65A421,1E3A56FA,b 00236507,p 7A3C7480
read RTC_CNTL_RESET_STATE_REG
[0;32mI (431) esp_image: segment 5: paddr=0x00034820 vaddr=0x4002ca00 size=0x01748 (  5960) load
MMU Map flash 00030000 to 3F000000
BF65A421,1E3A56FA,b 00236507,p 7A3C7480
MMU Map flash 00030000 to 3F3F0000
BF65A421,1E3A56FA,b 00236507,p 7A3C7480
MMU Map flash 00030000 to 3F000000
BF65A421,1E3A56FA,b 00236507,p 7A3C7480
I (963) cache: Instruction cache 	: size 8KB, 4Ways, cache line size 32Byte
I (983) cpu_start: Pro cpu up.
I (1010) cpu_start: Application information:
I (1032) cpu_start: Project name:     led
I (1054) cpu_start: App version:      b8c968e-dirty
I (1076) cpu_start: Compile time:     Jun 13 2020 03:13:53
I (1108) cpu_start: ELF file SHA256:  0000000000000000...
I (1131) cpu_start: ESP-IDF:          v4.2-dev-1660-g7d7521367
I (1153) cpu_start: Single core mode
I (1185) heap_init: Initializing. RAM available for dynamic allocation:
I (1199) heap_init: At 3FFC07C8 len 0003B838 (238 KiB): DRAM
I (1202) heap_init: At 3FFFC000 len 00003A10 (14 KiB): DRAM
I (1213) cpu_start: Pro cpu start user code
W (2561) rtc_init: o_code calibration fail
pi write  00000018,10000000
spi read  00000028,00000000
WTF
spi write  00000028,00000017
spi read  00000000,00000000
M25P80: Unknown cmd 9d
I (1378) spi_flash: detected chip: issi
I (1394) spi_flash: flash io: dio
W (1395) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (1397) cpu_start: Starting scheduler on PRO CPU.


The fun ends here as there is no timer interrupts and therefore freertos will not tick.
// TODO!!! FIX OLAS
#define ETS_WIFI_MAC_INTR_SOURCE 
https://github.com/espressif/esp-idf/blob/master/components/freertos/xtensa/readme_xtensa.txt

On first try, we got this
ESP-ROM:esp32s2-rc4-20191025
Build:Oct 25 2019
rst:0x0 (N/A),boot:0x12 (DOWNLOAD(USB/UART0/1/SPI))
ets_main.c 460 

```

```
Same elf loaded with gdb
(gdb) load
Loading section .flash.rodata, size 0x576c lma 0x3f000020
Loading section .dram0.data, size 0x1e74 lma 0x3ffbe150
Loading section .iram0.vectors, size 0x403 lma 0x40024000
Loading section .iram0.text, size 0x9d44 lma 0x40024404
Loading section .flash.text, size 0x147f7 lma 0x40080020
Start address 0x40025524, load size 155422

```




## May 2020
Espressifs implementation is a bit more accurate but a bit slow on my old computer.
They have also released a useable rom file. If you want to run gdb I recommend the gdb version from the bin directory here.
This shows a way of how I compile the espressif version in parallell.
To build a parallell qeum_espressif qemu binary with espressifs implementation do:
```
cd
git clone https://github.com/Ebiroll/qemu-xtensa-esp32 -b  esp-develop esp-develop
mkdir qemu_espressif
cd qemu_espressif
 ../esp-develop/configure --target-list=xtensa-softmmu     --enable-debug --enable-sanitizers     --disable-strip     --disable-capstone --disable-vnc
make
cp ../esp-develop/pc-bios/esp32-r0-rom.bin .
Make sure you have a usable esp32flash.bin file then start with
xtensa-softmmu/qemu-system-xtensa -nographic -M esp32 -drive file=esp32flash.bin,if=mtd,format=raw -s
```

## Update Nov 2019
Espressif has released a qemu for esp32
    https://github.com/espressif/qemu
However they have not yet released a usable rom.elf
Here is one which might be usable in the future, https://dl.espressif.com/dl/esp32_rom.elf

This version is more accurate than my implementation but lacks some features like 1306 emulation. 
In the future, I will start using this instead and have created a branch, esp-develop.
After clone you can switch to this branch,
    git checkout esp-develop


But then you have to start qemu with,
    xtensa-softmmu/qemu-system-xtensa -nographic  -M esp32   -drive file=esp32flash.bin,if=mtd,format=raw

you must also copy the file
    esp32-r0-rom.bin 
    cp ../qemu-xtensa-esp32/pc-bios/esp32-r0-rom.bin .


To start with different boot strapping value (boot)
    -global driver=esp32.gpio,property=strap_mode,value=0x0f

To connect the serial deviice to port 8880, However I was not able to flash.
    -serial tcp::8880,server,nowait

    export ESPPORT=socket://localhost:8880
    idf.py flash

To start with the openeth emulation, add
    -nic user,model=open_eth,hostfwd=tcp::10080-:80 -s

# Espressif qemu, openeth examples, requires esp-idf 4.1 

    Top) → Component config → Ethernet
    Espressif IoT Development Framework Configuration
     [ ] Support ESP32 internal EMAC controller ----
     [ ] Support SPI to Ethernet Module ----
     [*] Support OpenCores Ethernet MAC (for use with QEMU) --->

And
    (Top) → Example Connection Configuration
     Espressif IoT Development Framework Configuration
     Connect using (Ethernet) --->

Starting with ethernet emulation
    qemu-system-xtensa -nographic -M esp32 -drive file=esp32flash.bin,if=mtd,format=raw -nographic -nic user,model=open_eth,hostfwd=tcp::10080-:80 -s

## Update Jan 2019

Head and version 3.2 works fine,
If you want to use ethernet emulation in earlier version you must patch this
https://github.com/Ebiroll/qemu_esp32/commit/bc2b6e2f50261885751ebf2335a82325eafbc656



Qemu also works with esp-adf
```
export ADF_PATH=~/esp/esp-adf
Or as in my case, I use the released 1.0 version
export ADF_PATH=~/esp/esp-adf-v1.0


Change the Makfile
PROJECT_NAME := your_proj
include $(ADF_PATH)/project.mk

Make sure to disable,
component config → ESP32-specific → SPI RAM config
PSI RAM emulation is next on the todo list.
[*] Ignore PSRAM when not found

```


## IMPORTANT update, Feb 2018

As the bootloader uses HW accelerated sha256 calculations this is now
emulated by qemu,checksum must be correct and should not be overwritten with 000

## IMPORTANT update, July 2017

Booting from emulated flash! This is very cool.
Now IT IS MANDATORY to have a proper flash file , ets_unpack_flash_code is not longer patched , qemu now relies on flash and MMU emulation.

Not that esp-idf is constantly being updated and you must keep the qemu source updated to keep it working.


> Serial flasher config  --->  Flash size (4 MB)

  However it seems that latest chages in the bootloader (esp-idf) breaks this,
  flash hash caclulations dont match.
  This is because bootloader always use hardware acceleration to calculate the hash and
  qemu does not emulate hardware SHA5 calculation.


```
  Component config  --->    
       mbedTLS  --->
          [ ] Enable hardware SHA acceleration                                     

Previously the toflash.c program overwrote the checksum with 00000
this must not happen anymore as the bootloader relies on proper hash of the image

    if (patch_hash==1) {
      for (j=0;j<33;j++)
        {
                tmp_data[file_size-j]=0;
        }
    }


```



To create a esp32flash.bin file build `qemu_flash` tool found in this repository.

> gcc toflash.c -o qemu_flash

Then run the ./qemu_flash program. 

> ./qemu_flash build/app-template.bin

Note that you have to use the .bin file as argument. This will generate a flash image with bootloader, partition information and flash file that the bootloder can use boot the proper application from.

You can also start with a flash dump image, then remove this from the toflash.c file. 
```
    // Overwrites esp32flash.bin file
    system("dd if=/dev/zero bs=1M count=4  | tr \"\\000\" \"\\377\" >  esp32flash.bin");

```
This way you can use calibratrion data from the flash file if you patch hw/xtensa/esp32.c of qemu to match your flash dump.
The printouts are very helpful to allow you to update to correct values.
```
case 0x5a004:
           printf("EFUSE MAC\n"); 
           return 0xc400c870;
           break;
      case 0x5a008:
           printf("EFUSE MAC\n"); 
           return 0xffda240a;
           break;
```


You no longer need to give  application as kernel parameter.
> -kernel /home/olas/esp/qemu_esp32/examples/07_flash_mmap/build/mmap_test.elf

```
STARTING QEMU, with forward of localhost port 10080 to qemu port 80
xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10080-192.168.4.3:80  -net dump,file=/tmp/vm0.pcap    -s   > io.txt


 This will reload the application and prevent the crash as the code is reloaded 
 on the correct locations.

    > (gdb) b app_main
    > (gdb) c
```
Press Ctrl-X o  to open the source
n
Press return to reuse latest command




It is sometimes possible to run only from the generated flash, 
```
xtensa-softmmu/qemu-system-xtensa  -cpu esp32 -M esp32  -s   > io.txt
TRYING to MAP esp32flash.binMAPPED esp32flash.binI (14) boot: ESP-IDF v3.0-dev-20-g9b955f4c 2nd stage bootloader
I (14) boot: compile time 12:24:53
I (14) boot: Enabling RNG early entropy source...
I (14) boot: SPI Speed      : 40MHz
I (14) boot: SPI Mode       : DIO
I (14) boot: SPI Flash Size : 2MB
I (14) boot: Partition Table:
I (14) boot: ## Label            Usage          Type ST Offset   Length
I (15) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (15) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (15) boot:  2 factory          factory app      00 00 00010000 00100000
I (15) boot: End of partition table
I (15) boot: Disabling RNG early entropy source...
I (16) boot: Loading app partition at offset 00010000
I (328) boot: segment 0: paddr=0x00010018 vaddr=0x00000000 size=0x0ffe8 ( 65512) 
I (328) boot: segment 1: paddr=0x00020008 vaddr=0x3f400010 size=0x1077c ( 67452) map
I (328) boot: segment 2: paddr=0x0003078c vaddr=0x3ffb0000 size=0x023d8 (  9176) load
I (329) boot: segment 3: paddr=0x00032b6c vaddr=0x40080000 size=0x00400 (  1024) load
I (329) boot: segment 4: paddr=0x00032f74 vaddr=0x40080400 size=0x12fb0 ( 77744) load
I (332) boot: segment 5: paddr=0x00045f2c vaddr=0x400c0000 size=0x00000 (     0) load
I (332) boot: segment 6: paddr=0x00045f34 vaddr=0x00000000 size=0x0a0d4 ( 41172) 
I (333) boot: segment 7: paddr=0x00050010 vaddr=0x400d0018 size=0x4c9a4 (313764) map
I (333) cpu_start: Pro cpu up.
I (333) cpu_start: Single core mode
I (334) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (334) heap_alloc_caps: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (334) heap_alloc_caps: At 3FFB7C40 len 000283C0 (160 KiB): DRAM
I (334) heap_alloc_caps: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (335) heap_alloc_caps: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (335) heap_alloc_caps: At 400933B0 len 0000CC50 (51 KiB): IRAM
I (335) cpu_start: Pro cpu start user code
I (478) cpu_start: Starting scheduler on PRO CPU.
Running in qemu
Running in qemu
ethoc: num_tx: 8 num_rx: 8
TCP/IP initializing...
TCP/IP initialized.
E (487) wifi: esp_wifi_internal_set_sta_ip 1281 wifi is not init
E (487) event: system_event_sta_got_ip_default 162 esp_wifi_internal_set_sta_ip ret=12289
Applications started.
I (1507) 1306: Setting UART configuration number 1...
I (1507) 1306: Setting UART configuration number 1...
I (1517) uart: queue free spaces: 10
1. clear display.
2. Set page and col to 0.
3. Set page to 4.
4. Set column to 4.
5. Send 8*0xff.
6. Send 8*0x0f.
7. Incremet nuUart Menu
1. clear display.
2. Set page and col to 0.
3. Set page to 4.
4. Set column to 4.
5. Send 8*0xff.
6. Send 8*0x0f.
```


## Setting up visual studio code
[Works pretty well in linux now](./VSCODE.md)



By following the instructions here, I added esp32 to qemu.
http://wiki.linux-xtensa.org/index.php/Xtensa_on_QEMU

prerequisites Debian/Ubuntu:
```
sudo apt-get install libpixman-1-0 libpixman-1-dev 
```

prerequisites Redhat/Centos
yum install glib2-devel pixman-devel




## qemu-esp32

```
git clone git://github.com/Ebiroll/qemu-xtensa-esp32
```

Requires that you run a patched gdb to run in qemu. If not you will get Remote 'g' packet reply is too long:
The key to using the original gdb, is to set num_regs = 104, in core-esp32.c
However debugging c-functions in the elf file requires a patched qemu.
In the bin directory there are patched versions of xtensa-esp32-elf-gdb.
You can also build your own. Just apply this patch,
https://github.com/jcmvbkbc/xtensa-toolchain-build/blob/master/fixup-gdb.sh
Another option is to build from this prepatched gdb crosstool source,
https://github.com/Ebiroll/gdb
One has to create a link or copy ar to x86_64-build_pc-linux-gnu-ar
Also dont forget to prepare this version for use with with qemu.
    cd gdb
    cp xtensa-config.c.qemu xtensa-config.c
    ./configure  --without-guile --target=xtensa-esp32-elf


## Building gdb 8.1 for qemu
```
cd esp
git clone https://github.com/espressif/binutils-gdb
cd binutils-gdb
git checkout remotes/origin/esp32-2018r1_gdb-8_1
./configure  --without-guile --host=x86_64-build_pc-linux-gnu --target=xtensa-esp32-elf --disable-werror
make
When feeling lucky apply this patch
https://github.com/jcmvbkbc/meta-zephyr-sdk/commit/42be8b49d5915a0bb94ef8dfbde50fe130e7a772

```

## Building qemu
```
mkdir ../qemu_esp32
cd ../qemu_esp32
Then run configure as,
../qemu-xtensa-esp32/configure --disable-werror --prefix=`pwd`/root --target-list=xtensa-softmmu --python=/usr/bin/python2
```

# Build qemu on linux with python3, i.e. on arch linux
```
../qemu-xtensa-esp32/configure --disable-werror --prefix=`pwd`/root --target-list=xtensa-softmmu --python=/usr/bin/python2
```


The goal was to run the app-template.elf and connect the debugger to qemu to allow debugging of the application. It works quite well and as the rom is patched slightly it its possible to run the elf file that is generated by the esp-idf qemu. However you must use requir theese configutation items. 
```

Dont use autodetect fom Main XTAL frequency
Main XTAL frequency (40 MHz)  --->                         
 
  [ ] Make exception and panic handlers JTAG/OCD aware 

This is no longer required but recomended.
[*] Run FreeRTOS only on first core,    
[ ] Initialize PHY in startup code
[ ] Make exception and panic handlers JTAG/OCD aware       

Optimization level (Debug) 
Not sure if this is necessary,
 Xtensa timer to use as the FreeRTOS tick source (Timer 0 (int 6, level 1))

Now I have also added the possibility to initialize PHY in startup code.
This however reqires that you match the EFUSE mac adresses with the ones dumped from the flash dump. (esp32.c)

      case 0x5a004:
           printf("EFUSE MAC\n"); 
           return 0xc400c870;
           break;
      case 0x5a008:
           printf("EFUSE MAC\n"); 
           return 0xffda240a;
           break;

Starting wifi works sometimes but its better to do like this, if you want to be able to flash and run 
the same app in qemu.

    #include "emul_ip.h"

    if (is_running_qemu()) {
        printf("Running in qemu\n");
        ESP_ERROR_CHECK( esp_event_loop_init(esp32_wifi_eventHandler, NULL) );

        //task_lwip_init(NULL);
        xTaskCreatePinnedToCore(task_lwip_init, "lwip_init", 2*4096, NULL, 14, NULL, 0); 

    }
    else {
      wifi_task(NULL);
    }


```



UART emulation is not so good as output is primaraly on stderr. Also interrupts are not emulated so well.
esp32.c creates a 3 threads with socket on port 8880-8882 tto allow connect to the serial port over a socket. 
In the starting terminal you can see output from all uarts mixed. However if you want to see output from only one uart,
do, 
    nc 127.0.0.1 8880
or
    nc 127.0.0.1 8881
for uart 0.

When running I advise that you build a patched gdb as described in this document or use the gdb provided in the /bin directory (linux 64 bit). This improves the debugging exerience quite a bit. The xtensa-esp32-elf-gdb.arch is built for arch linux.

When debugging in the rom you can use,
```
    (gdb) add-symbol-file rom.elf 0x40000000
An even better rom file is located here    
https://dl.espressif.com/dl/esp32_rom.elf

```
This also works for the original gdb and gives you all names of the functions in rom0.

You can get and compile your own elf file from rom.S found here,
https://github.com/cesanta/mongoose-os/tree/6ccea79b028d8d76191b9c64855ef40c67e096b8/platforms/esp32/src/rom
Or build your own  from esp-idf/components/esp32/ld/esp32.rom.ld:
    //   sort -k 5 esp32.rom.ld | perl -npe 's/=/,/; s/;//'
    xtensa-esp32-elf-gcc -Wl,-N,-Ttext,0x40000000 -nostdlib rom.S -o rom.elf 

To setup esp-idf do, 
```
git clone --recursive https://github.com/espressif/esp-idf.git
```

#  To keep the esp-idf updated  
  git pull & git submodule update --recursive
  git  submodule update --init

```
export PATH=$PATH:$HOME/esp/xtensa-esp32-elf/bin
export IDF_PATH=~/esp/esp-idf
```

#  Recomended version of esp-idf for qemu.

You can most likely use the latest version of esp-idf, just dont start wifi and run on one core only.
Dual core should work because  esp_crosscore_int_send_yield is implemented.
Also rtc_clk_xtal_freq_get works.
However one thread sometimes seems to get stuck waiting for s_flash_op_complete,
Workaround until the problem is fixed,
```
(gdb) thread 2
(gdb) set s_flash_op_complete=1
(gdb) c
```

```
You can probably use head of esp-idf

This version works anyways,
git checkout release/v3.3
git submodule update --recursive
git submodule update --init
If you get strange errors like, 
No rulte to create, esp/esp-idf/tools/kconfig/conf-idf
try cd $IDF_PATH/tools/kconfig; 
    make clean 
    make

For a short time there was a divide by zero.

#5  __udivdi3 (n=128, d=4611190139757918081) at /home/ivan/e/crosstool-NG/.build/src/gcc-5.2.0/libgcc/libgcc2.c:1288
#6  0x400df781 in __udivmoddi4 (rp=0x0, d=4611190964391638913, n=0) at /home/ivan/e/crosstool-NG/.build/src/gcc-5.2.0/libgcc/libgcc2.c:1088
#7  __udivdi3 (n=0, d=4611190964391638913) at /home/ivan/e/crosstool-NG/.build/src/gcc-5.2.0/libgcc/libgcc2.c:1288
#8  0x400d0708 in select_rtc_slow_clk (slow_clk=RTC_SLOW_FREQ_RTC) at /home/olof/esp/esp-idf/components/esp32/./clk.c:120
#9  0x400d0721 in esp_clk_init () at /home/olof/esp/esp-idf/components/esp32/./clk.c:50
#10 0x40080bd5 in start_cpu0_default () at /home/olof/esp/esp-idf/components/esp32/./cpu_start.c:207
#11 0x40080e6a in call_start_cpu0 () at /home/olof/esp/esp-idf/components/esp32/./cpu_start.c:168
```


The emulation has also been some problems with do_global_ctors().
 should be available at this location... 0x3f409fb8
__init_array_start , however nvs reuses 0x3f40000 as I have not gotten it to recognize that this is already mmapped by the bootloader. However when running full bootloader 

Or you can try version 3.2
```
 git checkout v3.2
 git submodule update --init
```


Or you can try version 3.1.2
```
git checkout v3.1.2
git submodule update --init

```

Or you can try version 2.0
```
 git checkout v2.0
 git submodule update --init
```
Anyways, things will probably work much better now.

#  Dumping the ROM0 & ROM1 using esp-idf esptool.py
```
cd ..
git clone --recursive https://github.com/espressif/esp-idf.git
#  dump_mem ROM0(776KB) to rom.bin
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 -b 921600 -p /dev/ttyUSB0 dump_mem 0x40000000 0x000C2000 rom.bin

#  dump_mem ROM1(64KB) to rom1.bin
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 -b 921600 -p /dev/ttyUSB0 dump_mem 0x3FF90000 0x00010000 rom1.bin

Note that rom0 is smaller than the actual dump.
Those two files will be loaded by qemu and must be in same directory as you start qemu.
```

If you see this,
```
I (440) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1540) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
W (2650) phy_init: failed to load RF calibration data (0x1102), falling back to full calibration
I (3740) phy: error: pll_cal exceeds 2ms!!!
I (4300) phy: error: pll_cal exceeds 2ms!!!
I (4860) phy: error: pll_cal exceeds 2ms!!!
I (5420) phy: error: pll_cal exceeds 2ms!!!
I (5980) phy: error: pll_cal exceeds 2ms!!!
I (6540) phy: error: pll_cal exceeds 2ms!!!
I (7100) phy: error: pll_cal exceeds 2ms!!!
I (7660) phy: error: pll_cal exceeds 2ms!!!
```
It is because you try to start wifi



#  Dumping flash content.
Now there is simple flash emulation in qemu. You need the file  esp32flash.bin to be in the same directory as rom.bin & rom1.bin.
If no flashfile exists, an empty file will be created.
```
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --baud 92600 read_flash 0 0x400000  esp32flash.bin
```
Another possibility in order to create a proper flash file is by running the following.
```
Double check the toflash.c program as it copies the generated esp32flash.bin file to the directory where we start qemu
It also assumes partitioning according to partitions_singleapp.bin
gcc toflash.c -o qemu_flash
./qemu_flash build/app-template.bin
```



BOOTLOADER UPDATE
==================
To get more correct mmap behaviour now the bootloader runs first.


This emulation reqires that you generate a proper flash file first. 
Serial flasher config  --->  Flash size (4 MB)

Then run the ./qemu_flash program. (described earlier)
    ./qemu_flash build/app-template.bin

Note that you have to use the .bin file as argument. This will generate a flash image with bootloader, partition information and flash file that the bootloder can use boot the proper application from.


To debug you can use,
    xtensa-esp32-elf-gdb.qemu   build/app-template.elf -ex 'target remote:1234'
    (gdb) b app_main

Or to debug the bootloader,
    xtensa-esp32-elf-gdb.qemu   build/bootloader/bootloader.elf -ex 'target remote:1234'
    (gdb) b bootloader_main
    (gdb) b bootloader_mmap


```
#define DPORT_PRO_FLASH_MMU_TABLE ((volatile uint32_t) 0x3FF10000)
PAGE 0 is the content of 0x3FF10000 the virtual adress ranged mapped would be adress 0x3f400000 - 0x3f410000
Page 1 would be adress 0x3f410000 - 0x3f420000
I guess paddr=2 would means that this virtual range is mapped to flash content at 0x20000? 
This is output from spi_flash_mmap_dump(); after running the second stage bootloader but before nv_init().
Note that the enable phy at startup would call nv_init before app_main. 
spi_flash_mmap_dump()
    Display information about mapped regions.
    This function lists handles obtained using spi_flash_mmap, along with range of pages allocated to each handle. It also lists all non-zero entries of MMU table and corresponding reference counts.
page 0: refcnt=1 paddr=2
page 77: refcnt=1 paddr=4
page 78: refcnt=1 paddr=5
Here is the bootloader output from loading the app partition.
I (80) boot: Loading app partition at offset 00010000
I (435) boot: segment 0: paddr=0x00010018 vaddr=0x00000000 size=0x0ffe8 ( 65512) 
I (435) boot: segment 1: paddr=0x00020008 vaddr=0x3f400010 size=0x03824 ( 14372) map
I (440) boot: segment 2: paddr=0x00023834 vaddr=0x3ffb2ac0 size=0x012a0 (  4768) load
I (451) boot: segment 3: paddr=0x00024adc vaddr=0x40080000 size=0x00400 (  1024) load
I (458) boot: segment 4: paddr=0x00024ee4 vaddr=0x40080400 size=0x0fdd4 ( 64980) load
I (496) boot: segment 5: paddr=0x00034cc0 vaddr=0x400c0000 size=0x00000 (     0) load
I (497) boot: segment 6: paddr=0x00034cc8 vaddr=0x00000000 size=0x0b340 ( 45888)
I (502) boot: segment 7: paddr=0x00040010 vaddr=0x400d0018 size=0x11dcc ( 73164) map
I (510) boot: segment 8: paddr=0x00051de4 vaddr=0x50000000 size=0x00008 (     8) load

```
Now you can also debug the bootloder with qemu. There used to be some 
problem with the elf file, but its fixed with the later versions of esp-idf,
```
Set Bootloader config
(X) Verbose 
Before running bootloader, locate all // TO TEST BOOTLOADER in (esp32.c)
and add or remove comments.

xtensa-softmmu/qemu-system-xtensa  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/bootloader/bootloader.elf -s  > io.txt

 Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (0) boot: Espressif ESP32 2nd stage bootloader v. V0.1
I (0) boot: compile time 23:25:16
D (1) esp_image: reading image header @ 0x1000
D (1) bootloader_flash: mmu set block paddr=0x00000000 (was 0xffffffff)
(qemu) Flash map  00000000 to memory, 3F720000
(qemu) Flash partition data is loaded.
D (1) boot: magic e9
D (1) boot: segments 04
D (1) boot: spi_mode 02
D (1) boot: spi_speed 00
D (1) boot: spi_size 01
I (1) boot: SPI Speed      : 40MHz
I (1) boot: SPI Mode       : DIO
I (1) boot: SPI Flash Size : 2MB
I (1) boot: Partition Table:
I (2) boot: ## Label            Usage          Type ST Offset   Length
D (2) bootloader_flash: mmu set paddr=00000000 count=1
D (2) boot: mapped partition table 0x8000 at 0x3f408000
D (2) boot: load partition table entry 0x3f408000
D (2) boot: type=0 subtype=0
I (2) boot: End of partition table
E (2) boot: nothing to load
user code done
```
Running the bootloader without dumpimg the flash content will give an output like this,
```
TRYING to MAP esp32flash.bin MAPPED esp32flash.bin
ets Jun  8 2016 00:22:57
rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (0) boot: Espressif ESP32 2nd stage bootloader v. V0.1
I (0) boot: compile time 23:25:16
D (1) esp_image: reading image header @ 0x1000
D (1) bootloader_flash: mmu set block paddr=0x00000000 (was 0xffffffff)
(qemu) Flash map  00000000 to memory, 3F720000
Flash partition data is loaded.
E (1) esp_image: image at 0x1000 has invalid magic byte
E (1) boot: failed to load bootloader header!
user code done
```
This because qemu will create an empty file esp32flash.bin and there is no partition table in this flash.



### Start qemu
```
  > xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.1.0/24,host=192.168.1.40,hostfwd=tcp::10077-192.168.1.3:7  -net dump,file=/tmp/vm0.pcap  -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S > io.txt

```

### Start the debugger
```
  > xtensa-esp32-elf-gdb  build/app-template.elf -ex 'target remote:1234'

  (gdb) x/10i $pc
  (gdb) x/10i 0x40000400
  (gdb) p/x $a0

  Call0 will set a0 with return adress, this will list the instructions at the call
  (gdb) x/10i $a0-3
  (gdb) info symbol 0x400d0eb1
  (gdb) layout next
  (gdb) b app_main
  (gdb) continue
  (gdb) list *$pc
Ctrl-X and the O opens the source
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

#  Running from dumped flash content.
If you dump the flash from an actual hardware device,
```
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --baud 920600 read_flash 0 0x400000  esp32flash.bin
```

You might get this problem
```
E (37) esp_image: Image hash failed - image is corrupt
E (37) boot: Factory app partition is not bootable
E (37) boot: No bootable app partitions in the partition table
```
This is due to incorrect dump or some flash emulation error.If you still want to run, you can replace the bootloader ony and patch the bootloader to skip the check. These are some hints to get you started.


    ./qemu_flash -bl
    xtensa-esp32-elf-gdb.qemu   build/bootloader/bootloader.elf -ex 'target remote:1234'
    (gdb) b bootloader_main

Dont forget to configure flashsize to < 4MB
Also remember that the bootloader has changed slightly during the different versions of the esp-id.


#  Objdump
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
# i2c emulation
Emulation of i2c0 is started but not yet finished. 
The files involved in the emulation are,
```
hw/i2c/i2c_esp32.c
hw/xtensa/tmpbme280.c
hw/display/ssd1306.c
```
The idea was to emulate a 1306 display over i2c.
Almost there but need improved emulation to get 06_duino example running.
This example uses more ssd1306 commands than the other examples.
[[https://github.com/Ebiroll/qemu_esp32/blob/master/img/1306.jpg] ]

# Results
If you get something like this,
```
Illegal entry instruction(pc = 40080a4c), PS = 0000001f
Illegal entry instruction(pc = 40080a4c), PS = 0000001f
```
Then you have probably not downloaded rom files or put them in another directory than where you start qemu.


I got my ESP32-dev board from Adafruit. https://dl.espressif.com/dl/schematics/ESP32-Core-Board-V2_sch.pdf  I have made two dumps and mapped the dumps into the files rom.bin & rom1.bin
The code in esp32.c also patches the function ets_unpack_flash_code.
Instead of loading the code it sets the user_code_start variable (0x3ffe0400) This will later be used
by the rom to start the loaded elf file. The functions SPIEraseSector & SPIWrite are also patched to return 0.
The rom function -- ets_unpack_flash_code is located at 0x40007018.


The serial device should also be emulated differently. Now serial output goes to stderr.
All io calls are printed on stdout thats why you must do  > io.txt 
Opening anoter terminal and doing tail -f io.txt allows you to see what io ports are
accessed.

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

The RER, WUR and RUR instructions are not implemented in qemu yet,
My version has some hadling of these instructions.


When configuring, choose
  Component config  --->   
    FreeRTOS  --->
      [*] Run FreeRTOS only on first core 
    ESP32-specific config  --->
      [ ] Initialize PHY in startup code  

```
However, it might not be necessary to run o only first core anymore.
This patch seems to do the trick. https://github.com/espressif/esp-idf/commit/47b8f78cb0e15fa43647788a808dac353167a485


##  Before the rom patches
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


#  Debugging tips
```
If you get an exception like this
Guru Meditation Error of type StoreProhibited occurred on core   0. Exception was unhandled.
Register dump:
PC      :  4008189e  PS      :  00060030  A0      :  800d05f6  A1      :  3ffe3e20  
A2      :  3ffb1134  A3      :  0002e49c  A4      :  3ffb008c  A5      :  fffffffc  
A6      :  3ffb008c  A7      :  ffffffff  A8      :  3ffe8000  A9      :  bffcffdc  
A10     :  3ffb12c4  A11     :  3ffe8000  A12     :  00000019  A13     :  00000001  
A14     :  7ffe7fe8  A15     :  00000000  SAR     :  00000004  EXCCAUSE:  0000001d  
EXCVADDR:  bffcffe0  LBEG    :  4000c46c  LEND    :  4000c477  LCOUNT  :  ffffffff  
Look at PC for the error then set a breakpoint there 
(gdb) b *0x4008189e
And reset qemu.
We break here ,   components/freertos/./heap_regions.c
(gdb) Pressing Ctrl-X and the o will open the source code if it exists. 
(gdb) where
(gdb) up

```


#  What is some of the problems with this code
Some i/o register name mapping in esp32.c is probably wrong.  The values returned are also many times wrong.
I did this mapping very quickly with grep to get a better understanding of what the rom was doing. I have made the emulation based on what the code expects, should have done more by reading the document.
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

#  Dumping the ROM with the main/main.c program
Please use other method (esptool.py), its easier and faster. This is saved for historical reasons and for the screen instructions. It can also be used to dump the content as seen by cpu1.
Set the environment properly. Build the romdump app and flash it.
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

#  This is head of qemu development.
Now it also works for esp32 debugging. (2.10.0)
```
git clone git://git.qemu.org/qemu.git

cd qemu
This is probably no longer necessary. git submodule update --init dtc

However uart emulation may cause this behaviour.
ERROR:/home/olas/qemu-2.10.1/accel/tcg/tcg-all.c:42:tcg_handle_interrupt: assertion failed: (qemu_mutex_iothread_locked())
```


Another version for qemu exists here https://github.com/OSLL/qemu-xtensa,

```
However I think the important stuff has been mereged into HEA of qemu.
git clone https://github.com/OSLL/qemu-xtensa -b  xtensa-esp32 
but requires that you run a patched gdb to run gdb with qemu. If not you will get Remote 'g' packet reply is too long:
The key to using the original gdb, is to set num_regs = 104, in core-esp32.c 


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
  obj-y += MemoryMapped.o

target/xtensa/Makefile.objs
  obj-y += core-esp32.o
```

If running qemu head (2.9.0) make sure to disable this. from the esp32 options
```
[ ] Make exception and panic handlers JTAG/OCD aware       
```


Interesting info. To be investigated. 
```
static HeapRegionTagged_t regions[]={
	{ (uint8_t *)0x3F800000, 0x20000, 15, 0}, //SPI SRAM, if available
	{ (uint8_t *)0x3FFAE000, 0x2000, 0, 0}, //pool 16 <- used for rom code
	{ (uint8_t *)0x3FFB0000, 0x8000, 0, 0}, //pool 15 <- can be used for BT
	{ (uint8_t *)0x3FFB8000, 0x8000, 0, 0}, //pool 14 <- can be used for BT
```

/* Use first 50 blocks in MMU for bootloader_mmap,
   50th block for bootloader_flash_read
*/
```
#define MMU_BLOCK0_VADDR  0x3f400000
#define MMU_BLOCK50_VADDR 0x3f720000
#define MMU_FLASH_MASK    0xffff0000
#define MMU_BLOCK_SIZE    0x00010000
```



#  Adding flash qemu emulation.


Here are some functions with the associated io instructions, to help me improve flash emulation.
```
From boatloader
Cache_Read_Disable(0);
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,20 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 58  DPORT_APP_CACHE_CTRL_REG  3ff00058=00000020
1 esp32_spi_read: +0xf8: 0x00000000
1 esp32_spi_read: +0x50: 0x00000004
1 esp32_spi_write: +0x50 = 0x00000004
written
---
Cache_Flush(0);  
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,20 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,30 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000030
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000030
io write 40,20  DPORT_PRO_CACHE_CTRL_REG 3ff00040
---

/**
  * @brief Set Flash-Cache mmu mapping.
  *        Please do not call this function in your SDK application.
  *
  * @param  int cpu_no : CPU number, 0 for PRO cpu, 1 for APP cpu.
  *
  * @param  int pod : process identifier. Range 0~7.
  *
  * @param  unsigned int vaddr : virtual address in CPU address space.
  *                              Can be IRam0, IRam1, IRom0 and DRom0 memory address.
  *                              Should be aligned by psize.
  *
  * @param  unsigned int paddr : physical address in Flash.
  *                              Should be aligned by psize.
  *
  * @param  int psize : page size of flash, in kilobytes. Should be 64 here.
  *
  * @param  int num : pages to be set.
  *
  * @return unsigned int: error status
  *                   0 : mmu set success
  *                   1 : vaddr or paddr is not aligned
  *                   2 : pid error
  *                   3 : psize error
  *                   4 : mmu table to be written is out of range
  *                   5 : vaddr is out of range
  */
unsigned int cache_flash_mmu_set(int cpu_no, int pid, unsigned int vaddr, unsigned int paddr,  int psize, int num);
#define MMU_BLOCK50_VADDR 0x3f720000
 int e = cache_flash_mmu_set(0, 0, MMU_BLOCK50_VADDR, map_at (0), 64, 1);
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 5c  DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io read 5c  DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io write 5c,8ff   DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008FF
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8ff  DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8ff
io write 100c8,0 
io write 44,8e6  DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8e6
io write 5c,8e6  DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6  DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8e6
---
Cache_Read_Enable(0)
1 esp32_spi_read: +0x50: 0x00000004
1 esp32_spi_write: +0x50 = 0x00000005
written
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
---
partitions = bootloader_mmap(ESP_PARTITION_TABLE_ADDR, ESP_PARTITION_TABLE_DATA_LEN); 

cache_flash_mmu_set( 0, 0, MMU_BLOCK0_VADDR, src_addr_aligned(0), 64, count ); 
D (2) bootloader_flash: mmu set paddr=00000000 count=1
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 5c  DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io read 5c  DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io write 5c,8ff 
 DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008FF
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8ff 
 DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8ff
io write 10000,0 
 MMU CACHE  3ff10000  0
io write 44,8e6 
 DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8e6
io write 5c,8e6 
 DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
 DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8e6
---
Cache_Read_Disable,
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 58  DPORT_APP_CACHE_CTRL_REG  3ff00058=00000028
---
Cache_Flush,
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,38 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000038
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000038
io write 40,28 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
---
cache_flash_mmu_set,
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 5c  DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io read 5c  DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io write 5c,8ff 
 DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008FF
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8ff 
 DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8ff
io write 10000,0 
 MMU CACHE  3ff10000  0
io write 44,8e6 
 DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8e6
io write 5c,8e6 
 DPORT_APP_CACHE_CTRL1_REG  3ff0005C=000008E6
io read 44 DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
 DPORT_PRO_CACHE_CTRL1_REG  3ff00044  8e6
---
Cache_Read_Enable
1 esp32_spi_read: +0x50: 0x00000005
1 esp32_spi_write: +0x50 = 0x00000005
written
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,28 
DPORT_PRO_CACHE_CTRL_REG 3ff00040
---
We still get 
flash read err, 1000
If unpatching the ets_unpack_flash_code rom function.


This does not work.
head -c 4M /dev/zero > esp32.flash

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


Running two cores, is default behaviour now, try (gdb) info threads
The app cpu starts stalled and will be unstalled by the pro cpu.
```
xtensa-softmmu/qemu-system-xtensa -d guest_errors,page,unimp  -cpu esp32 -M esp32 -m 4M -smp 2 -pflash esp32.flash -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S  > io.txt
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (97134) heap_alloc_caps: Initializing heap allocator:
I (97134) heap_alloc_caps: Region 19: 3FFB5B8C len 0002A474 tag 0
I (97134) heap_alloc_caps: Region 25: 3FFE8000 len 00018000 tag 1
check b=0x3ffb5b98 size=173144 ok
check b=0x3ffdfff0 size=0 ok
check b=0x3ffe800c size=98276 ok
I (97135) cpu_start: Pro cpu up.
I (97135) cpu_start: Starting app cpu, user_code_start is 0x00000000
I (97135) cpu_start: Starting app cpu, entry point is 0x40080b50
I (68034) cpu_start: App cpu up.
I (97140) cpu_start: App cpu started, user_code_start is 0x40080b50
I (97140) cpu_start: Pro cpu start user code
I (97140) rtc: rtc v160 Nov 22 2016 19:00:05
I (97141) rtc: XTAL 40M
I (97142) cpu_start: Starting scheduler on PRO CPU.
I (68253) cpu_start: Starting scheduler on APP CPU.

There used to be some problem with the scheduling and probably some other error as well.
When calling nvs_flash_init()qemu always hangs here,

(gdb) info threads
  Id   Target Id         Frame 
  2    Thread 2 (CPU#1 [running]) 0x40081a75 in spi_flash_op_block_func (arg=0x1)
    at /home/olas/esp/esp-idf/components/spi_flash/./cache_utils.c:67
* 1    Thread 1 (CPU#0 [halted ]) 0x400d2444 in esp_vApplicationIdleHook ()
    at /home/olas/esp/esp-idf/components/esp32/./freertos_hooks.c:52
This was added to call_start_cpu0(),cpu_start.c for the extra user code start info
    int test= (void (*)(void))REG_READ(DPORT_APPCPU_CTRL_D_REG);
    void  *user_code_start =(void *) test;
    ESP_EARLY_LOGI(TAG, "Starting app cpu, user_code_start is %p", user_code_start);
```
This seems fixed now,
https://github.com/espressif/esp-idf/commit/47b8f78cb0e15fa43647788a808dac353167a485
You should be able to run on two cores.


This is now emulated better,
```
     void esp_crosscore_int_send_yield(int coreId) {                                                                  
             assert(coreId<portNUM_PROCESSORS);                                                                       
    //Mark the reason we interrupt the other CPU   
    portENTER_CRITICAL(&reasonSpinlock);
      reason[coreId]|=REASON_YIELD; 
         portEXIT_CRITICAL(&reasonSpinlock);           
             //Poke the other CPU.                                             
      if (coreId==0) {                                           
        WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, DPORT_CPU_INTR_FROM_CPU_0);
      } else {                                                                                                        
        WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, DPORT_CPU_INTR_FROM_CPU_1);              
}

This results in

io write dc,1   if coreId==0

io write e0,1   if coreId==1

```

##  Making free_rtos tick.
Good information here,
https://github.com/espressif/esp-idf/blob/master/components/freertos/xtensa/readme_xtensa.txt

```
The timer tick is handled here,
_frxt_timer_int ()
This calls the 
xTaskIncrementTick function.

```


## Detailed boot analaysis
[More about the boot of the romdumps](./BOOT.md)


##  Patched gdb,
In the bin directory there is an improved gdb version. The following patch was applied,
https://github.com/jcmvbkbc/xtensa-toolchain-build/blob/master/fixup-gdb.sh
To use it properly you must increase the num_regs = 104 to i.e. 172 in core-esp32.c



## Another version of qemu for xtensa
The base of all this is done by Max Filippov
https://github.com/OSLL/qemu-xtensa

#Another version of qemu with esp32 exists here,
However the work is not yet finished.
   
    git clone https://github.com/OSLL/qemu-xtensa
    cd qemu-xtensa
    git checkout  xtensa-esp32
    git submodule update --init dtc
    cd ..
    mkdir build-qemu-xtensa
    cd build-qemu-xtensa
    ../qemu-xtensa/configure --disable-werror --prefix=`pwd`/root --target-list=xtensa-softmmu

#  Networking support
     There exists networking support of an emulated opencore network device,
     components/emul_ip/lwip_ethoc.c is the driver.
     All files in the net/ direcory is just for reference, they are currently not used.

     To get emulated network to work with 3.1 version of esp-idf you must patch esp-idf components/lwip/port/freertos/sys_arch.c
     sys_init(void) ...
    //esp_vfs_lwip_sockets_register();


```  
     Dont try running   emulated_net(); on actual hardware. 
     Probably not harmful but I put the emulated hardware here, i uesd the DR_REG_EMAC_BASE

  #define	OC_BASE          0x3ff69000
  #define	OC_DESC_START    0x3ff69400
  #define	OC_BUF_START     0x3FFF8000          //pool 6 blk 1 <- can be used as trace memory

  TODO! Check if it is safe to use this memory?? 
  This could possibly screw up your data.

  Note this one  components/esp32/include/soc/soc.h
     DR_REG_EMAC_BASE                        0x3ff69000
     Here are the pins needed to get ethernet support.
     http://www.smartarduino.com/esp32/esp32_chip_pin_list_en.pdf
     Note that this could be outdated.

```  

```  
The examples/14_basic_webserver worked when I tried it,
Start qemu like this,
 xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10080-192.168.4.3:80  -net dump,file=/tmp/vm0.pcap  -s    > io.txt 

Then start browser on http://127.0.0.1:10080

```  

[Networking options in qemu](./QEMU_NET.md)

## Remote debugging with gdbstub.c
It is a good idea to save the original  xtensa-esp32-elf-gdb as the one in the bin directory works best with qemu

```    
  Component config  --->
       ESP32-specific config  --->  
             Panic handler behaviour (Invoke GDBStub)  --->   
```

    esp32.c creates a 3 threads with socket on port 8880-8882 tto allow connect to the serial port over a socket.
    This allows connecting to panic handler gdbstub. You can also use this to debug the debugstub code.
``` 
    xtensa-esp32-elf-gdb.sav  build/app-template.elf  -ex 'target remote:8880'
    Another solution if you are running qemu-gdb is to set a breakpoint in the stub.
    (gdb)b esp_gdbstub_panic_handler 
    or if you use default settings
    (gdb)b commonErrorHandler
    (gdb) where (or up) will show you what the problem is


    (gdb)b _xt_panic

```
## Analysis of an exception
[Mostly assembly dumps](./EXCEPTION.md)

    This gdbstub panic handler is also nice to have when running on target.
    xtensa-esp32-elf-gdb   build/app-template.elf   -b 115200 -ex 'target remote /dev/ttyUSB0'
   
## Memory access breakpoints.

I noticed that memory got overwritten as priv_ethoc suddenly contained faulty iobase.
Memory access breakpoints comes very handy for these tyoes of errors.
```
To find a variable based on location in memory
(gdb) info symbol 0x40154e6c

To track memory writes
(gdb) watch priv_ethoc.io_base
To track memory reads
(gdb) rwatch priv_ethoc.io_base
To track memory reads and writes
(gdb) awatch *0x3ffe0400
Together with add-symbol-file rom.elf 0x40000000 this allows you to easy find where  a register is accessed.
Here is an important register. DPORT_PRO_CACHE_CTRL_REG
(gdb) awatch *0x3ff00040
(gdb) info watchpoints
Dont forget that you can reset qemu hardware and start again.
```      
## esp_intr_alloc
In order to invoke the correct interrupts this is some useful info
```      
esp_intr_alloc(ETS_TIMER1_INTR_SOURCE, 0, &frc_timer_isr, NULL, NULL);
io write 1e4,2
esp_intr_alloc(ETS_TG0_WDT_LEVEL_INTR_SOURCE, 0, task_wdt_isr, NULL, NULL); 
io write 144,3 
ret = esp_intr_alloc(ETS_I2C_EXT0_INTR_SOURCE, intr_alloc_flags, fn, arg, handle); 
int intr=get_free_int(flags, cpu, force);
intr=12
xt_set_interrupt_handler(intr, handler, arg); 
intr_matrix_set(cpu, source, intr);  
io write 1c8,c 


```      


## Rom symbols from rom.elf
To make debugging the rom functions there is a file rom.elf that contains debug information for the rom file. 
It was created with the help from this project, https://github.com/jcmvbkbc/esp-elf-rom
This rom.elf also works with the original gdb with panic handler gdbstub.

```      
        xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/build/app-template.elf  -s -S > io.txt
        xtensa-esp32-elf-gdb   build/app-template.elf  -ex 'target remote:1234'
       (gdb) add-symbol-file rom.elf 0x40000000
       (gdb) b start_cpu0_default
       (gdb) c
       (gdb) b app_main
```

##  .gdbinit freertos_show_threads 
The .gdbinit file contains code to list all tasks, freertos_show_threads 
It will list all the tasks, 
  hande,name,core,stack,stack_usage
It will also list current location of the $pc
Take care that because of how the Xtensa core works, the leftmost digit of the hex number may be off and you have to manually correct it to '4'. For example, for $pc = 0x800820a2, you would want to look up 0x400820a2.
list *0x400820a2, then you can see what the ipc0 thread is doing.
Note that --terminating-- , in this example it is empty, no threads are terminating.

```
(gdb) freertos_show_threads 
0x3ffb80a0	IDLE	0x0	0x3ffb7b44	0x3ffb7f00		956
   0x400d0d9e <esp_task_wdt_feed+6>:	or	a2, a10, a10
   0x400d0da1 <esp_task_wdt_feed+9>:	l32r	a3, 0x400d00c0 <_stext+168>
   0x400d0da4 <esp_task_wdt_feed+12>:	l32i	a3, a3, 0
0x3ffb78b4	main	0x0	0x3ffb6898	0x3ffb7750		3768
$1 = 0x80083070
0x3ffb9798	tiT		0x7fffffff	0x3ffb8f7c	0x3ffb9620	1700
$2 = 0x800820a2
0x3ffba140	poll_task	0x7fffffff	0x3ffb9924	0x3ffba060	1852
$3 = 0x80083070
0x3ffb8890	Tmr Svc		0x0		0x3ffb8334	0x3ffb8790		1116
$4 = 0x80083b9e
--suspended--
0x3ffbaae8	echo_thread	0x7fffffff	0x3ffba2cc	0x3ffba8b0	1508
$5 = 0x800820a2
0x3ffb670c	ipc0		0x0		0x3ffb61b0	0x3ffb6600	1104
$6 = 0x800820a2
--terminating--
```


# The ESP32 memory layout

The MMU will map flash data/instruction to the processors depending on how these are set.
```
/* Flash MMU table for PRO CPU */
//#define DPORT_PRO_FLASH_MMU_TABLE ((volatile uint32_t*) 0x3FF10000)
4 regions with 64 


/* Flash MMU table for APP CPU */
//#define DPORT_APP_FLASH_MMU_TABLE ((volatile uint32_t*) 0x3FF12000)
```

```
MMU_BLOCK0_VADDR 0x3f400000    // Dont use this one. Must be mapped to flash.rodata 
MMU_BLOCK1_VADDR 0x3f410000
..

MMU_BLOCK50_VADDR 0x3f720000
..
MMU_BLOCK63_VADDR 0x3fA30000

qemu emulates flash mapping by looking at the X_FLASH_MMU_TABLE[]
A value of 2 in DPORT_PRO_FLASH_MMU_TABLE[1] means that 
contents of the flash file at location 0x20000-0x30000 will be mapped to virtual address 0x3f410000-0x3f420000
I think that from BLOCK50 and up  memory is mapped as instructions. BLOCK0-49 is data.

```
# Flash layout single app (subject to change)
```

Offset, length,    name  ,  data  
 0x1000  , 2000          ,    Bootloader
 0x8000  , >100          ,    Partition table
 0x9000  , 6000    nvs   ,    wifi auth data 
 0xf000  , 1000    phy_init   rf data, calibration data
 0x10000 , >50000 factory,    application
0x3E8000 is the end for a 4MB flash

Old locations? Used in olfer version of the idf.
  0x6000 , 6000    nvs   ,   wifi auth data 

Note that the esp32 can save rf calibration data for subsequent startups

qemu uses a simplified memory map, (subject to change)

            0x2000_0000 - 0x4000_0000  ram
            0x4000_0000 - 0x40BF_FFFF  ram1

            0x3ff0_0000 - 0x3ff0_0000   system_io
The rom dumps are just copied into ram, but is probably protected.
Also some of the ram was also dumped and is restored, this is probably a good thing.	    
            0x3FF9_0000 - 0x3FF9_FFFF   rom1.bin 
            0x4000_0000 - 0x40c1_FFFF   rom.bin

            0x5000_0000 - 0x5008_0000  ulp ram
            0x6000_0000 -              wifi io, undocumented i/o
```

flash mmu emulation is done by copying from the file esp32flash.bin when the
registers DPORT_PRO_FLASH_MMU_TABLE are written to.

# Embedded memory.

```
ROM0  384 KB   0x4000_0000 0x4005_FFFF    Static MPU
ROM1  64 KB    0x3FF9_0000 0x3FF9_FFFF    Static MPU
SRAM0 64 KB    0x4007_0000 0x4007_FFFF    Static MPU
      128 KB   0x4008_0000 0x4009_FFFF    SRAM0 MMU
SRAM1 128 KB   0x3FFE_0000 0x3FFF_FFFF    Static MPU
      128 KB   0x400A_0000 0x400B_FFFF    Static MPU
SRAM2 72 KB    0x3FFA_E000 0x3FFB_FFFF    Static MPU
      128 KB   0x3FFC_0000 0x3FFD_FFFF    SRAM2 MMU
RTC FAST 8 KB  0x3FF8_0000 0x3FF8_1FFF    RTC FAST MPU
       8 KB    0x400C_0000 0x400C_1FFF    RTC FAST MPU
RTC SLOW 8 KB  0x5000_0000 0x5000_1FFF    RTC SLOW MPU

```

The on-chip memory is governed by fixed-function MPUs, configurable MPUs, and MMUs:

Both the upper 128 KB of SRAM0 and the upper 128 KB of SRAM2 are governed by an MMU. 
      128 KB   0x4008_0000 0x4009_FFFF    SRAM0 MMU
      128 KB   0x3FFC_0000 0x3FFD_FFFF    SRAM2 MMU


The internal RAM MMUs divide the memory range they govern into 16 pages. The
page size is configurable as 8 KB, 4 KB and 2 KB. When the page size is 8 KB, the 16 pages span the entire 128
KB memory region; when the page size is 4 KB or 2 KB, a non-MMU-covered region of 64 or 96 KB,
respectively, will exist at the end of the memory space. 

The address range of the first 32 KB of the ROM 0 (0x4000_0000 ~ 0x4000_7FFF) can be remapped in order to
access a part of Internal SRAM 1 that normally resides in a memory range of 0x400B_0000 ~ 0x400B_7FFF.
While remapping, the 32 KB SRAM cannot be accessed by an address range of 0x400B_0000 ~ 0x400B_7FFF
any more, but it can still be accessible through the data bus (0x3FFE_8000 ~ 0x3FFE_FFFF



#Layout

```

Bus Type   |  Boundary Address     |     Size | Target
            Low Address | High Add 
            0x0000_0000  0x3F3F_FFFF             Reserved 
Data        0x3F40_0000  0x3F7F_FFFF   4 MB      External Memory
Data        0x3F80_0000  0x3FBF_FFFF   4 MB      External Memory
            0x3FC0_0000  0x3FEF_FFFF   3 MB      Reserved
Data        0x3FF0_0000  0x3FF7_FFFF  512 KB     Peripheral (io)
Data        0x3FF8_0000  0x3FFF_FFFF  512 KB     Embedded Memory
Instruction 0x4000_0000  0x400C_1FFF  776 KB     Embedded Memory rom
Instruction 0x400C_2000  0x40BF_FFFF  11512 KB   External Memory
            0x40C0_0000  0x4FFF_FFFF  244 MB     Reserved
            0x5000_0000 0x5000_1FFF   8 KB       Embedded Memory
            0x5000_2000 0xFFFF_FFFF              Reserved

```

The linker scripts contains information of how to layout the code and variables.
componenets/esp32/ld
```
  /* IRAM for PRO cpu. Not sure if happy with this, this is MMU area... */
  iram0_0_seg (RX) :                 org = 0x40080000, len = 0x20000
```

Monitor info tree from espressifs qemu
```
memory-region: system
  0000000000000000-ffffffffffffffff (prio 0, i/o): system
    000000003ff00000-000000003ff3ffff (prio 0, i/o): misc.esp32.dport
      000000003ff000dc-000000003ff000eb (prio -1, i/o): misc.esp32.crosscoreint
      000000003ff00104-000000003ff0032b (prio -1, i/o): misc.esp32.intmatrix
    000000003ff03000-000000003ff030bf (prio 0, i/o): misc.esp32.sha
    000000003ff40000-000000003ff4007b (prio 0, i/o): esp_soc.uart
    000000003ff42000-000000003ff42fff (prio 0, i/o): ssi.esp32.spi
    000000003ff43000-000000003ff43fff (prio 0, i/o): ssi.esp32.spi
    000000003ff44000-000000003ff44fff (prio 0, i/o): esp32.gpio
    000000003ff47000-000000003ff47013 (prio 0, i/o): timer.esp32.frc
    000000003ff47020-000000003ff47033 (prio 0, i/o): timer.esp32.frc
    000000003ff48000-000000003ff4813f (prio 0, i/o): misc.esp32.rtc_cntl
    000000003ff48400-000000003ff487ff (prio -1000, i/o): esp32.rtcio
    000000003ff48800-000000003ff48bff (prio -1000, i/o): esp32.rtcio
    000000003ff49000-000000003ff4afff (prio -1000, i/o): esp32.iomux
    000000003ff4b000-000000003ff4bfff (prio -1000, i/o): esp32.hinf
    000000003ff4e000-000000003ff4efff (prio -1000, i/o): esp32.analog
    000000003ff4f000-000000003ff4ffff (prio -1000, i/o): esp32.i2s0
    000000003ff50000-000000003ff5007b (prio 0, i/o): esp_soc.uart
    000000003ff53000-000000003ff53fff (prio -1000, i/o): esp32.i2c0
    000000003ff55000-000000003ff55fff (prio -1000, i/o): esp32.slchost
    000000003ff58000-000000003ff58fff (prio -1000, i/o): esp32.slc
    000000003ff5a000-000000003ff5a1ff (prio 0, i/o): nvram.esp32.efuse
    000000003ff5f000-000000003ff5f0ff (prio 0, i/o): timer.esp32.timg
    000000003ff60000-000000003ff600ff (prio 0, i/o): timer.esp32.timg
    000000003ff64000-000000003ff64fff (prio 0, i/o): ssi.esp32.spi
    000000003ff65000-000000003ff65fff (prio 0, i/o): ssi.esp32.spi
    000000003ff66000-000000003ff66fff (prio -1000, i/o): esp32.apbctrl
    000000003ff67000-000000003ff67fff (prio -1000, i/o): esp32.i2c1
    000000003ff69000-000000003ff69053 (prio 0, i/o): open_eth.regs
    000000003ff69400-000000003ff697ff (prio 0, i/o): open_eth.desc
    000000003ff6d000-000000003ff6dfff (prio -1000, i/o): esp32.i2s1
    000000003ff6e000-000000003ff6e07b (prio 0, i/o): esp_soc.uart
    000000003ff75144-000000003ff75147 (prio 0, i/o): misc.esp32.rng
    000000003ff90000-000000003ff9ffff (prio 0, i/o): alias esp32.drom @esp32.irom 0000000000060000-000000000006ffff
    000000003ffae000-000000003fffffff (prio 0, ram): esp32.dram
    0000000040000000-000000004006ffff (prio 0, rom): esp32.irom
    0000000040070000-0000000040077fff (prio 0, ram): esp32.icache0
    0000000040078000-000000004007ffff (prio 0, ram): esp32.icache1
    0000000040080000-000000004009ffff (prio 0, ram): esp32.iram
    0000000050000000-0000000050001fff (prio 0, ram): esp32.rtcslow
    000000005ffc3000-000000005ffc30bf (prio 0, i/o): alias mr-apb-0x3ff03000 @misc.esp32.sha 0000000000000000-00000000000000bf
    0000000060000000-000000006000007b (prio 0, i/o): alias mr-apb-0x3ff40000 @esp_soc.uart 0000000000000000-000000000000007b
    0000000060002000-0000000060002fff (prio 0, i/o): alias mr-apb-0x3ff42000 @ssi.esp32.spi 0000000000000000-0000000000000fff
    0000000060003000-0000000060003fff (prio 0, i/o): alias mr-apb-0x3ff43000 @ssi.esp32.spi 0000000000000000-0000000000000fff
    0000000060004000-0000000060004fff (prio 0, i/o): alias mr-apb-0x3ff44000 @esp32.gpio 0000000000000000-0000000000000fff
    0000000060007000-0000000060007013 (prio 0, i/o): alias mr-apb-0x3ff47000 @timer.esp32.frc 0000000000000000-0000000000000013
    0000000060007020-0000000060007033 (prio 0, i/o): alias mr-apb-0x3ff47020 @timer.esp32.frc 0000000000000000-0000000000000013
    0000000060008000-000000006000813f (prio 0, i/o): alias mr-apb-0x3ff48000 @misc.esp32.rtc_cntl 0000000000000000-000000000000013f
    0000000060008400-00000000600087ff (prio -1000, i/o): esp32.rtcio-apb
    0000000060008800-0000000060008bff (prio -1000, i/o): esp32.rtcio-apb
    0000000060009000-000000006000afff (prio -1000, i/o): esp32.iomux-apb
    000000006000b000-000000006000bfff (prio -1000, i/o): esp32.hinf-apb
    000000006000e000-000000006000efff (prio -1000, i/o): esp32.analog-apb
    000000006000f000-000000006000ffff (prio -1000, i/o): esp32.i2s0-apb
    0000000060010000-000000006001007b (prio 0, i/o): alias mr-apb-0x3ff50000 @esp_soc.uart 0000000000000000-000000000000007b
    0000000060013000-0000000060013fff (prio -1000, i/o): esp32.i2c0-apb
    0000000060015000-0000000060015fff (prio -1000, i/o): esp32.slchost-apb
    0000000060018000-0000000060018fff (prio -1000, i/o): esp32.slc-apb
    000000006001a000-000000006001a1ff (prio 0, i/o): alias mr-apb-0x3ff5a000 @nvram.esp32.efuse 0000000000000000-00000000000001ff
    000000006001f000-000000006001f0ff (prio 0, i/o): alias mr-apb-0x3ff5f000 @timer.esp32.timg 0000000000000000-00000000000000ff
    0000000060020000-00000000600200ff (prio 0, i/o): alias mr-apb-0x3ff60000 @timer.esp32.timg 0000000000000000-00000000000000ff
    0000000060024000-0000000060024fff (prio 0, i/o): alias mr-apb-0x3ff64000 @ssi.esp32.spi 0000000000000000-0000000000000fff
    0000000060025000-0000000060025fff (prio 0, i/o): alias mr-apb-0x3ff65000 @ssi.esp32.spi 0000000000000000-0000000000000fff
    0000000060026000-0000000060026fff (prio -1000, i/o): esp32.apbctrl-apb
    0000000060027000-0000000060027fff (prio -1000, i/o): esp32.i2c1-apb
    000000006002d000-000000006002dfff (prio -1000, i/o): esp32.i2s1-apb
    000000006002e000-000000006002e07b (prio 0, i/o): alias mr-apb-0x3ff6e000 @esp_soc.uart 0000000000000000-000000000000007b
    0000000060035144-0000000060035147 (prio 0, i/o): alias mr-apb-0x3ff75144 @misc.esp32.rng 0000000000000000-0000000000000003

memory-region: esp32.rtcfast_i
  00000000400c0000-00000000400c1fff (prio 0, ram): esp32.rtcfast_i
```



