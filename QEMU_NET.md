# Qemu backend network options:
```  
    -netdev user,id=str[,ipv4[=on|off]][,net=addr[/mask]][,host=addr]
         [,ipv6[=on|off]][,ipv6-net=addr[/int]][,ipv6-host=addr]
         [,restrict=on|off][,hostname=host][,dhcpstart=addr]
         [,dns=addr][,ipv6-dns=addr][,dnssearch=domain][,tftp=dir]
         [,bootfile=f][,hostfwd=rule][,guestfwd=rule][,smb=dir[,smbserver=addr]]
                configure a user mode network backend with ID 'str',
                its DHCP server and optional services
    -netdev tap,id=str[,fd=h][,fds=x:y:...:z][,ifname=name][,script=file][,downscript=dfile]
         [,helper=helper][,sndbuf=nbytes][,vnet_hdr=on|off][,vhost=on|off]
         [,vhostfd=h][,vhostfds=x:y:...:z][,vhostforce=on|off][,queues=n]
         [,poll-us=n]
                configure a host TAP network backend with ID 'str'
                use network scripts 'file' (default=/etc/qemu-ifup)
                to configure it and 'dfile' (default=/etc/qemu-ifdown)
                to deconfigure it
                use '[down]script=no' to disable script execution
                use network helper 'helper' (default=/home/olas/qemu_esp32/root/libexec/qemu-bridge-helper) to
                configure it
                use 'fd=h' to connect to an already opened TAP interface
                use 'fds=x:y:...:z' to connect to already opened multiqueue capable TAP interfaces
                use 'sndbuf=nbytes' to limit the size of the send buffer (the
                default is disabled 'sndbuf=0' to enable flow control set 'sndbuf=1048576')
                use vnet_hdr=off to avoid enabling the IFF_VNET_HDR tap flag
                use vnet_hdr=on to make the lack of IFF_VNET_HDR support an error condition
                use vhost=on to enable experimental in kernel accelerator
                    (only has effect for virtio guests which use MSIX)
                use vhostforce=on to force vhost on for non-MSIX virtio guests
                use 'vhostfd=h' to connect to an already opened vhost net device
                use 'vhostfds=x:y:...:z to connect to multiple already opened vhost net devices
                use 'queues=n' to specify the number of queues to be created for multiqueue TAP
                use 'poll-us=n' to speciy the maximum number of microseconds that could be
                spent on busy polling for vhost net 
                
    -netdev bridge,id=str[,br=bridge][,helper=helper]
                configure a host TAP network backend with ID 'str' that is
                connected to a bridge (default=br0)
                using the program 'helper (default=/home/olas/qemu_esp32/root/libexec/qemu-bridge-helper)



     To debug lwip set #define LWIP_DEBUG 1 in cc.h
     I think there is some problem with the heaps. When disabling debug of  heaps we get an error here
     heap_alloc_caps_init()
     vPortDefineHeapRegionsTagged.

     I (32079) heap_alloc_caps: Initializing heap allocator:
     I (32079) heap_alloc_caps: Region 19: 3FFB5640 len 0002A9C0 tag 0
     I (32080) heap_alloc_caps: Region 25: 3FFE8000 len 00018000 tag 1

     http://www.freertos.org/thread-local-storage-pointers.html

     Add this for debugging in ethernet_input,ethernet.c.

    printf("ethernet_input: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\n",
     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
     (unsigned)ethhdr->src.addr[0], (unsigned)ethhdr->src.addr[1], (unsigned)ethhdr->src.addr[2],
     (unsigned)ethhdr->src.addr[3], (unsigned)ethhdr->src.addr[4], (unsigned)ethhdr->src.addr[5],
     (unsigned)htons(ethhdr->type));


     // Some threads when running lwip and the echo task,
     "tiT"   tcpip
     "ech"
     "mai"   main task
     "IDL"   idle task
      Yes! Got it to work with this.
      xtensa-softmmu/qemu-system-xtensa -net nic,model=vlan0 -net user,id=simnet,ipver4=on,net=192.168.1.0/24,host=192.168.1.40,hostfwd=tcp::10077-192.168.1.3:7  -net dump,file=/tmp/vm0.pcap  -d guest_errors,unimp  -cpu esp32 -M esp32 -m 16M  -kernel  ~/esp/qemu_esp32/build/app-template.elf -s -S   > io.txt
      nc 127.0.0.1 10077 

      The options allows you to do connect to the echoserver
      in order to test the echo server that is running on port 7
      wireshark /tmp/vm0.pcap
      However with latest esp-idf it is broken, needs debugging.

```    


http://blog.vmsplice.net/2011/04/how-to-capture-vm-network-traffic-using.html