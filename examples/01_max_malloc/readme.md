# Start qemu

xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -net nic,model=vlan0  -net user,id=simnet,ipver4=on,net=192.168.1.0/24,host=192.168.1.40,hostfwd=tcp::10077-192.168.1.3:7  -net dump,file=/tmp/vm0.pcap  -kernel  ~/esp/qemu_esp32/examples/01_max_malloc/build/memsize.elf  -s -S > io.txt

# start gdb
xtensa-esp32-elf-gdb  build/memsize.elf -ex 'target remote:1234'

```
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
```

#After free memory patch, in december

xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -kernel ~/esp/qemu_esp32/examples/01_max_malloc/build/memsize.elf -S -s > io.txt
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB4944 len 0002B6BC (173 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 40092738 len 0000D8C8 (54 KiB): IRAM
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (4) cpu_start: Starting scheduler on PRO CPU.
starting
Free heap size 264888
ALLOCS: 1/20, SIZE 131072/131072
ALLOCS: 2/20, SIZE 65536/196608
ALLOCS: 3/20, SIZE 32768/229376
ALLOCS: 4/20, SIZE 16384/245760
ALLOCS: 5/20, SIZE 8192/253952
ALLOCS: 6/20, SIZE 4096/258048
ALLOCS: 7/20, SIZE 2048/260096
ALLOCS: 8/20, SIZE 2048/262144
ALLOCS: 9/20, SIZE 1024/263168
ALLOCS: 10/20, SIZE 512/263680
ALLOCS: 11/20, SIZE 512/264192
ALLOCS: 12/20, SIZE 256/264448
ALLOCS: 13/20, SIZE 128/264576
ALLOCS: 14/20, SIZE 128/264704
Free heap size 64
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
Free heap size 264888

