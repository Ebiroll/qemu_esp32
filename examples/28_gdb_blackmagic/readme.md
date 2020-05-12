Work in progress, allow gdb to connect over wifi on a live system
wifistub.c is the important file, the other files comes from here,
https://github.com/blacksphere/blackmagic

The idea is to add extended-remote and
(gdb) load filename.elf capabilities.

Some useful monitor commands can also be nice to investigate a running system.


How the code get access to freertos task information,
``` 
CFLAGS+=-DportREMOVE_STATIC_QUALIFIER 

extern List_t * volatile pxDelayedTaskList;
extern List_t * volatile pxOverflowDelayedTaskList;
extern List_t pxReadyTasksLists[configMAX_PRIORITIES];

``` 

For the cmake build system there is some bug, you must do
export CFLAGS="-DportREMOVE_STATIC_QUALIFIER -DINCLUDE_pcTaskGetTaskName=1"

xtensa-esp32-elf-gdb build/wifigdb.elf   -ex 'target remote 192.168.4.3:2345'


Allows reading from memory on a live system,


xtensa-esp32-elf-gdb build/wifigdb.elf   -b 115200 -ex 'target remote /dev/ttyUSB0'



The purpose of this project is to understand the gdb-stub and maybe improve it with some OTA flashing capabilities.

https://www-zeuthen.desy.de/dv/documentation/unixguide/infohtml/gdb/Thread-List-Format.html

As all other code here, its for learning, and in this case we learn about the gdbstub and the gdb protocol


Basic gdb-stub frame
``` 
Start with $ then single letter command more argumets # and checksum
If checksum is OK answer with + otherwise -
-> $q(extra)#cs
<- +
``` 


Interesting gdb commands
``` 
(gdb) info threads
(gdb) set debug remote 1
(gdb) set remotetimeout 30 
(gdb) set debug xtensa 1

(gdb) target remote:2345
(gdb) target remote 192.168.1.139:2345


(gdb) info threads
(gdb) monitor help
(gdb) set debug protocol 5

Not so useful
(gdb) set debug target 1
``` 


In qemu
======

```
qemu_flash build/wifigdb.elf

xtensa-softmmu/qemu-system-xtensa -d unimp,guest_errors -cpu esp32 -M esp32 -m 4M   -net nic,model=vlan0 -net user,id=simnet,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::2345-192.168.4.3:2345  -net dump,file=/tmp/vm0.pcap   -s     >  io.txt


> xtensa-esp32-elf-gdb.qemu 
(gdb) target extended-remote:2345

However due to far from perfect network emulation, it does not run so well in qemu. :-(

Also try 
(gdb) target extended-remote:2345
``` 
