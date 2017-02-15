Example of working ESP32 and 2.2 Inch SPI TFT LCD Serial Port Module Display ILI9341

Specifically configured for the ESP-WROVER-KIT

User Interface adapted from Sermus' implementation on ESP8266 (https://github.com/Sermus/ESP8266_Adafruit_ILI9341)

Tested against latest esp-idf and toolchain as of 2017-01-21

IMPORTANT: If you previously made copies of the following files from this repo to the `esp-idf` tree, you ought to remove them:

    esp-idf/components/driver/dma.c
    esp-idf/components/driver/hspi.c
    esp-idf/components/driver/spi.c
    esp-idf/components/driver/include/driver/dma.h
    esp-idf/components/driver/include/driver/hspi.h
    esp-idf/components/driver/include/driver/spi.h

You should check the git status for esp-idf afterward to ensure they are not present nor expected by git.
