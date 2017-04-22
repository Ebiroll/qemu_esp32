#  UDP 

Uses a POSIX socket to send UDP data

To build with emulated network support,
    ln -s ../../components components

   xtensa-softmmu/qemu-system-xtensa -d unimp -net nic,model=vlan0 -net user,ipver4=on,net=192.168.4.0/24,host=192.168.4.1,hostfwd=udp::6001-192.168.4.3:6001 -net dump,file=/tmp/vm0.pcap  -cpu esp32 -M esp32 -m 4M  -kernel ~/esp/qemu_esp32/examples/19_udp/build/app-template.elf  -s    > io.txt

To examine packages,
    wireshark /tmp/vm0.pcap

