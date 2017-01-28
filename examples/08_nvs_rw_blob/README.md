# Non-Volatile Storage (NVS) Read and Write Example

Demonstrates how to read and write a single integer value and a blob (binary large object) using NVS to preserve them between ESP32 module restarts.

  * value - tracks number of ESP32 module soft and hard restarts.
  * blob - contains a table with module run times. The table is read from NVS to dynamically allocated RAM. New run time is added to the table on each manually triggered soft restart and written back to NVS. Triggering is done by pulling down GPIO0.

Example also shows how to implement diagnostics if read / write operation was successful. 

If not done already, consider checking simpler example *07_nvs_rw_value*, that has been used as a starting point for preparing this one. 

Detailed functional description of NVS and API is provided in [documentation](http://esp-idf.readthedocs.io/en/latest/api/nvs_flash.html).

See the README.md file in the upper level 'examples' directory for more information about examples.

When running in qemu, init phy
```
assertion "false && "item should have been present in cache"" failed: file "/home/olas/esp/esp-idf/components/nvs_flash/src/nvs_item_hash_list.cpp", line 85, function: void nvs::HashList::erase(size_t)
```

When running in qemu, first time:
```
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB4DD0 len 0002B230 (172 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 40093270 len 0000CD90 (51 KiB): IRAM
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (4) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00008000 to memory, 3F408000
Restart counter = 0
Run time:
Nothing saved yet!
Restarting...
```

Second time
```
xtensa-softmmu/qemu-system-xtensa  -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.2.0/24,host=192.168.2.40,hostfwd=tcp::10077-192.168.2.3:7  -net dump,file=/tmp/vm0.pcap  -cpu esp32 -M esp32 -m 4M -kernel  ~/esp/qemu_esp32/examples/08_nvs_rw_blob/build/nvs-rw-blob.elf -s   > io.txt 
TRYING to MAP esp32flash.binMAPPED esp32flash.binets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB4DD0 len 0002B230 (172 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 40093270 len 0000CD90 (51 KiB): IRAM
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (4) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00008000 to memory, 3F408000
Restart counter = 1
Run time:
1: 1000
Restarting...
E (10236) wifi: esp_wifi_stop 786 wifi is not init

ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
(qemu)  00000000 to memory, 3F410000
(qemu)  00000000 to memory, 3F420000
(qemu)  00000000 to memory, 3F430000
(qemu)  00000000 to memory, 3F440000
(qemu)  00000000 to memory, 3F450000
(qemu)  00000000 to memory, 3F460000
(qemu)  00007000 to memory, 3F407000
I (65991) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (65992) heap_alloc_caps: At 3FFB4DD0 len 0002B230 (172 KiB): DRAM
I (65992) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (65992) heap_alloc_caps: At 40093270 len 0000CD90 (51 KiB): IRAM
I (65992) cpu_start: Pro cpu up.
I (65992) cpu_start: Single core mode
I (65993) cpu_start: Pro cpu start user code
I (65995) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00008000 to memory, 3F408000
Restart counter = 2
Run time:
1: 1000
2: 1000
Restarting...
E (76227) wifi: esp_wifi_stop 786 wifi is not init
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
(qemu)  00000000 to memory, 3F410000
(qemu)  00000000 to memory, 3F420000
(qemu)  00000000 to memory, 3F430000
(qemu)  00000000 to memory, 3F440000
(qemu)  00000000 to memory, 3F450000
(qemu)  00000000 to memory, 3F460000
(qemu)  00007000 to memory, 3F407000
I (80719) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (80719) heap_alloc_caps: At 3FFB4DD0 len 0002B230 (172 KiB): DRAM
I (80719) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (80720) heap_alloc_caps: At 40093270 len 0000CD90 (51 KiB): IRAM
I (80720) cpu_start: Pro cpu up.
I (80720) cpu_start: Single core mode
I (80720) cpu_start: Pro cpu start user code
I (80722) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00008000 to memory, 3F408000
Restart counter = 3
Run time:
1: 1000
2: 1000
3: 1000
Restarting...
E (90955) wifi: esp_wifi_stop 786 wifi is not init


```