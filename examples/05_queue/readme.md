# Start qemu

xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -kernel  ~/esp/qemu_esp32/examples/05_queue/build/queue.elf  -s -S > io.txt

# start gdb
xtensa-esp32-elf-gdb.qemu  build/queue.elf -ex 'target remote:1234'

```

```

