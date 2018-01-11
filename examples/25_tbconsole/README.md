# Basic interpreter

https://github.com/espressif/esp-idf/blob/master/docs/api-guides/romconsole.rst


http://hackaday.com/2016/10/27/basic-interpreter-hidden-in-esp32-silicon/


On the esp32 it works, in qemu not...

```
make monitor

I (0) cpu_start: App cpu up.
I (974) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (996) heap_alloc_caps: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (1017) heap_alloc_caps: At 3FFB60B8 len 00029F48 (167 KiB): DRAM
I (1038) heap_alloc_caps: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (1059) heap_alloc_caps: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (1081) heap_alloc_caps: At 40092DB8 len 0000D248 (52 KiB): IRAM
I (1102) cpu_start: Pro cpu start user code
Falling back to built-in command interpreter.
OK
>
>
>help
A very Basic ROM console. Available commands/functions:
LIST
NEW
RUN
NEXT
LET
IF
GOTO
GOSUB
RETURN
REM
FOR
INPUT
PRINT
PHEX
POKE
STOP
BYE
MEM
?
'
DELAY
END
RSEED
HELP
ABOUT
IOSET
IODIR
PEEK
ABS
RND
IOGET
USR
>
>about
ESP32 ROM Basic (c) 2016 Espressif Shanghai
Derived from TinyBasic Plus by Mike Field and Scott Lawrence
>
>quit
What? 
>
>
```


# Have not been able to get it to run on qemu.


# qemu
To connect to uart
  nc 127.0.0.1 8880

Stops in this loop,


  wifi read 1c 
  io read 4808c 
  io write 4808c,0 
  io read 5f048 
  io write 5f048,0 
  wifi read 0 

This would be on address  0x6000001c &  0x60000000

```
To send cr/lf
telnet
telnet> toggle crlf
open 127.0.0.1 8880
To exit press Ctrl-]
telnet> quit
```