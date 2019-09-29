export BATCH_BUILD=1
export V=0
#cd 01_max_malloc
#idf.py build
#cd ../02_gpio
#idf.py build
#cd ../03_dual
#idf.py build
#cd ../03_unstall
#idf.py build
#cd ../04_schedule
#idf.py build
cd 04_sha
gcc ../../toflash-cmake.c -o qemu_flash
qemu_flash build/sha.bin
cd ~/qemu_esp32
xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M  -s  > io.txt
