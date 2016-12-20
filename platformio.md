#Platform IO

This looked promising http://docs.platformio.org/en/latest/frameworks/espidf.html

To try it out I did the following.
Check out this link.
https://primalcortex.wordpress.com/2016/08/18/platformio/

On arch,
```
yaourt -S platformio

// This does not seem like the latest version. 1.12
yaourt -S atom-editor-bin

cd /project dir
```

#platformio = pio

```
pio platforms install espressif32

pio init --board esp32thing


Add this to platformio.ini
[platformio]
src_dir = main


```



#Arch linux

Arch linux uses python 3 as default.
Set python interpreter as python2

```
make menuconfig

   SDK tool configuration  --->
      (python2) Python 2 interpreter
```

I had to cp sdkconfig.h manually to main directory to compile.
