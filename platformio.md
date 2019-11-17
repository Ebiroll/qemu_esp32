# Platform IO

You dont need this to compile qemu, but it looks nice and could be worth evaluating.
This looked promising http://docs.platformio.org/en/latest/frameworks/espidf.html

I tried platformio for this project.
https://github.com/Ebiroll/esp32_ultra

To try it out I did the following.
# On ubuntu 
I installed.
http://platformio.org/platformio-ide

It will download all you need to this directory.
~/.platformio/

Buildfiles will be generated in proj_dir/.pioenvs/


# On arch-linux,
Check out this link.
https://primalcortex.wordpress.com/2016/08/18/platformio/

```
yaourt -S platformio

// This does not seem like the latest version. 1.12
yaourt -S atom-editor-bin

cd /project dir
```
# platformio = pio

```
pio platforms install espressif32

pio init --board esp32thing

Add this to platformio.ini
[platformio]
src_dir = main

This way you can chose to use esp-idf or platformio.

```



# Arch linux

Arch linux uses python 3 as default.
Set python interpreter as python2

```
make menuconfig

   SDK tool configuration  --->
      (python2) Python 2 interpreter
```

I had to cp sdkconfig.h manually to main directory to compile.
