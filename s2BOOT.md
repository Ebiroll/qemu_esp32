```
ead RTC_CNTL_RESET_STATE_REG
ESP-ROM:esp32s2-rc4-20191025
Build:Oct 25 2019
rst:0x1 (POWERON),boot:0xa (SPI_FAST_FLASH_BOOT)
unimp write  00001200,00008000
MMU Map flash 00000000 to 3F000000
FFFFFFFF,FFFFFFFF,b 100204E9,p 020150AA
SPIWP:0xee
mode:DIO, clock div:2
load:0x3ffe8100,len:0x4
load:0x3ffe8104,len:0x21d4
load:0x40050000,len:0x1634
load:0x40054000,len:0x2580
qemu-system-xtensa: esp32_sha_len:32
entry 0x40050318
/home/olof/esp32s2/hw/misc/esp32_rtc_cntl.c:164:23: runtime error: index 1 out of bounds for type '_Bool [1]'
/home/olof/esp32s2/hw/misc/esp32_rtc_cntl.c:167:57: runtime error: index 1 out of bounds for type '_Bool [1]'
/home/olof/esp32s2/hw/misc/esp32_rtc_cntl.c:167:34: runtime error: index 1 out of bounds for type 'IRQState *[1]'
I (204) boot: ESP-IDF v4.2-dev-1660-g7d7521367 2nd stage bootloader
I (215) boot: compile time 00:07:35
D (219) bootloader_flash: mmu set block paddr=0x00000000 (was 0xffffffff)
unimp write  000012FC,00008000
MMU Map flash 00000000 to 3F3F0000
FFFFFFFF,FFFFFFFF,b 100204E9,p 020150AA
I (226) boot: chip revision: 0
D (230) boot.esp32s2: magic e9
D (231) boot.esp32s2: segments 04
D (232) boot.esp32s2: spi_mode 02
D (232) boot.esp32s2: spi_speed 00
D (233) boot.esp32s2: spi_size 01
I (234) boot.esp32s2: SPI Speed      : 40MHz
I (235) boot.esp32s2: SPI Mode       : DIO
I (236) boot.esp32s2: SPI Flash Size : 2MB
read RTC_CNTL_RESET_STATE_REG
D (240) boot: Enabling RTCWDT(9000 ms)
I (245) boot: Enabling RNG early entropy source...
D (249) bootloader_flash: mmu set paddr=00000000 count=1 size=c00 src_addr=8000 src_addr_aligned=0
unimp write  00001200,00008000
MMU Map flash 00000000 to 3F000000
FFFFFFFF,FFFFFFFF,b 100204E9,p 020150AA
D (252) boot: mapped partition table 0x8000 at 0x3f008000
D (263) flash_parts: partition table verified, 4 entries
I (264) boot: Partition Table:
I (265) boot: ## Label            Usage          Type ST Offset   Length
D (267) boot: load partition table entry 0x3f008000
D (269) boot: type=1 subtype=2
I (270) boot:  0 nvs              WiFi data        01 02 00009000 00006000
D (272) boot: load partition table entry 0x3f008020
D (273) boot: type=1 subtype=1
I (274) boot:  1 phy_init         RF data          01 01 0000f000 00001000
D (275) boot: load partition table entry 0x3f008040
D (276) boot: type=0 subtype=0
I (277) boot:  2 factory          factory app      00 00 00010000 00100000
I (279) boot: End of partition table
read RTC_CNTL_RESET_STATE_REG
D (282) boot: Trying partition index -1 offs 0x10000 size 0x100000
D (285) esp_image: reading image header @ 0x10000
D (286) bootloader_flash: mmu set block paddr=0x00010000 (was 0xffffffff)
unimp write  000012FC,00008001
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
D (290) esp_image: image header: 0xe9 0x06 0x02 0x01 40025524
V (292) esp_image: loading segment header 0 at offset 0x10018
V (295) esp_image: segment data length 0x576c data starts 0x10020
V (296) esp_image: segment 0 map_segment 1 segment_data_offs 0x10020 load_addr 0x3f000020
read RTC_CNTL_RESET_STATE_REG
I (300) esp_image: segment 0: paddr=0x00010020 vaddr=0x3f000020 size=0x0576c ( 22380) map
D (302) esp_image: free data page_count 0x0000003f
D (304) bootloader_flash: mmu set paddr=00010000 count=1 size=576c src_addr=10020 src_addr_aligned=10000
unimp write  00001200,00008001
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
V (317) esp_image: loading segment header 1 at offset 0x1578c
D (318) bootloader_flash: mmu set block paddr=0x00010000 (was 0xffffffff)
unimp write  000012FC,00008001
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
V (320) esp_image: segment data length 0x1e74 data starts 0x15794
V (322) esp_image: segment 1 map_segment 0 segment_data_offs 0x15794 load_addr 0x3ffbe150
read RTC_CNTL_RESET_STATE_REG
I (324) esp_image: segment 1: paddr=0x00015794 vaddr=0x3ffbe150 size=0x01e74 (  7796) load
D (329) esp_image: free data page_count 0x0000003f
D (331) bootloader_flash: mmu set paddr=00010000 count=1 size=1e74 src_addr=15794 src_addr_aligned=10000
unimp write  00001200,00008001
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
V (337) esp_image: loading segment header 2 at offset 0x17608
D (338) bootloader_flash: mmu set block paddr=0x00010000 (was 0xffffffff)
unimp write  000012FC,00008001
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
V (339) esp_image: segment data length 0x404 data starts 0x17610
V (340) esp_image: segment 2 map_segment 0 segment_data_offs 0x17610 load_addr 0x40024000
read RTC_CNTL_RESET_STATE_REG
I (343) esp_image: segment 2: paddr=0x00017610 vaddr=0x40024000 size=0x00404 (  1028) load
D (345) esp_image: free data page_count 0x0000003f
D (346) bootloader_flash: mmu set paddr=00010000 count=1 size=404 src_addr=17610 src_addr_aligned=10000
unimp write  00001200,00008001
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
V (348) esp_image: loading segment header 3 at offset 0x17a14
D (349) bootloader_flash: mmu set block paddr=0x00010000 (was 0xffffffff)
unimp write  000012FC,00008001
MMU Map flash 00010000 to 3F3F0000
100206E9,40025524,b 632F6664,p 51EB851F
V (350) esp_image: segment data length 0x85fc data starts 0x17a1c
V (351) esp_image: segment 3 map_segment 0 segment_data_offs 0x17a1c load_addr 0x40024404
read RTC_CNTL_RESET_STATE_REG
I (353) esp_image: segment 3: paddr=0x00017a1c vaddr=0x40024404 size=0x085fc ( 34300) load
D (354) esp_image: free data page_count 0x0000003f
D (355) bootloader_flash: mmu set paddr=00010000 count=2 size=85fc src_addr=17a1c src_addr_aligned=10000
unimp write  00001200,00008001
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
unimp write  00001204,00008002
MMU Map flash 00020000 to 3F010000
3350FFAD,0020C010,b 8A88000A,p 23571A62
V (367) esp_image: loading segment header 4 at offset 0x20018
D (368) bootloader_flash: mmu set block paddr=0x00020000 (was 0xffffffff)
unimp write  000012FC,00008002
MMU Map flash 00020000 to 3F3F0000
3350FFAD,0020C010,b 8A88000A,p 23571A62
V (370) esp_image: segment data length 0x147f8 data starts 0x20020
V (371) esp_image: segment 4 map_segment 1 segment_data_offs 0x20020 load_addr 0x40080020
read RTC_CNTL_RESET_STATE_REG
I (372) esp_image: segment 4: paddr=0x00020020 vaddr=0x40080020 size=0x147f8 ( 83960) map
D (374) esp_image: free data page_count 0x0000003f
D (375) bootloader_flash: mmu set paddr=00020000 count=2 size=147f8 src_addr=20020 src_addr_aligned=20000
unimp write  00001200,00008002
MMU Map flash 00020000 to 3F000000
3350FFAD,0020C010,b 8A88000A,p 23571A62
unimp write  00001204,00008003
MMU Map flash 00030000 to 3F010000
BF65A421,1E3A56FA,b 00236507,p FFFFFFFF
V (404) esp_image: loading segment header 5 at offset 0x34818
D (405) bootloader_flash: mmu set block paddr=0x00030000 (was 0xffffffff)
unimp write  000012FC,00008003
MMU Map flash 00030000 to 3F3F0000
BF65A421,1E3A56FA,b 00236507,p FFFFFFFF
V (407) esp_image: segment data length 0x1748 data starts 0x34820
V (409) esp_image: segment 5 map_segment 0 segment_data_offs 0x34820 load_addr 0x4002ca00
read RTC_CNTL_RESET_STATE_REG
I (411) esp_image: segment 5: paddr=0x00034820 vaddr=0x4002ca00 size=0x01748 (  5960) load
D (413) esp_image: free data page_count 0x0000003f
D (413) bootloader_flash: mmu set paddr=00030000 count=1 size=1748 src_addr=34820 src_addr_aligned=30000
unimp write  00001200,00008003
MMU Map flash 00030000 to 3F000000
BF65A421,1E3A56FA,b 00236507,p FFFFFFFF
V (418) esp_image: image start 0x00010000 end of last section 0x00035f68
D (420) bootloader_flash: mmu set block paddr=0x00030000 (was 0xffffffff)
unimp write  000012FC,00008003
MMU Map flash 00030000 to 3F3F0000
BF65A421,1E3A56FA,b 00236507,p FFFFFFFF
qemu-system-xtensa: esp32_sha_len:32
D (426) boot: Calculated hash: 0000000000000000000000000000000000000000000000000000000000000000
D (428) bootloader_flash: mmu set paddr=00030000 count=1 size=20 src_addr=35f70 src_addr_aligned=30000
unimp write  00001200,00008003
MMU Map flash 00030000 to 3F000000
BF65A421,1E3A56FA,b 00236507,p FFFFFFFF
D (430) bootloader_flash: mmu set paddr=00030000 count=1 size=20 src_addr=35f70 src_addr_aligned=30000
unimp write  00001200,00008003
MMU Map flash 00030000 to 3F000000
BF65A421,1E3A56FA,b 00236507,p FFFFFFFF
read RTC_CNTL_RESET_STATE_REG
read RTC_CNTL_RESET_STATE_REG
read RTC_CNTL_RESET_STATE_REG
read RTC_CNTL_RESET_STATE_REG
read RTC_CNTL_RESET_STATE_REG
read RTC_CNTL_RESET_STATE_REG
I (435) boot: Loaded app from partition at offset 0x10000
D (575) boot: Mapping segment 0 as DROM
D (676) boot: Mapping segment 4 as IROM
D (727) boot: calling set_cache_and_start_app
D (748) boot: configure drom and irom and start
V (1165) boot: d mmu set paddr=00010000 vaddr=3f000000 size=22380 n=1
unimp write  00001200,00008001
MMU Map flash 00010000 to 3F000000
100206E9,40025524,b 632F6664,p 51EB851F
V (1198) boot: rc=0
V (1217) boot: i mmu set paddr=00020000 vaddr=40080000 size=83960 n=2
V (1256) boot: rc=0
D (1286) boot: start: 0x40025524


gdb) load build/led.elf 
Loading section .flash.rodata, size 0x576c lma 0x3f000020
Loading section .dram0.data, size 0x1e74 lma 0x3ffbe150
Loading section .iram0.vectors, size 0x403 lma 0x40024000
Loading section .iram0.text, size 0x9d44 lma 0x40024404
Loading section .flash.text, size 0x147f7 lma 0x40080020
Start address 0x40025524, load size 155422
Transfer rate: 4336 KB/sec, 1967 bytes/write.


I (1375) cache: Instruction cache 	: size 8KB, 4Ways, cache line size 32Byte
I (1392) cpu_start: Pro cpu up.
I (1434) cpu_start: Application information:
I (1463) cpu_start: Project name:     led
I (1498) cpu_start: App version:      f558dab-dirty
I (1535) cpu_start: Compile time:     May 31 2020 00:07:36
I (1584) cpu_start: ELF file SHA256:  0000000000000000...
I (1618) cpu_start: ESP-IDF:          v4.2-dev-1660-g7d7521367
I (1654) cpu_start: Single core mode
I (1693) heap_init: Initializing. RAM available for dynamic allocation:
I (1707) heap_init: At 3FFC07C8 len 0003B838 (238 KiB): DRAM
I (1710) heap_init: At 3FFFC000 len 00003A10 (14 KiB): DRAM
I (1723) cpu_start: Pro cpu start user code
read RTC_CNTL_RESET_STATE_REG
/home/olof/esp32s2/hw/misc/esp32_rtc_cntl.c:52:13: runtime error: index 1 out of bounds for type 'Esp32ResetCause [1]'
W (3596) rtc_init: o_code calibration fail
read RTC_CNTL_RESET_STATE_REG
```