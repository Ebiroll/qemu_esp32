#Building nuttx

https://acassis.wordpress.com/2018/01/04/running-nuttx-on-esp32-board/


git clone https://bitbucket.org/nuttx/tools
cd tools/kconfig-frontends/
./configure --enable-mconf --enable-nconf --disable-gconf --disable-qconf
make
make install

git clone https://bitbucket.org/nuttx/nuttx.git nuttx
git clone https://bitbucket.org/nuttx/apps.git

# Build nsh
cd nuttx
make distclean
./tools/configure.sh esp32-core/nsh

#Enable debug symbols
make menuconfig
Also turn off optimizations for better debugging experience.
Build setup ->   Optimization Level (Suppress Optimization)


make


#Make elf to smaller bin
~/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 elf2image --flash_mode dio --flash_size 4MB -o ./nuttx2.bin nuttx

home/olof/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 elf2image --flash_mode dio --flash_size 4MB -o ./nuttx2.bin nuttx/nuttx


# Copy partitions and bootloader from other project
mkdir -p  build/bootloader
cp ../06_1306_interactive/build/bootloader/bootloader.bin build/bootloader/
cp ../06_1306_interactive/build/partitions_singleapp.bin build/

./qemu_flash nuttx/nuttx2.bin

# Start qemu
xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M   -s -S   > io.txt

#Start debugger
cd nuttx


(gdb) b os_start(void)


# Due to faulty qemu serial intterupt emulation,
There is an interrupt, loop that ends up here,
 esp32_serialin()
(gdb)b esp32_serialout()
or here,
int dup2(int fd1, int fd2)

(gdb) si
Then press and hold return

#0  esp32_serialout (priv=<optimized out>, value=<optimized out>, offset=<optimized out>) at chip/esp32_serial.c:394
#1  esp32_interrupt (cpuint=<optimized out>, context=<optimized out>, arg=0x3ffb0e04 <g_uart0port>) at chip/esp32_serial.c:756
#2  0x400d85f4 in irq_dispatch (irq=38, context=0x3ffb0bc0 <g_idlestack+448>) at irq/irq_dispatch.c:109
#3  0x400d2667 in xtensa_irq_dispatch (irq=38, regs=0x3ffb0bc0 <g_idlestack+448>) at common/xtensa_irqdispatch.c:92
#4  0x400d2470 in xtensa_int_decode (cpuints=0, regs=<optimized out>) at chip/esp32_intdecode.c:142
#5  0x40080474 in _xtensa_level1_handler () at common/xtensa_int_handlers.S:277

 inode_addref(inode);

#TODO Set a breakpont here and fix qemu
(gdb) b esp32_interrupt()

          handled      = false;
   747           priv->status = esp32_serialin(priv, UART_INT_RAW_OFFSET);
   748           status       = esp32_serialin(priv, UART_STATUS_OFFSET); 
   749           enabled      = esp32_serialin(priv, UART_INT_ENA_OFFSET);     