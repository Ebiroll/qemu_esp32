The puropse of this example is to test the use a heltec LORA board as
a LoraWan Single channel gateway


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


# Linker problems

I have done similar function level replacements in the past using the existing esp-idf build system. So I don't think any additional support is required for this rather rare use case.

Approach 1)
You can add a replacement library with your custom printf before newlib is searched using EXTRA_LDFLAGS = -lmy_printf_lib to your Makefile.

Approach 2)
A linker will always first search object files and then library files. Use this behavior to add an object file (not a library!) to your project replacing for example printf with your own version. You then useEXTRA_LDFLAGS = my_printf.o to add this object file to the link process.
If you also want to replace a system header file, then this is possible too. Have a look at the various options gcc offers to inject additional include files (-include) or to influence the include search order.

Approach 3)
You could use the -Wl,--wrap,printf linker flag to inject your own version of printf. The linker will in this case resolve all calls to printf with a call to __wrap_printf. Your replacement version is then called __wrap_printf and the old newlib's is called __real_printf. I used this technique to replace newlib's malloc and free with a thread-safe version in another project unrelated to ESP32.