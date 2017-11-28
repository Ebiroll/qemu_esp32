#Simple example of SX1276 LoRa® based module with SPI interface

SX1272 taken from here
https://github.com/Waziup/waziup-gateway

Better code could possibly be located here,
http://www.airspayce.com/mikem/arduino/RadioHead/

#Lora wan datasheet
http://www.semtech.com/images/datasheet/an1200.24.pdf


# You must checkout the arduino helpfiles
cd components && \
git clone https://github.com/espressif/arduino-esp32.git arduino && \
cd .. && \
make menuconfig



make menuconfig has some Arduino options

    "Autostart Arduino setup and loop on boot"

        If you enable this options, your main.cpp should be formated like any other sketch

	


        Another option not used in this example, you need to implement app_main() and call initArduino(); in it.


#Use the arduino library

cd components
git clone https://github.com/espressif/arduino-esp32.git arduino 




