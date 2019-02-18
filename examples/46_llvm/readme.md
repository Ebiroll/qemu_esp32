## Xtensa llvm test

This tests the llvm compiler for xtensa,
https://esp32.com/viewtopic.php?f=2&t=9226

Builing llvm
``` 
cd ~/esp
git clone https://github.com/espressif/llvm-xtensa.git
git clone https://github.com/espressif/clang-xtensa.git llvm-xtensa/tools/clang
mkdir llvm_build
cd llvm_build
cmake ../llvm-xtensa -DLLVM_TARGETS_TO_BUILD=Xtensa -DCMAKE_BUILD_TYPE=Release -G "Ninja"
cmake --build .

``` 

Compiling the example 
``` 
~/esp/llvm_build/bin/clang -target xtensa -fomit-frame-pointer -S add.c -o main/test.S

``` 







In qemu
======

```
../../qemu_flash build/llvm.bin

xtensa-softmmu/qemu-system-xtensa -d unimp,guest_errors -cpu esp32 -M esp32 -m 4M  >  io.txt

xtensa-esp32-elf-gdb build/llvm.elf   -ex 'target remote 192.168.4.3:1234'


> xtensa-esp32-elf-gdb.qemu 
(gdb) target extended-remote:1234

``` 
