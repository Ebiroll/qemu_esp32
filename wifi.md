#Trace 

http://git.qemu-project.org/?p=qemu.git;a=blob_plain;f=docs/tracing.txt;hb=HEAD

../qemu-xtensa/configure --enable-trace-backends=simple  --prefix=`pwd`/root --target-list=xtensa-softmmu,xtensaeb-softmmu

echo trace_open_eth_reg_read > /tmp/events


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
#In wifi_init() 


  esp-idf/components/nvs_flash/src/nvs_api.cpp
    304         size_t dataSize;           
    305         err = s_nvs_storage.getItemDataSize(entry.mNsIndex, type, key, dataSize);  
    306         if (err != ESP_OK) {                   
  > 307             return err;                          
    308         }                                                     

0  nvs_get_str_or_blob (handle=1, type=nvs::ItemType::BLOB, key=0x3f4053b0 "log", out_value=0x3ffc5cb4, length=0x3ffc2a60)
    at /home/olas/esp/esp-idf/components/nvs_flash/src/nvs_api.cpp:307
#1  0x400f5fa0 in nvs_get_blob (handle=1, key=0x3f4053b0 "log", out_value=0x3ffc5cb4, length=0x3ffc2a60)
    at /home/olas/esp/esp-idf/components/nvs_flash/src/nvs_api.cpp:330
#2  0x400fbca8 in nvs_log_init ()
#3  0x400fbd8f in misc_nvs_load ()
#4  0x400fbda4 in misc_nvs_init ()
#5  0x400d7a62 in wifi_init ()


0x4008ab5c <set_chan_freq_sw_start+200> memw                     
0x4008ab5f <set_chan_freq_sw_start+203> l32i.n a8, a6, 0                
0x4008ab61 <set_chan_freq_sw_start+205> bgez   a8, 0x4008ab5c <set_chan_freq_sw_start+200>

0x880020c0

0x6000e0c4 


0x40084fc5 <ram_check_noise_floor+89>   memw   
0x40084fc8 <ram_check_noise_floor+92>   l32i.n a9, a11, 0         // wifi read 33c00 ..  0x980020b5
0x40084fca <ram_check_noise_floor+94>   movi.n a8, 1
0x40084fcc <ram_check_noise_floor+96>   sub    a9, a9, a12       // a12=0x980020b5
0x40084fcf <ram_check_noise_floor+99>   bltu   a13, a9, 0x40084fd4 <ram_check_noise_floor+104>
0x40084fd2 <ram_check_noise_floor+102>  movi.n a8, 0    
0x40084fd4 <ram_check_noise_floor+104>  memw         
0x40084fd7 <ram_check_noise_floor+107>  l32i.n a9, a10, 0                       
0x40084fd9 <ram_check_noise_floor+109>  extui  a8, a8, 0, 8
 0x40084fdc <ram_check_noise_floor+112>  bbsi   a9, 24, 0x40084fec <ram_check_noise_floor+128>
 0x40084fdf <ram_check_noise_floor+115>  beqz   a8, 0x40084fc5 <ram_check_noise_floor+89>   // 0x980020c0
0x40084fe2 <ram_check_noise_floor+118>  j      0x40084fea <ram_check_noise_floor+126>
0x40084fe5 <ram_check_noise_floor+121>  movi.n a8, 0 
0x40084fe7 <ram_check_noise_floor+123>  j      0x40084fec <ram_check_noise_floor+128>                 


misc_nvs_init ()
Here we enter gdbstub()

