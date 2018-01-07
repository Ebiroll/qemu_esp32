# Non-Volatile Storage (NVS) Read and Write Example

Demonstrates how to read and write a single integer value using NVS.
The value holds the number of ESP32 module restarts. Since it is written to NVS, the value is preserved between restarts.
Example also shows how to check if read / write operation was successful, or certain value is not initialized in NVS. Diagnostic is provided in plain text to help track program flow and capture any issues on the way.
Check another example *08_nvs_rw_blob*, that shows how to read and write variable length binary data (blob).

Detailed functional description of NVS and API is provided in 
[documentation](http://esp-idf.readthedocs.io/en/latest/api/nvs_flash.html).

See the README.md file in the upper level 'examples' directory for more information about examples.

If you want to step into the spi driver you can experiment with this,
  [ ] Enable SPI flash ROM driver patched functions
It used to run in qemu but it stopped working at some point in time.


When running in qemu, first time:
```
 xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -kernel  ~/esp/qemu_esp32/examples/07_nvs_rw_value/build/nvs-rw-value.elf  -s   > io.txt 
TRYING to MAP esp32flash.binMAPPED esp32flash.binets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB4DD0 len 0002B230 (172 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 400931C4 len 0000CE3C (51 KiB): IRAM
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
TBD(pc = 40080d3a): /home/olas/qemu-xtensa-esp32/target-xtensa/translate.c:1404
I (4) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00007000 to memory, 3F407000
(qemu) Flash partition data is loaded.

Opening Non-Volatile Storage (NVS) ... Done
Reading restart counter from NVS ... The value is not initialized yet!
Updating restart counter in NVS ... Done
Committing updates in NVS ... Done

Restarting in 10 seconds...
Restarting in 9 seconds...
Restarting in 8 seconds...
Restarting in 7 seconds...
Restarting in 6 seconds...
Restarting in 5 seconds...
Restarting in 4 seconds...
Restarting in 3 seconds...
Restarting in 2 seconds...
Restarting in 1 seconds...
Restarting in 0 seconds...
Restarting now.
E (112543) wifi: esp_wifi_stop 786 wifi is not init

qemu-system-xtensa: terminating on signal 2

```
When running in qemu, second time:
Same result, value is not stored. :-(
Now problem is fixed but fis fix good? See further down.
I write 32 bytes when 
status wren    
```
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB4DD0 len 0002B230 (172 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 400931C4 len 0000CE3C (51 KiB): IRAM
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (4) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00008000 to memory, 3F408000

Opening Non-Volatile Storage (NVS) ... Done
Reading restart counter from NVS ... Done
Restart counter = 1
Updating restart counter in NVS ... Done
Committing updates in NVS ... Done

Restarting in 10 seconds...
Restarting in 9 seconds...
Restarting in 8 seconds...

```

Works 13 times then we get this error. Try resetting 13 times.
```
assertion "state == EntryState::WRITTEN || state == EntryState::EMPTY" failed: file "/home/olas/esp/esp-idf/components/nvs_flash/src/nvs_page.cpp", line 315, function: esp_err_t nvs::Page::eraseEntryAndSpan(size_t)
abort() was called at PC 0x400e099f
Guru Meditation Error: Core  0 panic'ed (abort)
```


```
Problem was here, wher is length??
Only 4 bytes written,
0 esp32_spi_write: +0x80 = 0xfe82aaaa
0x00, CMD_REG , should only write bit 18 
0xf8  SPI_EXT2_REG


0 esp32_spi_read: +0x1c: 0x50000000
0 esp32_spi_write: +0x1c = 0x50000000
written
0 esp32_spi_read: +0x20: 0x5c000000
0 esp32_spi_write: +0x20 = 0x5c000000
written
0 esp32_spi_read: +0xf8: 0x00000000
1 esp32_spi_read: +0xf8: 0x00000000
0 esp32_spi_write: +0x10 = 0x00000000
0 esp32_spi_write: +0x00 = 0x08000000
0 esp32_spi_read: +0x00: 0x00000000
0 esp32_spi_read: +0x10: 0x00000002
0 esp32_spi_write: +0x04 = 0x0400c03c
Address esp32_spi_write_address: TX 0000c03c[4 reserved]
0 esp32_spi_write: +0x80 = 0xfe82aaaa
written
0 esp32_spi_read: +0xf8: 0x00000000
1 esp32_spi_read: +0xf8: 0x00000000
0 esp32_spi_write: +0x10 = 0x00000000
0 esp32_spi_write: +0x00 = 0x08000000
0 esp32_spi_read: +0x00: 0x00000000
0 esp32_spi_read: +0x10: 0x00000002
0 esp32_spi_write: +0x00 = 0x40000000
status wren
0 esp32_spi_read: +0x00: 0x00000000
0 esp32_spi_write: +0x10 = 0x00000000
0 esp32_spi_write: +0x00 = 0x08000000
0 esp32_spi_read: +0x00: 0x00000000
0 esp32_spi_read: +0x10: 0x00000002
0 esp32_spi_write: +0x00 = 0x02000000
0 esp32_spi_read: +0x00: 0x00000000
0 esp32_spi_read: +0xf8: 0x00000000
1 esp32_spi_read: +0xf8: 0x00000000
0 esp32_spi_write: +0x10 = 0x00000000
0 esp32_spi_write: +0x00 = 0x08000000
0 esp32_spi_read: +0x00: 0x00000000
0 esp32_spi_read: +0x10: 0x00000002
```
