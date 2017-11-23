The puropse of this example is to test the gps arduion library

http://wiki.paparazziuav.org/wiki/Sensors/GPS



in the project folder, create a folder called components and clone this repository inside

mkdir -p components && \
cd components && \
git clone https://github.com/espressif/arduino-esp32.git arduino && \
cd .. && \
make menuconfig



make menuconfig has some Arduino options

    "Autostart Arduino setup and loop on boot"

        If you enable this options, your main.cpp should be formated like any other sketch

	


        Another option not used in this example, you need to implement app_main() and call initArduino(); in it.

        Keep in mind that setup() and loop() will not be called in this case. If you plan to base your code on examples provided in esp-idf, please make sure move the app_main() function in main.cpp from the files in the example.

        //file: main.cpp
        #include "Arduino.h"

        extern "C" void app_main()
        {
            initArduino();
            pinMode(4, OUTPUT);
            digitalWrite(4, HIGH);
            //do your own thing
        }

    "Disable mutex locks for HAL"
        If enabled, there will be no protection on the drivers from concurently accessing them from another thread/interrupt/core
    "Autoconnect WiFi on boot"
        If enabled, WiFi will start with the last known configuration
        Else it will wait for WiFi.begin

make flash monitor will build, upload and open serial monitor to your board
