
I got a bit curios about nuttx so I tried compiling it to run with qemu

Some more info.
https://acassis.wordpress.com/2018/01/04/running-nuttx-on-esp32-board/


git clone https://bitbucket.org/nuttx/tools

cd tools/kconfig-frontends/

./configure --enable-mconf --enable-nconf --disable-gconf --disable-qconf





git clone https://bitbucket.org/nuttx/nuttx.git nuttx

# Build nsh
./tools/configure.sh esp32-core/nsh

#Try to enable debug symbols also


cd tools

//./configure.sh  esp32-core/nsh
./configure.sh  esp32-core/smp
cd -
make oldconfig
. ./setenv.sh
make



  Before sourcing the setenv.sh file above, you should examine it and
  perform edits as necessary so that TOOLCHAIN_BIN is the correct path to
  the directory than holds your toolchain binaries.


Only got this output.
make[1]: Leaving directory `/home/olas/nuttx/configs'
make: the `-C' option requires a non-empty string argument
Usage: make [options] [target] ...

See configs/esp32-core/README.txt for further instructions

cd ..
git clone https://bitbucket.org/nuttx/apps.git
cd nuttx
Added,
CONFIG_APPS_DIR = ../apps
to Makefile.unix



/home/olas/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 elf2image --flash_mode dio --flash_size 4MB -o ./nuttx2.bin nuttx

