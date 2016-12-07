# Start qemu
xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -net user,id=simnet,ipver4=on,net=192.168.1.0/24,host=192.168.1.40,hostfwd=tcp::10077-192.168.1.3:7  -net dump,file=/tmp/vm0.pcap  -kernel  ~/esp/qemu_esp32/examples/09_onchip_sensor/build/onchip-sensor.elf  -s -S > io.txt
Warning: vlan 0 with no nics
ets Jun  8 2016 00:22:57

# start gdb
xtensa-esp32-elf-gdb  build/onchip-sensor.elf -ex 'target remote:1234'

```
rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing heap allocator:
I (1) heap_alloc_caps: Region 19: 3FFB4908 len 0002B6F8 tag 0
I (1) heap_alloc_caps: Region 25: 3FFE8000 len 00018000 tag 1
check b=0x3ffb4914 size=177884 ok
check b=0x3ffdfff0 size=0 ok
check b=0x3ffe800c size=98276 ok
I (2) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (2) rtc: rtc v160 Nov 22 2016 19:00:05
I (2) rtc: XTAL 26M
I (4) cpu_start: Starting scheduler on PRO CPU.
Welcome to Noduino Quantum
Try to read the sensors inside the ESP32 chip ... 
ESP32 onchip Temperature = 0
------
io read 4880c 
io write 4880c,c0000 
io read 4884c 
io write 4884c,20000 
io read 4884c 
io write 4884c,2000000 
io read 4884c 
io write 4884c,0 
io read 4884c 
io write 4884c,1000000 
io read 4884c 
io write 4884c,4000000 
io read 48844 
io read 4884c 
io write 4884c,0 
io read 4884c 
io write 4884c,0 
io read 4884c 
io write 4884c,0 
io read 4880c 
io write 4880c,0 
-------
(gdb) where
#0  0x4008c0d4 in adc1_read_test ()
#1  0x4008c311 in hall_sens_read_full ()
#2  0x4008c34e in hall_sens_read ()
#3  0x400d179d in read_sens_task (pvParameters=<optimized out>)
    at /home/olas/esp/qemu_esp32/examples/09_onchip_sensor/main/./app_main.c:28

Stuck in loop
io read 48854 
io read 48854 
io read 48854 
io read 48854 
io read 48854 
io read 48854 
io read 48854 

0x4008c0be <adc1_read_test+242> and    a2, a9, a2
0x4008c0c1 <adc1_read_test+245> memw 
0x4008c0c4 <adc1_read_test+248> s32i.n a2, a8, 0 
0x4008c0c6 <adc1_read_test+250> memw
0x4008c0c9 <adc1_read_test+253> l32i.n a2, a8, 0
0x4008c0cb <adc1_read_test+255> or     a11, a2, a11
0x4008c0ce <adc1_read_test+258> memw
0x4008c0d1 <adc1_read_test+261> s32i   a11, a8, 0
0x4008c0d4 <adc1_read_test+264> memw
0x4008c0d7 <adc1_read_test+267> l32i.n a9, a8, 0
0x4008c0d9 <adc1_read_test+269> bnone  a9, a10, 0x4008c0d4 <adc1_read_test+264>

-> Stuck in this loop, need better qemu adc emulation

0x4008c0dc <adc1_read_test+272> memw
0x4008c0df <adc1_read_test+275> l32i.n a2, a8, 0
0x4008c0e1 <adc1_read_test+277> l32r   a8, 0x4008bfc8
0x4008c0e4 <adc1_read_test+280> sub    a2, a8, a2
0x4008c0e7 <adc1_read_test+283> extui  a2, a2, 0, 16
0x4008c0ea <adc1_read_test+286> retw.n 

```