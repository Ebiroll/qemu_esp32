# Nuttx example
Nuttx does not rely on the esp-idf toolchain.

https://acassis.wordpress.com/2018/01/04/running-nuttx-on-esp32-board/

Builing kconfig-conf,
>which kconfig-conf
git clone https://bitbucket.org/nuttx/tools
cd tools/kconfig-frontends/
./configure --enable-mconf --enable-nconf --disable-gconf --disable-qconf
make
make install


# Build nuttx & nsh

git clone https://bitbucket.org/nuttx/nuttx.git nuttx
git clone https://bitbucket.org/nuttx/apps.git


cd nuttx
make distclean
./tools/configure.sh esp32-core/nsh

# Enable debug symbols
make menuconfig
 Build Setup → Debug Options →  [*] Generate Debug Symbols 
Also turn off optimizations for better debugging experience.
Build setup ->   Optimization Level (Suppress Optimization)

make


# Elf to image,
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 elf2image --flash_mode dio --flash_size 4MB -o ./nuttx2.bin nuttx

home/olof/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 elf2image --flash_mode dio --flash_size 4MB -o ./nuttx2.bin nuttx/nuttx


# Copy partitions and bootloader from other project
mkdir -p  build/bootloader
cp ../../06_1306_interactive/qemu_flash .
cp ../../06_1306_interactive/build/bootloader/bootloader.bin build/bootloader/
cp ../../06_1306_interactive/build/partitions_singleapp.bin build/

./qemu_flash nuttx2.bin

# Start qemu
xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M   -s -S   > io.txt

# Start debugger
 xtensa-esp32-elf-gdb.qemu nuttx -ex 'target remote:1234'

(gdb) b os_start(void)


# Serial emulation

 Now the serial emulation has improved to allow running nsh, the nuttx shell.

 esp32_interrupt() , is the UART interrupt handler


# Connect to uart0
nc 127.0.0.1 8880

```

TRYING to MAP esp32flash.binMAPPED esp32flash.binI (0) boot: ESP-IDF v3.1-dev-145-gc401a74 2nd stage bootloader
I (0) boot: compile time 15:56:45
I (0) boot: Enabling RNG early entropy source...
I (0) boot: SPI Speed      : 40MHz
I (0) boot: SPI Mode       : DIO
I (0) boot: SPI Flash Size : 2MB
I (0) boot: Partition Table:
I (0) boot: ## Label            Usage          Type ST Offset   Length
I (0) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (0) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (0) boot:  2 factory          factory app      00 00 00010000 00100000
I (0) boot: End of partition table
I (0) esp_image: segment 0: paddr=0x00010020 vaddr=0x3ffb0e10 size=0x00098 (   152) load
I (0) esp_image: segment 1: paddr=0x000100c0 vaddr=0x40080000 size=0x00400 (  1024) load
I (0) esp_image: segment 2: paddr=0x000104c8 vaddr=0x40080400 size=0x0032c (   812) load
I (0) esp_image: segment 3: paddr=0x000107fc vaddr=0x400c0000 size=0x00000 (     0) load
I (0) esp_image: segment 4: paddr=0x00010804 vaddr=0x00000000 size=0x0f804 ( 63492) 
I (3) esp_image: segment 5: paddr=0x00020010 vaddr=0x3f400010 size=0x01f8c (  8076) map
I (4) esp_image: segment 6: paddr=0x00021fa4 vaddr=0x00000000 size=0x0e06c ( 57452) 
I (6) esp_image: segment 7: paddr=0x00030018 vaddr=0x400d0018 size=0x169a0 ( 92576) map
I (10) boot: Loaded app from partition at offset 0x10000
I (10) boot: Disabling RNG early entropy source...
esp32_i2c_interruptSet: new IRQ val 0xd8c30120

NuttShell (NSH)
nsh> Started uart socket
help
help usage:  help [-v] [<cmd>]

  [           cmp         false       mkdir       rm          true        
  ?           dirname     free        mh          rmdir       uname       
  basename    dd          help        mount       set         umount      
  break       df          hexdump     mv          sh          unset       
  cat         echo        kill        mw          sleep       usleep      
  cd          exec        ls          ps          test        xd          
  cp          exit        mb          pwd         time        

Builtin Apps:
nsh> ls
/:
 dev/
 proc/
nsh> pwd
/

```

# Adding application examples to nsh
make menuconfig

→ Application Configuration → Examples



```
Builtin Apps:
  helloxx
  null
nsh> helloxx
helloxx
helloxx_main: Saying hello from the dynamically constructed instance
CHelloWorld::HelloWorld: Hello, World!!
helloxx_main: Saying hello from the instance constructed on the stack
CHelloWorld::HelloWorld: Hello, World!!
helloxx_main: Saying hello from the statically constructed instance
CHelloWorld::HelloWorld: CONSTRUCTION FAILED!
nsh> 
```
