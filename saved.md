Now this is not required if you use updated qemu-esp32.
```
Ahome/olas/esp/qemu_esp32/examples/06_ssd1306-esp-idf-i2c/sdkconfig - Espressif IoT Development Framework Confi
g> Component config > ESP32-specific --------------------------------------------------------------------------




                       +------------ Timers used for gettimeofday function ------------+
                       |  Use the arrow keys to navigate this window or press the      |  
                       |  hotkey of the item you wish to select followed by the <SPACE |  
                       |  BAR>. Press <?> for additional information about this        |  
                       | +-----------------------------------------------------------+ |  
                       | |                     ( ) RTC                               | |  
                       | |                     ( ) RTC and FRC1                      | |  
                       | |                     ( ) FRC1                              | |  
                       | |                     (X) None                              | |  
                       | |                                                  
```


If you get a crash like this,
I (124) cpu_start: Pro cpu start user code
 File 'esp32flash.bin' is truncated or corrupt.
 
qemu) MMU 117f4  1
(qemu) MMU 117f8  3ff12b00
(qemu) MMU 117fc  17
(qemu) MMU 11808  4000c2e0
(qemu) MMU 1180c  4000c2f6
(qemu) MMU 11814  800855cd
(qemu) MMU 11818  3ff11a90
(qemu) MMU 1181c  40081144
(qemu) MMU 11800  14
(qemu) MMU 117bc  400d12f0
(qemu) MMU 116e0  3ff11790
(qemu) MMU 116c4  3ff11790
(qemu) MMU 116d8  60033
(qemu) MMU 116d4  400d12f0
(qemu) MMU 116c0  400d12f0
(qemu) MMU 1170c  800855cd
```
Then try starting with -s -S, this will wait for the debugger.


This seems to happen most often with C++ applications.


Needs furher investigations!

It seems like c++ code has more problems in qemu, maybe because the
```
code is not mapped properly and because of 
 The .flash.rodata section and
 do_global_ctors();

```

## The workaround is,

    > (gdb) b call_start_cpu0
    > (gdb) c
Now the bootloader sets up the system to what the application exects,
However some bug somewhere sometimes overwrites the code,

    > (gdb) load build/app-template.elf

