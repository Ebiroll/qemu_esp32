# Wifi analysis
Stuck here
```

    #0  0x400d7345 in chip_pre_init ()
    #1  0x400d73cd in wifi_init ()
    #2  0x400d25f0 in esp_wifi_init ()


io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io read 3f0  DPORT_PRO_DCACHE_DBUG0_REG  3ff003F0=0x80
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000028
io write 40,20 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 40  DPORT_PRO_CACHE_CTRL_REG  3ff00040=00000020
io write 40,28 DPORT_PRO_CACHE_CTRL_REG 3ff00040
io read 44  DPORT DPORT_PRO_CACHE_CTRL1_REG  3ff00044=8E6
io write 44,8e6 
io write 1e8,a 
io write 47030,0 
io write 47028,84 
io write 47020,0 
io read 5a004 
io read 5a008 
io read d0 
io write d0,4 
io read d0 
io write d0,0 
io write cc,ffffffff 
io write 104,0 
```


# wifi_io 
New approach, map all memory access  0x60000000
```
static const MemoryRegionOps esp_wifi_ops = {
    .read = esp_wifi_read,
    .write = esp_wifi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};



    wifi_io = g_malloc(sizeof(*wifi_io));
    memory_region_init_io(wifi_io, NULL, &esp_wifi_ops, NULL, "esp32.wifi",
                          0x80000);

    memory_region_add_subregion(system_memory, 0x60000000, wifi_io);


Before app_main, this is read
wifi read 1f048 
wifi write 1f048,0 
wifi read f0a4 
wifi write f0a4,0 
wifi read f0a4 
wifi write f0a4,0 
wifi write e010,1c30b66 
wifi read e010 
wifi write e010,1740966 
wifi read e010 
wifi write e010,1cc0266 
wifi read e010 
wifi write e010,1900366 
wifi read e010 
wifi write e010,1400566 
wifi read e010 
```
#At init

misc_nvs_init ()
Here we enter gdbstub()

