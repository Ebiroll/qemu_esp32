# HTTP Request Example

Uses a POSIX socket to make a very simple HTTP request.

See the README.md file in the upper level 'examples' directory for more information about examples.



xtensa-softmmu/qemu-system-xtensa -d unimp -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10023-192.168.4.3:10023 -net dump,file=/tmp/vm0.pcap   -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/examples/13_http_request/build/http-request.elf  -s   > io.txt


As root try,
nc -l -p 80 
Then type response


xtensa-esp32-elf-gdb.qemu   build/http-request.elf  -ex 'target remote:1234'

(gdb)b http_get_task


./qemu_flash build/http-request.bin
'''
I (12) boot: ESP-IDF v3.0-dev-49-g3c9ea3cb 2nd stage bootloader
I (12) boot: compile time 23:40:05
I (12) boot: Enabling RNG early entropy source...
I (12) boot: SPI Speed      : 40MHz
I (12) boot: SPI Mode       : DIO
I (12) boot: SPI Flash Size : 2MB
I (12) boot: Partition Table:
I (12) boot: ## Label            Usage          Type ST Offset   Length
I (13) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (13) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (13) boot:  2 factory          factory app      00 00 00010000 00100000
I (13) boot: End of partition table
I (13) boot: Disabling RNG early entropy source...
I (14) boot: Loading app partition at offset 00010000
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  3
(qemu) MMU 100c8  4
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  4
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  4
(qemu) MMU 100c8  5
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  4
(qemu) MMU 100c8  5
(qemu) MMU 100c8  6
(qemu) MMU 100c8  7
(qemu) MMU 100c8  8
(qemu) MMU 100c8  9
(qemu) MMU 100c8  1
I (319) boot: segment 0: paddr=0x00010018 vaddr=0x00000000 size=0x0ffe8 ( 65512) 
(qemu) MMU 100c8  2
I (319) boot: segment 1: paddr=0x00020008 vaddr=0x3f400010 size=0x0c5c4 ( 50628) map
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
I (319) boot: segment 2: paddr=0x0002c5d4 vaddr=0x3ffb0000 size=0x02300 (  8960) load
(qemu) MMU 10000  2
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
I (320) boot: segment 3: paddr=0x0002e8dc vaddr=0x40080000 size=0x00400 (  1024) load
(qemu) MMU 10000  2
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
I (321) boot: segment 4: paddr=0x0002ece4 vaddr=0x40080400 size=0x127b8 ( 75704) load
(qemu) MMU 10000  2
(qemu) MMU 10004  3
(qemu) MMU 10008  4
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  4
I (323) boot: segment 5: paddr=0x000414a4 vaddr=0x400c0000 size=0x00000 (     0) load
(qemu) MMU 10000  4
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  4
I (323) boot: segment 6: paddr=0x000414ac vaddr=0x00000000 size=0x0eb5c ( 60252) 
(qemu) MMU 100c8  1
(qemu) MMU 100c8  2
(qemu) MMU 100c8  4
(qemu) MMU 100c8  5
I (324) boot: segment 7: paddr=0x00050010 vaddr=0x400d0018 size=0x498d4 (301268) map
(qemu) MMU 10000  2
(qemu) MMU 10134  5
(qemu) MMU 10138  6
(qemu) MMU 1013c  7
(qemu) MMU 10140  8
(qemu) MMU 10144  9
I (324) cpu_start: Pro cpu up.
I (324) cpu_start: Single core mode
I (325) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (325) heap_alloc_caps: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (325) heap_alloc_caps: At 3FFB7B90 len 00028470 (161 KiB): DRAM
I (325) heap_alloc_caps: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (326) heap_alloc_caps: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (326) heap_alloc_caps: At 40092BB8 len 0000D448 (53 KiB): IRAM
I (326) cpu_start: Pro cpu start user code
HOST RER TBD
I (382) cpu_start: Starting scheduler on PRO CPU.
Running in qemu
ethoc: num_tx: 8 num_rx: 8
TCP/IP initializing...
TCP/IP initialized.
E (386) wifi: esp_wifi_internal_set_sta_ip 1281 wifi is not init
Applications started.
[0;31mE (386) event: system_event_sta_got_ip_default 162 esp_wifi_internal_set_sta_ip ret=12289
I (1406) example: Connected to AP
I (1406) example: DNS lookup succeeded. IP=192.168.4.40
I (1406) example: ... allocated socket

I (1416) example: ... connected
I (1416) example: ... socket send success
Hello
I (12176) example: ... done reading from socket. Last read return=0 errno=128

I (12176) example: 10... 
I (13176) example: 9... 
I (14176) example: 8... 
I (15176) example: 7... 

'''


Older log from hardware
'''
ts Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (0) cpu_start: Pro cpu up.
I (0) cpu_start: Starting app cpu, entry point is 0x40080e00
I (0) cpu_start: App cpu up.
I (8) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (8) heap_alloc_caps: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (8) heap_alloc_caps: At 3FFB79C0 len 00028640 (161 KiB): DRAM
I (9) heap_alloc_caps: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (9) heap_alloc_caps: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (9) heap_alloc_caps: At 4009AB8C len 00005474 (21 KiB): IRAM
I (9) cpu_start: Pro cpu start user code
HOST RER TBD
I (11) cpu_start: Starting scheduler on PRO CPU.
Running in qemu
ethoc: num_tx: 8 num_rx: 8
I (38) cpu_start: Starting scheduler on APP CPU.
TCP/IP initializing...
TCP/IP initialized.
Applications started.
E (38) wifi: esp_wifi_internal_set_sta_ip 1259 wifi is not init
E (38) event: system_event_sta_got_ip_default 162 esp_wifi_internal_set_sta_ip ret=12289
I (38) example: Connected to AP
I (38) example: DNS lookup succeeded. IP=93.184.216.34
I (38) example: ... allocated socket

stopping queue
'''

Need to improve network emulation to get this to work in 


It can work, sometimes if you serve the web requests like this,

  nc -l 80 < README.md

  qemu xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp -net nic,model=vlan0 -net user,ipver4=on,net=192.168.4.0/24,host=192.168.4.1,hostfwd=tcp::10090-192.168.4.3:10090 -net dump,file=/tmp/vm0.pcap  -cpu esp32 -M esp32 -m 4M  -kernel ~/esp/qemu_esp32/examples/13_http_request/build/http-request.elf -s    > io.txt 
