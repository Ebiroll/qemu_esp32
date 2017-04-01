#Websocket example

http://www.barth-dev.de/websockets-on-the-esp32/

Somewhat works with qemu, use 128.0.0.1:10080 for websocket address.

#To start qemu, use the following.


xtensa-softmmu/qemu-system-xtensa -d unimp,guest_errors  -cpu esp32 -M esp32 -m 4M -kernel  ~/esp/qemu_esp32/examples/15_websocket/build/WebSocket_demo.elf  -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.1.0/24,host=192.168.1.40,hostfwd=tcp::10080-192.168.1.3:9998  -net dump,file=/tmp/vm0.pcap  -s -S  > io.txt