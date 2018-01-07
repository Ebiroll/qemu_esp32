cd components
git clone https://github.com/espressif/arduino-esp32.git arduino && \
cd .. && \
make menuconfig

make menuconfig has some Arduino options

    "Autostart Arduino setup and loop on boot"

        If you enable this options, your main.cpp should be formated like any other sketch


I used a logic level transformation but it shold not be necessary.

https://www.aliexpress.com/item/10pcs-lot-Logic-Level-Shifter-Bi-Directional-Four-way-two-way-logic-level-transformation-module/32690066582.htm


https://www.aliexpress.com/item/3V-5V-Matrix-8X32-LED-Display-Module-Red-Dot-matrix-Screen-HT1632C-ATMEGA8-MCU/32477784337.html?spm=a2g0s.9042311.0.0.IlBj05


A better examle could probably be here,

http://www.lucadentella.it/category/ledmatrix_ht1632c/

https://github.com/lucadentella/LedMatrix



Ideas from, 

https://github.com/googlecreativelab/anypixel

https://github.com/heisice/wifi_led_display

https://github.com/nkolban/esp32-snippets

// Html led
https://github.com/jlaso/js-cool-matrix-led

The Idea is to have a wifi controlled MAX 7219 led matrix 