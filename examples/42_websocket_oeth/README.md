# Websocket Sample application

This example is hardcoded to use espressifs qemu emulated openeth

# Running in qemu
qemu-system-xtensa -nographic -M esp32 -drive file=esp32flash.bin,if=mtd,format=raw -nographic -nic user,model=open_eth,hostfwd=tcp::10080-:80 -s