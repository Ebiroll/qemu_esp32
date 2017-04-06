#Examples to test qemu or just fun.
Most of these examples has run in qemu but they can break as
esp-idf evolves.

01_max_malloc, check how much memory that is possible to allocate
02_uart, Uart testing
03_dual, Test with 2 cores
03_unstall, Expeiments with unstalling one core without help from esp-idf
04_schedule,2 cores. Inspired by a bugreport, seems fixed in esp-idf now,
05_queue, Test with queues
06_duino, ssd1306 i2c run by the Arduino i2c driver
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
17_wifi_scan, Does not compile yet
20_ext_wakeup, Sleep mode with external wakeup
20_timer_wakeup, Sleep mode with timer wakeup
21_can, can driver example. Not runnable in QEMU
23_ota, Over the AIR flashing example

