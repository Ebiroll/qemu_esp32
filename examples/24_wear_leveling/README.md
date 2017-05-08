# SD Card example

This example demonstrates how to use wear levelling library and FATFS library to store files in a partition inside SPI flash. Example does the following steps:

1. Use an "all-in-one" `esp_vfs_fat_spiflash_mount` function to:
    - find a partition in SPI flash,
    - initialize wear levelling library using this partition
    - mount FAT filesystem using FATFS library (and format the filesystem, if the filesystem can not be mounted),
    - register FAT filesystem in VFS, enabling C standard library and POSIX functions to be used.
2. Create a file using `fopen` and write to it using `fprintf`.
3. Open file for reading, read back the line, and print it to the terminal.

## Example output

Here is an typical example console output. 

```
Try to open file ...
I (239) wear_level: Reading file
Read from file: 'Hello User! I'm happy to see you!1'
W (239) wear_levelling: wl_unmount Delete driver
```
# To run example in qemu,

To run in qemu, you must prepare flash

gcc  toflash.c  -o toflash
./toflash build/wear_levelling_example.bin

Now it runs

# qemu

    xtensa-softmmu/qemu-system-xtensa  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/examples/24_wear_leveling/build/wear_levelling_example.elf   -s  > io.txt
    xtensa-esp32-elf-gdb.qemu  build/wear_levelling_example.elf -ex 'target remote:1234'


#Examining FAT data,

Dump the FAT flash partition
    ~/esp/esp-idf/components/esptool_py/esptool/esptool.py --baud 920600 read_flash 0x110000 0x100000 fat.bin
    mkdir fat_partition
    sudo mount -o loop,offset=0x0 fat.bin fat_partition

Or if you use the qemu image directly,
    sudo mount -o loop,offset=0x110000 esp32flash.bin fat_partition
However there are still bugs in qemu spi flash emulation that causes the FAT data to be corrupted.