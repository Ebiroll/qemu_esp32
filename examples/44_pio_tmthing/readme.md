This example uses this board.

https://github.com/LilyGO/TTGO-TM-ESP32

![Screenshoot of download tool](https://raw.githubusercontent.com/Ebiroll/qemu_esp32/master/examples/44_pio_tmthing/TM.jpg)


# Amazingly confusing information
The following seems to be valid.
TFT, 2.4" ST7789V LCD, connected over SPI (2)
Audio PCM5102A connected over i2s and i2c.
SD-card reader.


We use this library

https://github.com/loboris/ESP32_TFT_library

# Building

make
make makefs
make flashfs
make flash

https://www.esp32.com/viewtopic.php?t=6089

# Source
Here are some (good?) information of the pins for the board.

https://github.com/karawin/Ka-Radio32


# Display, ST7789V

https://www.newhavendisplay.com/appnotes/datasheets/LCDs/ST7789V.pdf

Name | type  | pin
-----| ------ | ----- 
K_SPI |	 	u8 | 	2
P_MISO	| u8 |	19
P_MOSI	| u8 |	23
P_CLK	| u8 |	18
P_LCD_CS	| u8 |	5
P_LCD_RST	| u8 |	17
P_LCD_A0	| u8 |	16

Not sure what the LCD_A0 pin does.


# Buttons 

Name | type  | pin
-----| ------ | ----- 
P_BTN0_A	| u8 |	37
P_BTN0_B	| u8 |	38
P_BTN0_C	| u8 |	39


# Audio PCM5102A

Audio PCM5102A i2s chip.
http://www.ti.com/lit/ds/symlink/pcm5102a.pdf

It is connected to the esp32 for i2s for audio data. 


Name | type  | pin
-----| ------ | ----- 
P_I2S_LRCK	| u8 |	25
P_I2S_BCLK	| u8 |	26
P_I2S_DATA |  u8	| 19


No I2C interface to this chip. Audio format is selected by one of the pins on the PCM5102A.

# SD card
Name | type  | pin
-----| ------ | ----- 
P_SD_DAT3	| u8 |	13
P_SD_CMD	| u8 |	15
P_SD_CLK	| u8 |	14
P_SD_DAT0	| u8 |	02
P_XDCS	| u8 |	2

# Others, unidentified
-----| ------ | ----- 
P_RST	| u8 |	33
P_DREQ	| u8 |	14
P_ENC1_B	| u8 |	17
P_I2C_RST	| u8 |	17
P_IR_SIGNAL	| u8 |	34

# Not used
Name | type  | pin
-----| ------ | ----- 
P_ADC	| u8 |	255
P_I2C_SCL	| u8 |	5
P_I2C_SDA	| u8 |	16



The display model is: 2.4" ST7789V LCD
Button keys: pin 37 pin 38 pin39
ESP32 and LED screens on controllers compatible with ILI9341.



http://www.ti.com/lit/ds/symlink/pcm5102a.pdf


Audio PCM5102A Generic wiring:

ESP pin   - I2S signal
----------------------
GPIO25/DAC1   - LRCK
GPIO26/DAC2   - BCLK
GPIO22        - DATA


Also info was collected here
https://github.com/kodera2t/ESP32_OLED_webradio

For the boards with ESP32-PICO-D4, please swap control switch from GPIO16 to GPIO0, since GPIO16 in PICO-D4 is used for internal SPI Flash RAM connection (pre-occupied). Swap can be done in components/controls/controls.c

```
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


Not sure about the PIN_NUM_MISO here, should be 19?
These did a job for me (->TFT control; not sound/SD)
PIN_NUM_MISO 0 // SPI MISO
PIN_NUM_MOSI 23 // SPI MOSI / SDA
PIN_NUM_CLK 18 // SPI CLOCK pin
PIN_NUM_CS 05 // Display CS pin

PIN_NUM_DC 16 // Display command/data pin / RS
PIN_NUM_TCS 0 // Touch screen CS pin
PIN_NUM_RST 17 // GPIO used for RESET control
PIN_NUM_BCKL 0 // GPIO used for backlight control
```