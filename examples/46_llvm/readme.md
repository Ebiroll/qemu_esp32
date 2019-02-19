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

int add(int i,int j) {
  return(i+j);
}


~/esp/llvm_build/bin/clang -target xtensa -fomit-frame-pointer -S add.c -o main/test.S

	.type	add,@function
add:
	entry	sp, 56
	mov.n	a8, a3
	mov.n	a9, a2
	s32i.n	a2, sp, 16
	s32i.n	a3, sp, 12
	l32i.n	a3, sp, 16
	l32i.n	a2, sp, 12
	add.n	a2, a3, a2
	s32i.n	a8, sp, 8
	s32i.n	a9, sp, 4
	retw.n

``` 

Comparing output from gcc and llvm
``` 
xtensa-esp32-elf-objdump -d build/main/app_main.o

00000000 <add2>:
   0:	004136        	entry	a1, 32
   3:	223a      	add.n	a2, a2, a3
   5:	f01d      	retw.n

``` 

This is the IR file from clang
``` 
~/esp/llvm_build/bin/clang -S -emit-llvm add.c
add.ll
define i32 @add(i32, i32) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32 %1, i32* %4, align 4
  %5 = load i32, i32* %3, align 4
  %6 = load i32, i32* %4, align 4
  %7 = add nsw i32 %5, %6
  ret i32 %7
}

``` 
Here is where the IR transformed 
https://github.com/espressif/llvm-xtensa/tree/esp-develop/lib/Target/Xtensa

# Instr info
https://github.com/espressif/llvm-xtensa/blob/esp-develop/lib/Target/Xtensa/XtensaInstrInfo.td



In qemu
======

Short expanation about the LX6 and ESP32
https://www.lortex.org/posts/esp32/2018/03/28/the-xtensa-architecture.html


```
../../qemu_flash build/llvm.bin

xtensa-softmmu/qemu-system-xtensa -d unimp,guest_errors -cpu esp32 -M esp32 -m 4M -s -S  >  io.txt

xtensa-esp32-elf-gdb.qemu build/llvm.elf   -ex 'target remote:1234'
> layout next
> b add
> si


> xtensa-esp32-elf-gdb.qemu 
(gdb) target extended-remote:1234

``` 
