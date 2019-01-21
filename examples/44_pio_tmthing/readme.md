This example results in a mirrired display...

https://github.com/LilyGO/TTGO-TM-ESP32



# Amazingly confusing information
=================================




https://www.esp32.com/viewtopic.php?t=6089




The display model is: 2.4" ST7789V LCD
Button keys: pin 37 pin 38 pin39

ESP32 and LED screens on controllers compatible with ILI9341.



Audio PCM5102A Generic wiring:

http://www.ti.com/lit/ds/symlink/pcm5102a.pdf

ESP pin   - I2S signal
----------------------
GPIO25/DAC1   - LRCK
GPIO26/DAC2   - BCLK
GPIO22        - DATA


Info from here
https://github.com/kodera2t/ESP32_OLED_webradio

For the boards with ESP32-PICO-D4, please swap control switch from GPIO16 to GPIO0, since GPIO16 in PICO-D4 is used for internal SPI Flash RAM connection (pre-occupied). Swap can be done in components/controls/controls.c



==============================
Pins used: miso=19, mosi=23, sck=18, cs=5
==============================

SPI: display device added to spi bus (2)
SPI: attached display device, speed=8000000
SPI: bus uses native pins: true
SPI: display init...
OK
SPI: Max rd speed = 1000000
SPI: Changed speed to 40000000



These did a job for me (->TFT control; not sound/SD)
PIN_NUM_MISO 0 // SPI MISO
PIN_NUM_MOSI 23 // SPI MOSI / SDA
PIN_NUM_CLK 18 // SPI CLOCK pin
PIN_NUM_CS 05 // Display CS pin

PIN_NUM_DC 16 // Display command/data pin / RS
PIN_NUM_TCS 0 // Touch screen CS pin
PIN_NUM_RST 17 // GPIO used for RESET control
PIN_NUM_BCKL 0 // GPIO used for backlight control