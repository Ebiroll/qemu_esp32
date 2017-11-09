#  Examples to test qemu, esp32 or just for fun.
Most of these examples has run in qemu but they can break as
esp-idf evolves. All of them should run on the esp32.

pcbreflux, also has nice examples.
    https://github.com/pcbreflux/espressif/tree/master/esp32/app

    01_max_malloc, check how much memory that is possible to allocate
    02_uart, Uart testing
    03_dual, Test with 2 cores
    03_unstall, Expeiments with unstalling one core without help from esp-idf
    04_schedule,2 cores. Inspired by a bugreport, seems fixed in esp-idf now,
    05_queue, Test with queues
    06_duino, ssd1306 i2c run by the Arduino i2c driver
    06_ssd1306-esp-idf-i2c, not yet tested with qemu
    06_oledu8g2, ssd1306 but also other displays
    06_ssd1306, experimental code with experimental 1306 driver.
    07_flash_mmap, test of mmap function
    07_flash_test, test of reading/writing to the flash
    07_nvs_rw_value, test of reading/writing to the flash with nv-library 
    08_nvs_rw_blob, test of reading/writing binary data with nv-library 
    09_onchip_sensor, test of temp sensor
    10_i2cscan, i2c scan with esp-idf i2c driver
    11_hvacIli9341, Test of the ESP WROVER Ili9341 display
    12_distance, Mini project with ultrasonic sensor and 1306 display
    14_bootwifi, Useful. Taken from here https://github.com/nkolban/esp32-snippets
    15_websocket, Websocket example
    16_wifi_sniffer, Wifi sniffer
    17_wifi_scan, Scans available AP, also supports WIFI_PROTOCOL_LR
    18_long_range, Experiments with undocumented WIFI_PROTOCOL_LR
    19_udp, example of udp with lwip. (Not finished yet)
    20_ext_wakeup, Sleep mode with external wakeup, (Needs review after esp-idf changes)
    20_timer_wakeup, Sleep mode with timer wakeup, (Needs review)
    21_can, can driver example. Not runnable in QEMU
    22_gsm_pppos, connect to gprs over gsm module, reqires patched esp-idf
    23_ota, Over the AIR flashing example
    24_esp32_explorer, (Not yet finnished)
    24_wear_leveling, wear leveling example
    25_tbconsole, Tiny basic console. Only works on hardware
    26_wifigdb, Serial debugger now running on wifi 
    28_gdb_blackmagic, Arm blackmagic gdb project (work in progress)
    29_spiffs_example, Spiff filesystem running 
    29_webspiff, Spiff filesystem with webserver demonstraring the MMU
    30_arduino, simplest possible arduino library example
    31_esp32-csid, SID audio example not yet tested.
    32_helltec, heltec lora and OLED
    32_lora, Semtec lora SPI example
    33_pkt_fwd, Fututre project to test as lora gateway
    34_9DoF, (in progress) Arduino library for NXP 9DOF breakout board
    35_gps, (in progress) Arduino library for gps