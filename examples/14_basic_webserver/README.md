

Good example from here

http://www.lucadentella.it/en/2017/07/08/esp32-20-webserver/


xtensa-softmmu/qemu-system-xtensa -d guest_errors,unimp  -cpu esp32 -M esp32 -m 4M -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.4.0/24,host=192.168.4.40,hostfwd=tcp::10080-192.168.4.3:80  -net dump,file=/tmp/vm0.pcap  -s   > io.txt 

http://127.0.0.1:10080/


Note, need to improve qemu network emulation. It works but you must wait.
Also if it does not work first time, try again.