#NXP 9DOF breakout example

https://learn.adafruit.com/nxp-precision-9dof-breakout/calibration-usb


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




