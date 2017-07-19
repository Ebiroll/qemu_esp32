Clone from here  https://github.com/nkolban/ESP32_Explorer
====================

##  The goal is to make this run in qemu. It does run
     However also does not run so well on target either...


```
  First generate flash
  gcc toflash.c -o qemu_flash

  ./qemu_flash build/app-template.bin

  Note that index.html is not in flash image

   xtensa-softmmu/qemu-system-xtensa -d unimp  -cpu esp32 -M esp32 -m 4M -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10080-192.168.4.3:80  -net dump,file=/tmp/vm0.pcap  -kernel  ~/esp/qemu_esp32/examples/24_esp32_explorer/build/app-template.elf  -s  > io.tx

```


To upload files use tftp. i.e.
```
  atftp -p -l index.html -r /index.html 192.168.1.99 69
  atftp -p -l ESP32Explorer.js -r ESP32Explorer.js 192.168.1.99 69

  This are not needed..
  atftp -p -l jquery/jquery-3.2.1.min.js  -r jquery/jquery-3.2.1.min.js
  atftp -p -l jquery/jquery-3.2.1.min.js  -r jquery/jquery-3.2.1.min.js 192.168.1.99 69
  atftp -p -l jqueryui/jquery-ui.min.js  -r jqueryui/jquery-ui.min.js  192.168.1.99 69
  atftp -p -l jqueryui/jquery-ui.min.css  -r jqueryui/jquery-ui.min.css  192.168.1.99 69
```

```
Or upload prebuild fat.bin 
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --baud 920600 write_flash 0x300000  fat.bin
```



To download fat 
```
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --baud 920600 read_flash 0x300000  0x100000 fat.bin
mkdir fat_partition
sudo mount -o loop,offset=0x0 fat.bin fat_partition
```




This is some information used earlier to debug a crash where flash.rodata was overwritten

```
xtensa-esp32-elf-gdb.qemu  build/app-template.elf -ex 'target remote:1234'

(gdb) b WL_Flash::WL_Flash();

Here we see that the vtable is initiated to 0x3f415118

(gdb) info symbol 0x3f415118
vtable for WL_Flash + 8 in section .flash.rodata
It seems to contain all 0xfffff
It seems like .flash.rodata gets overwritten by MMU emulation.
It would be wise to investigate how the bootloader sets upp the MMU registers to prevent this from happening.

  (gdb)  p/x (int *) *(0x3f415118+0)


  When restarting and looking at data before executing we see,
  p/x (int *) *(0x3f415118+0)
  $1 = 0x4015d7e8

  (gdb)  p/x (int *) *(0x3f415118+36)
  $4 = 0x40154e6c
  (gdb) info symbol 0x40154e6c
  WL_Flash::config(WL_Config_s*, Flash_Access*) in section .flash.text

It seems like, qemu overwrites .flash.roadata
=================

The location where roadata is overwritten is in esp_partition_find_first
(gdb) b esp_partition_find_first

If not running in qemu the bootloader sets up the mapping between flash and memory.
.flash.roadata
Un



(gdb) b ESP32_Explorer::start
(gdb) b FATFS_VFS::mount
(gdb) b esp_vfs_fat_spiflash_mount
(gdb) b wl_mount

We crash in wl_mount in wear_leveling.cpp (112)

result = wl_flash->config(&cfg, part); 

(gdb) layout next

(gdb) si

 <0x40154baa <wl_mount(esp_partition_t const*, wl_handle_t*)+218> call8  0x40154fd0 <WL_Flash::WL_Flash()>                       
<
B+ <0x40154bad <wl_mount(esp_partition_t const*, wl_handle_t*)+221> l32i.n a2, a4, 0
   <0x40154baf <wl_mount(esp_partition_t const*, wl_handle_t*)+223> l32i.n a2, a2, 36
   <0x40154bb1 <wl_mount(esp_partition_t const*, wl_handle_t*)+225> mov.n  a12, a5
   <0x40154bb3 <wl_mount(esp_partition_t const*, wl_handle_t*)+227> addi   a11, a1, 16
   <0x40154bb6 <wl_mount(esp_partition_t const*, wl_handle_t*)+230> mov.n  a10, a4
   <0x40154bb8 <wl_mount(esp_partition_t const*, wl_handle_t*)+232> callx8 a2                                       


(gdb) p *wl_flash
$15 = {<Flash_Access> = {_vptr$Flash_Access = 0x3f415118 <vtable for WL_Flash+8>}, configured = false, initialized = false, state = {pos = 0, max_pos = 0, move_count = 0, access_count = 0, max_count = 0, block_size = 0, version = 0, crc = 0}, cfg = {start_addr = 0, full_mem_size = 0, page_size = 0,    sector_size = 0, updaterate = 0, wr_size = 0, version = 0, temp_buff_size = 0, crc = 0}, flash_drv = 0x0, addr_cfg = 0, addr_state1 = 0, addr_state2 = 0,
  index_state1 = 0, index_state2 = 0, flash_size = 0, state_size = 0, cfg_size = 0, temp_buff = 0x0, dummy_addr = 0, used_bits = 0 '\000'}

(gdb) p/x $a4
$16 = 0x3ffd352c
(gdb) p/x $a2
$17 = 0x3f415118
(gdb) ni
(gdb) p/x $a2
$18 = 0xffffffff

  This causes a crash, the vtable for WL_Flash(); contains all 0xfffffff (-1)

```



