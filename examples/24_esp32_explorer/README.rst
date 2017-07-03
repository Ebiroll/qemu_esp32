Clone from here  https://github.com/nkolban/ESP32_Explorer
====================

The goal is to make this run in qemu.

 xtensa-softmmu/qemu-system-xtensa -d unimp  -cpu esp32 -M esp32 -m 4M -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10080-192.168.4.3:80  -net dump,file=/tmp/vm0.pcap  -kernel  ~/esp/qemu_esp32/examples/24_esp32_explorer/build  -s  > io.tx


This is a template application to be used with `Espressif IoT Development Framework`_ (ESP-IDF). 

Please check ESP-IDF docs for getting started instructions.

Code in this repository is Copyright (C) 2016 Espressif Systems, licensed under the Apache License 2.0 as described in the file LICENSE.

.. _Espressif IoT Development Framework: https://github.com/espressif/esp-idf


