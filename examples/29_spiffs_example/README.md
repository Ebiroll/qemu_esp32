### Full example of using **SPIFFS** with ESP32

---

#### Features

* full implementation of **spiffs** with **VFS**
* **directories** are supported
* file **timestamp** is added to standard spiffs
* example of **list** functions
* example of **file copy** functions

*When using file related functions which has filename argument, prefix* **/spiffs/**  *has to be added to the file name.*

---

#### How to build

Configure your esp32 build environment as for other **esp-idf examples**

Clone the repository

`git clone https://github.com/loboris/ESP32_spiffs_example.git`

Execute menuconfig and configure your Serial flash config and other settings. Included *sdkconfig.defaults* sets some defaults to be used.

Navigate to **SPIffs Example Configuration** and set **SPIFFS** options.

Select if you want to use **wifi** (recommended) to get the time from **NTP** server and set your WiFi SSID and password.

`make menuconfig`

Make and flash the example.

`make all && make flash`

---

#### Prepare **SPIFFS** image

*It is not required to prepare the image, as the spiffs will be automatically formated on first use, but it might be convenient.*

SFPIFFS **image** can be prepared on host and flashed to ESP32.

**NEW**: Tested and works under **Linux**, **Windows** and **Mac OS**

Copy the files to be included on spiffs into **components/spiffs_image/image/** directory. Subdirectories can also be added.

Execute:

`make makefs`

to create **spiffs image** in build directory **without flashing** to ESP32

Execute:

`make flashfs`

to create **spiffs image** in *build* directory and **flash** it to ESP32

---

#### Example functions

* get the time from NTP server and set the system time (if WiFi is enabled)
* register spiffs as VFS file system; if the fs is not formated (1st start) it will be formated and mounted
* perform some file system tests
  * write text to file
  * read the file back
  * make directory
  * copy file
  * remove file
  * remove directory
  * list files in root directory and subdirectories


---

**Example output:**

```

I (1185) cpu_start: Pro cpu start user code
I (1240) cpu_start: Starting scheduler on PRO CPU.
I (1242) cpu_start: Starting scheduler on APP CPU.
I (1242) [SPIFFS example]: Time is not set yet. Connecting to WiFi and getting time over NTP.
I (1271) wifi: wifi firmware version: 6c86a1c
I (1271) wifi: config NVS flash: enabled
I (1271) wifi: config nano formating: disabled
I (1272) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1281) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1313) wifi: Init dynamic tx buffer num: 32
I (1313) wifi: Init dynamic rx buffer num: 32
I (1314) wifi: wifi driver task: 3ffc2324, prio:23, stack:4096
I (1316) wifi: Init static rx buffer num: 16
I (1320) wifi: Init dynamic rx buffer num: 32
I (1324) wifi: Init rx ampdu len mblock:7
I (1328) wifi: Init lldesc rx ampdu entry mblock:4
I (1332) wifi: wifi power manager task: 0x3ffc9cbc prio: 21 stack: 2560
I (1339) [SPIFFS example]: Setting WiFi configuration SSID LoBoInternet...
I (1346) wifi: wifi timer task: 3ffcad44, prio:22, stack:3584
I (1371) phy: phy_version: 350, Mar 22 2017, 15:02:06, 1, 0
I (1371) wifi: mode : sta (24:0a:c4:00:97:c0)
I (1492) wifi: n:1 0, o:1 0, ap:255 255, sta:1 0, prof:1
I (2150) wifi: state: init -> auth (b0)
I (2152) wifi: state: auth -> assoc (0)
I (2155) wifi: state: assoc -> run (10)
I (2189) wifi: connected with LoBoInternet, channel 1
I (4252) event: ip: 192.168.0.16, mask: 255.255.255.0, gw: 192.168.0.1
I (4252) [SPIFFS example]: Initializing SNTP
I (4253) [SPIFFS example]: Waiting for system time to be set... (1/10)
I (6260) [SPIFFS example]: System time is set.
I (6260) wifi: state: run -> init (0)
I (6260) wifi: n:1 0, o:1 0, ap:255 255, sta:1 0, prof:1
E (6262) wifi: esp_wifi_connect 816 wifi not start


I (7262) [SPIFFS example]: ==== STARTING SPIFFS TEST ====

I (7262) [SPIFFS]: Registering SPIFFS file system
I (7263) [SPIFFS]: Mounting SPIFFS files ystem
I (7268) [SPIFFS]: Start address: 0x180000; Size 1024 KB
I (7311) [SPIFFS]: Mounted


==== Write to file "/spiffs/test.txt" ====
     361 bytes written

==== Reading from file "/spiffs/test.txt" ====
     361 bytes read [
ESP32 spiffs write to file, line 1
ESP32 spiffs write to file, line 2
ESP32 spiffs write to file, line 3
ESP32 spiffs write to file, line 4
ESP32 spiffs write to file, line 5
ESP32 spiffs write to file, line 6
ESP32 spiffs write to file, line 7
ESP32 spiffs write to file, line 8
ESP32 spiffs write to file, line 9
ESP32 spiffs write to file, line 10

]

==== Reading from file "/spiffs/spiffs.info" ====
     405 bytes read [
INTRODUCTION

Spiffs is a file system intended for SPI NOR flash devices on embedded targets.
Spiffs is designed with following characteristics in mind:

  * Small (embedded) targets, sparse RAM without heap
  * Only big areas of data (blocks) can be erased
  * An erase will reset all bits in block to ones
  * Writing pulls one to zeroes
  * Zeroes can only be pulled to ones by erase
  * Wear leveling

]

LIST of DIR [/spiffs/]
T  Size      Date/Time         Name
-----------------------------------
d         -                    /spiffs
d         -  17/05/2017 13:00  images
f      8532  17/05/2017 13:00  testSpiffs.c
f       405  17/05/2017 13:00  spiffs.info
f       361  17/05/2017 15:05  test.txt
-----------------------------------
       9298 in 3 file(s)
-----------------------------------
SPIFFS: free 772 KB of 957 KB

==== Make new directory "/spiffs/newdir" ====
     Directory created

LIST of DIR [/spiffs/]
T  Size      Date/Time         Name
-----------------------------------
d         -                    /spiffs
d         -  17/05/2017 13:00  images
f      8532  17/05/2017 13:00  testSpiffs.c
f       405  17/05/2017 13:00  spiffs.info
f       361  17/05/2017 15:05  test.txt
d         -  17/05/2017 15:05  newdir
-----------------------------------
       9298 in 3 file(s)
-----------------------------------
SPIFFS: free 772 KB of 957 KB
     Copy file from root to new directory...

LIST of DIR [/spiffs/newdir]
T  Size      Date/Time         Name
-----------------------------------
f       361  17/05/2017 15:05  test.txt.copy
-----------------------------------
        361 in 1 file(s)
-----------------------------------
SPIFFS: free 771 KB of 957 KB
     Removing file from new directory...

LIST of DIR [/spiffs/newdir]
T  Size      Date/Time         Name
-----------------------------------
-----------------------------------
SPIFFS: free 772 KB of 957 KB
     Removing directory...

LIST of DIR [/spiffs/]
T  Size      Date/Time         Name
-----------------------------------
d         -                    /spiffs
d         -  17/05/2017 13:00  images
f      8532  17/05/2017 13:00  testSpiffs.c
f       405  17/05/2017 13:00  spiffs.info
f       361  17/05/2017 15:05  test.txt
-----------------------------------
       9298 in 3 file(s)
-----------------------------------
SPIFFS: free 772 KB of 957 KB

==== List content of the directory "images" ====

LIST of DIR [/spiffs/images]
T  Size      Date/Time         Name
-----------------------------------
f     39310  17/05/2017 13:00  test1.jpg
f     50538  17/05/2017 13:00  test2.jpg
f     38460  17/05/2017 13:00  test3.jpg
f     47438  17/05/2017 13:00  test4.jpg
-----------------------------------
     175746 in 4 file(s)
-----------------------------------
SPIFFS: free 772 KB of 957 KB



```
