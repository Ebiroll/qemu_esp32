Seed studio 12864 OLED, 
Probably a SSG1106 controller chip.
This is for future emulation of 1106

mkdir -p components && \
cd components && \
git clone https://github.com/espressif/arduino-esp32.git arduino && \
cd .. && \
make menuconfig

Select     "Autostart Arduino setup and loop on boot"

If you get this,
 (0) flash_parts: partition 3 invalid - offset 0x150000 size 0x140000 exceeds flash chip size 0x200000
E (0) boot: Failed to verify partition table
E (0) boot: load partition table error!
user code done

 Serial flasher config  ---> 
   Flash size (4 MB)  --->        





SSG1106 compatible with SSD1306 basic, difference is that SSH1106 control chip RAM space is 132*64, while SSD1306 space is 128*64.

The 1.3-inch OLED 128*64 dot matrix, so in the middle of the screen production took 128 row. When using SSD1306 program point SSH1106 screen, only need to change address to 0x02 row to start.

The SH1106 controller has an internal RAM of 132x64 pixel (while the SSD1306 has 128x64 pixel).

It seems, that the 128x64 OLED is centered in most cases within the 132x64 area, that means pixel (2,0) in ram is pixel (0,0) on the display.