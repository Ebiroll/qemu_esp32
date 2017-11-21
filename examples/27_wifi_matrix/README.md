cd components
git clone https://github.com/espressif/arduino-esp32.git arduino && \
cd .. && \
make menuconfig

make menuconfig has some Arduino options

    "Autostart Arduino setup and loop on boot"

        If you enable this options, your main.cpp should be formated like any other sketch



Ideas from, 

https://github.com/googlecreativelab/anypixel

https://github.com/heisice/wifi_led_display

https://github.com/nkolban/esp32-snippets

// Html led
https://github.com/jlaso/js-cool-matrix-led

The Idea is to have a wifi controlled MAX 7219 led matrix 