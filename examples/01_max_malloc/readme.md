# Start qemu

xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -net user,id=simnet,ipver4=on,net=192.168.1.0/24,host=192.168.1.40,hostfwd=tcp::10077-192.168.1.3:7  -net dump,file=/tmp/vm0.pcap  -kernel  ~/esp/qemu_esp32/examples/01_max_malloc/build/memsize.elf  -s -S > io.txt

# start gdb
xtensa-esp32-elf-gdb  build/memsize.elf -ex 'target remote:1234'


ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing heap allocator:
I (1) heap_alloc_caps: Region 19: 3FFB1E8C len 0002E174 tag 0
I (1) heap_alloc_caps: Region 25: 3FFE8000 len 00018000 tag 1
check b=0x3ffb1e98 size=188760 ok
check b=0x3ffdfff0 size=0 ok
check b=0x3ffe800c size=98276 ok
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (2) rtc: rtc v160 Nov 22 2016 19:00:05
I (2) rtc: XTAL 26M
I (4) cpu_start: Starting scheduler on PRO CPU.
starting
ALLOCS: 1/20, SIZE 131072/131072
ALLOCS: 2/20, SIZE 65536/196608
ALLOCS: 3/20, SIZE 32768/229376
ALLOCS: 4/20, SIZE 16384/245760
ALLOCS: 5/20, SIZE 8192/253952
ALLOCS: 6/20, SIZE 8192/262144
ALLOCS: 7/20, SIZE 4096/266240
ALLOCS: 8/20, SIZE 4096/270336
ALLOCS: 9/20, SIZE 2048/272384
ALLOCS: 10/20, SIZE 1024/273408
ALLOCS: 11/20, SIZE 512/273920
ALLOCS: 12/20, SIZE 512/274432
ALLOCS: 13/20, SIZE 256/274688
ALLOCS: 14/20, SIZE 256/274944
free idx 13
free idx 12
free idx 11
free idx 10
free idx 9
free idx 8
free idx 7
free idx 6
free idx 5
free idx 4
free idx 3
free idx 2
free idx 1
free idx 0
