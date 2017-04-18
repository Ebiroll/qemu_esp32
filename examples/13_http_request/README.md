# HTTP Request Example

Uses a POSIX socket to make a very simple HTTP request.

See the README.md file in the upper level 'examples' directory for more information about examples.

Does not quite run with emulated network,


xtensa-softmmu/qemu-system-xtensa -d unimp -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10023-192.168.4.3:10023 -net dump,file=/tmp/vm0.pcap   -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/examples/13_http_request/build/http-request.elf  -s   > io.txt



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
