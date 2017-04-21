# Interactive tests of qemu 1306 or physical display


This is done in order to debug and develop the 1306 qemu emulation. It runs well on esp32 hardware with a 1306 display connected to i2c  and expects to use these pins.

#define I2C_MASTER_SCL_IO    22    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    21    /*!< gpio number for I2C master data  */



To run the example you must copy the components/emul_ip from the top directory. or better, make a symbolic link.
  ln -s ../../components components
This is needed to get the emulated network running.


To start emulation do,

    xtensa-softmmu/qemu-system-xtensa -d unimp -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10023-192.168.4.3:10023  -cpu esp32 -M esp32 -m 4M  -kernel  ~/esp/qemu_esp32/examples/06_1306_interactive/build/app-template.elf -s   > io.txt

To see the i2c traffic, try -d guest_errors,unimp

To connect to the menu socket, in qemu try 

    nc 127.0.0.1 10023

To connect to uart-1 try,
    nc 127.0.0.1 8881
Press return to get menu, not all characters are displayed due to bug. TODO , fix

To connect to the menu when running on hw try look at display for ip and try,

    nc 192.168.x.y 10023


To run the debugger, do
    xtensa-esp32-elf-gdb.qemu  build/app-template.elf -ex 'target remote:1234'




