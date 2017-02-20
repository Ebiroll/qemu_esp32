#Test of freertos queues
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB4DD8 len 0002B228 (172 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 40093390 len 0000CC70 (51 KiB): IRAM
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (4) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00008000 to memory, 3F408000
I (5120) test queues: Received = 200
I (5120) test queues: Received = 100
I (5620) test queues: Received = 100
I (5620) test queues: Received = 200
I (6120) test queues: Received = 200
I (6120) test queues: Received = 100
I (6620) test queues: Received = 100
I (6620) test queues: Received = 200
I (7120) test queues: Received = 200
I (7120) test queues: Received = 100
I (7620) test queues: Received = 100
I (7620) test queues: Received = 200
I (8120) test queues: Received = 200
I (8120) test queues: Received = 100
I (8620) test queues: Received = 100
I (8620) test queues: Received = 200
I (9120) test queues: Received = 200
I (9120) test queues: Received = 100
I (9620) test queues: Received = 100
I (9620) test queues: Received = 200

#Flash emulation
It seems that I probably have to emulate flash erase write (0xff) on erase. To prevent this crash.
If you get this assert try, mv mv esp32flash.bin saveflash.bin
When starting qemu you should start with an empty flash.

ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB4DD8 len 0002B228 (172 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 40093390 len 0000CC70 (51 KiB): IRAM
I (1) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (4) cpu_start: Starting scheduler on PRO CPU.
(qemu)  00008000 to memory, 3F408000
assertion "state == EntryState::WRITTEN || state == EntryState::EMPTY" failed: file "/home/olas/esp/esp-idf/components/nvs_flash/src/nvs_page.cpp", line 315, function: esp_err_t nvs::Page::eraseEntryAndSpan(size_t)
abort() was called at PC 0x400de1a7
Guru Meditation Error: Core  0 panic'ed (abort)

Backtrace: 0x400de20e:0x3ffb64f0 0x00000000:0x3ffb6510 0x400ec571:0x3ffb6540 0x400ecb6d:0x3ffb6580 0x400ecda6:0x3ffb65f0 0x400ed5c4:0x3ffb6650 0x400ebdf8:0x3ffb66b0 0x400ebad6:0x3ffb6700 0x400ebb22:0x3ffb6720 0x400ef512:0x3ffb6740 0x400d09d5:0x3ffb6770

Rebooting...





#This was also used to analyse why Initialize PHY in startup code fails to work. 
Emulating register_chipv7
To allow easier use of qemu without having to disable,  Initialize PHY in startup code.

Another option would to use calibration data from flash dump,
Stop in phy_byte_to_word to find where mac adress is stored, probably in EFUSE area.


```
register_chipv7_phy
calls 
   register_chipv7_phy_init_param, looks at some tables..
   rf_init, 


```

```

#0  get_rf_freq_cap (freq=2414, freq_offset=0, x_reg=0x3ffe3d0c "0ww", cap_array=0x3ffe3d0f "") at phy_chip_v7_ana.c:802
#1  0x40085c59 in get_rf_freq_init () at phy_chip_v7_ana.c:900
#2  0x40086a26 in set_chan_freq_hw_init (tx_freq_offset=2 '\002', rx_freq_offset=4 '\004') at phy_chip_v7_ana.c:1492
#3  0x40086c79 in rf_init () at phy_chip_v7_ana.c:795
#4  0x40089aca in register_chipv7_phy (init_param=<optimized out>, rf_cal_data=0x3ffb6834 "", rf_cal_level=2 '\002') at phy_chip_v7.c:2880
#5  0x400d1528 in esp_phy_init (init_data=0x3f401b54 <phy_init_data>, mode=PHY_RF_CAL_FULL, calibration_data=0x3ffb6834)
    at /home/olas/esp/esp-idf/components/esp32/./phy_init.c:50
#6  0x400d0787 in do_phy_init () at /home/olas/esp/esp-idf/components/esp32/./cpu_start.c:295
#7  0x400809ec in start_cpu0_default () at /home/olas/esp/esp-idf/components/esp32/./cpu_start.c:215



#0  chip_v7_ana_rx_cfg () at phy_chip_v7_ana.c:688
#1  0x40086c08 in rf_init () at phy_chip_v7_ana.c:776
#2  0x40089aca in register_chipv7_phy (init_param=<optimized out>, rf_cal_data=0x3ffb6834 "", rf_cal_level=2 '\002') at phy_chip_v7.c:2880
#3  0x400d1528 in esp_phy_init (init_data=0x3f401b54 <phy_init_data>, mode=PHY_RF_CAL_FULL, calibration_data=0x3ffb6834)
    at /home/olas/esp/esp-idf/components/esp32/./phy_init.c:50
#4  0x400d0787 in do_phy_init () at /home/olas/esp/esp-idf/components/esp32/./cpu_start.c:295
#5  0x400809ec in start_cpu0_default () at /home/olas/esp/esp-idf/components/esp32/./cpu_start.c:215
#6  0x40080be2 in call_start_cpu0 () at /home/olas/esp/esp-idf/components/esp32/./cpu_start.c:138


#0  rfpll_init () at phy_chip_v7_ana.c:243
wifi read 26000 
wifi write 26000,400 
wifi read e000 
wifi write e000,1f30163 
wifi read e000 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,37000 
wifi read e00c 
wifi write e00c,68 
wifi read e00c 
wifi read e00c 
wifi read e00c 
wifi write e00c,1200068 
wifi read e00c 


#chip_v7_ana_rx_cfg ()
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,266 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1000266 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,566 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1060566 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,566 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1c00566 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,a66 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1100a66 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,3d000 
wifi read e008 
wifi write e008,26a 
wifi read e008 
wifi read e008 
wifi read e008 
wifi write e008,110026a 
wifi read e008 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,866 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1010866 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,866 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1000866 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,866 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1200866 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,866 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1000866 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,966 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1000966 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,1f000 
wifi read e010 
wifi write e010,966 
wifi read e010 
wifi read e010 
wifi read e010 
wifi write e010,1040966 
wifi read e010 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,3f000 
wifi read e004 
wifi write e004,967 
wifi read e004 
wifi read e004 
wifi read e004 
wifi write e004,1010967 
wifi read e004 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,3f000 
wifi read e004 
wifi write e004,b67 
wifi read e004 
wifi read e004 
wifi read e004 
wifi write e004,1010b67 
wifi read e004 

#rom_chip_v7_tx_init
wifi read e008 
wifi write e008,1a8036b 
wifi read e008 
wifi read e008 
wifi write e008,106046b 
wifi read e008 
wifi read e008 
wifi write e008,108056b 
wifi read e008 
wifi read e008 
wifi write e008,1b8066b 
wifi read e008 
wifi read e008 
wifi write e008,15b076b 
wifi read e008 
wifi read e008 
wifi write e008,1740a6b 
wifi read e008 

#rom_chip_v7_bt_init
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,3d000 
wifi read e008 
wifi write e008,26a 
wifi read e008 
wifi read e008 
wifi read e008 
wifi write e008,120026a 
wifi read e008 
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,3d000 
wifi read e008 
wifi write e008,6a 
wifi read e008 
wifi read e008 
wifi read e008 
wifi write e008,102006a 
wifi read e008 

#rom_pbus_debugmode
io read 24 
wifi read 609c 
wifi write 609c,0 
wifi read 6094 
wifi write 6094,1 


#rom_pbus_set_dco
wifi read 6094 
wifi write 6094,c00b 
wifi read 60a0 
wifi read 6094 
wifi write 6094,c009 
wifi read 6094 
wifi write 6094,c00f 
wifi read 60a0 
wifi read 6094 
wifi write 6094,c00d 
wifi read 6094 
wifi write 6094,1400b 
wifi read 60a0 
wifi read 6094 
wifi write 6094,14009 
wifi read 6094 
wifi write 6094,1400f 
wifi read 60a0 
wifi read 6094 
wifi write 6094,1400d 


#rom_pbus_xpd_tx_off
wifi read 6094 
wifi write 6094,8013 
wifi read 60a0 
wifi read 6094 
wifi write 6094,8011 
wifi read 6094 
wifi write 6094,8017 
wifi read 60a0 
wifi read 6094 
wifi write 6094,8015 
wifi read 6094 
wifi write 6094,8007 
wifi read 60a0 
wifi read 6094 
wifi write 6094,8005 
wifi read 6094 
wifi write 6094,10007 
wifi read 60a0 
wifi read 6094 
wifi write 6094,10005 
wifi read 6094 
wifi write 6094,8003 
wifi read 60a0 
wifi read 6094 
wifi write 6094,8001 

#rom_pbus_xpd_rx_off

#set_chan_freq_hw_init


#get_rf_freq_init
(qemu ret) internal i2c block 0x67 0b
(qemu) internal i2c 62 0
(qemu ret) internal i2c block 0x62 00 0
(qemu ret) internal i2c block 0x62 00 0
(qemu ret) internal i2c block 0x62 00 0
(qemu) internal i2c 62 0
(qemu ret) internal i2c block 0x62 00 0
(qemu ret) internal i2c block 0x62 00 0
(qemu) internal i2c 62 2
(qemu ret) internal i2c block 0x62 02 0
(qemu ret) internal i2c block 0x62 02 0
(qemu ret) internal i2c block 0x62 02 0
(qemu) internal i2c 62 2
(qemu ret) internal i2c block 0x62 02 0


Calls get_rf_freq_cap
#get_rf_freq_cap




Calls ram_rfpll_reset
#ram_rfpll_reset
Just returns..

#rom_rfpll_set_freq

I think its here we get the problem.

I (1631) phy: error: pll_cal exceeds 2ms!!!


#chip_v7_rxmax_ext_ana

.

Here we get the problem.

I (1631) phy: error: pll_cal exceeds 2ms!!!

#write_wifi_chan_data


> 0x400869d9 <write_wifi_chan_data+89>    ssr    a13                                                                                      
    0x400869dc <write_wifi_chan_data+92>    srl    a13, a15                                                                                 
    0x400869df <write_wifi_chan_data+95>    bgeu   a14, a8, 0x400869f6 <write_wifi_chan_data+118>                                           
    0x400869e2 <write_wifi_chan_data+98>    slli   a13, a8, 3                                                                               
    0x400869e5 <write_wifi_chan_data+101>   mov.n  a5, a13                                                                                  
    0x400869e7 <write_wifi_chan_data+103>   ssr    a5                                                  


#bt_i2c_set_wifi_data

#pll correct dcap
wifi read e044 
wifi write e044,3f000 
wifi read e044 
wifi write e044,3f000 
wifi read e004 
wifi write e004,562 
wifi read e004 
wifi read e004 

#set_channel_rfpll_freq 

It loops and loops. then it ends Here
Guru Meditation Error: Core  0 panic'ed (Unhandled debug exception)
Debug exception reason: BREAK instr 
TBD(pc = 40080cd6): /home/olas/qemu-xtensa-esp32/target-xtensa/translate.c:1404
Register dump:
PC      : 0x40087790  PS      : 0x00060d36  A0      : 0x8008780d  A1      : 0x3ffe3c80  
A2      : 0x00000000  A3      : 0x00000000  A4      : 0x00000028  A5      : 0x00000000  
A6      : 0x00000985  A7      : 0x00000026  A8      : 0x00000985  A9      : 0x0000000a  
A10     : 0x00000003  A11     : 0x00000001  A12     : 0x00000009  A13     : 0x00000007  
A14     : 0x00000005  A15     : 0x00000000  SAR     : 0x0000001b  EXCCAUSE: 0x00000001  
EXCVADDR: 0x00000000  LBEG    : 0x4000c2e0  LEND    : 0x4000c2f6  LCOUNT  : 0x00000000  

Backtrace: 0x40087790:0x3ffe3c80 0x4008780d:0x3ffe3d60 0x40086f28:0x3ffe3d80 0x40088cf3:0x3ffe3da0 0x400894b1:0x3ffe3dc0 0x40089acd:0x3ffe3de0 0x400d1528:0x3ffe3e00 0x400d0787:0x3ffe3e20 0x400809ec:0x3ffe3e40 0x40080be2:0x3ffe3e70 0x40007c34:0x3ffe3eb0 0x40000740:0x3ffe3f20



```



```
   rf_init, 
wifi read 8030 
wifi write 8030,f8000000 
wifi write e020,80 
wifi write e024,80 
wifi write e028,80 
wifi write e02c,80 
wifi write e030,80 
wifi read 60a0 
wifi write 60a0,1000000 

Calls,
rom_i2c_writeReg_Mask


```




```
rtc_get_xtal

io read 480b0 
io read 480b4 RTC_CNTL_STORE5_REG 3ff480b4=4C4B4C4B
io write 480b0,280028 
```

```
    0x4008950e <register_chipv7_phy_init_param+18>  call8  0x4008ef08 <rtc_get_xtal>                                                        
    0x40089511 <register_chipv7_phy_init_param+21>  extui  a10, a10, 0, 8                                                                   
    0x40089514 <register_chipv7_phy_init_param+24>  movi.n a9, 24                                                                           
    0x40089516 <register_chipv7_phy_init_param+26>  movi.n a8, 2                                                                            
  > 0x40089518 <register_chipv7_phy_init_param+28>  beq    a10, a9, 0x40089528 <register_chipv7_phy_init_param+44>                          
    0x4008951b <register_chipv7_phy_init_param+31>  addi   a10, a10, -26                                  

```
register_chipv7_phy_init_param looks for 
$14 = 0x3f401b65
(gdb) p/x $a11
$15 = 0x0




# Start qemu

```
xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -kernel  ~/esp/qemu_esp32/examples/05_queue/build/queue.elf  -s -S > io.txt
```

# start gdb
```
xtensa-esp32-elf-gdb.qemu  build/queue.elf -ex 'target remote:1234'
```



