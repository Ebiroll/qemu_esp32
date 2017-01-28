
I got a bit curios about nuttx so I tried compiling it to run with qemu

git clone https://bitbucket.org/nuttx/nuttx.git nuttx

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
