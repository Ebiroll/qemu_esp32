#Use of U8G2 library with ssd1306
This example runs on hardware and can also run in qemu if I2C_MASTER_NUM is set to I2C_MASTER_NUM I2C_NUM_0

Olikraus now also has hw i2c for SSD 1306
https://github.com/olikraus/u8g2/commit/b9b3a65c4f2b4e25adf3bd53bd4ada1a29499072
This code might have to be reworked.


[(!image)https://raw.githubusercontent.com/Ebiroll/qemu_esp32/master/img/1306.jpg]

```
(qemu)  00000000 to memory, 3F400000
TRYING to MAP esp32flash.bin
MAPPED esp32flash.bin
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
[0;32mI (0) cpu_start: Pro cpu up.[0m
[0;32mI (0) cpu_start: Single core mode[0m
[0;32mI (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:[0m
[0;32mI (1) heap_alloc_caps: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM[0m
[0;32mI (1) heap_alloc_caps: At 3FFB2290 len 0002DD70 (183 KiB): DRAM[0m
[0;32mI (1) heap_alloc_caps: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM[0m
[0;32mI (2) heap_alloc_caps: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM[0m
[0;32mI (2) heap_alloc_caps: At 400895FC len 00016A04 (90 KiB): IRAM[0m
[0;32mI (2) cpu_start: Pro cpu start user code[0m
[0;32mI (4) cpu_start: Starting scheduler on PRO CPU.[0m
[0;32mI (5) ssd1306: u8g2_InitDisplay[0m
[0;32mI (5) u8g2_hal: sda_io_num 21[0m
[0;32mI (5) u8g2_hal: scl_io_num 22[0m
[0;32mI (5) u8g2_hal: clk_speed 50000[0m
[0;32mI (5) u8g2_hal: i2c_param_config 1[0m
[0;32mI (5) u8g2_hal: i2c_driver_install 0[0m
i2c apb write 1301c,78 
i2c apb write 1301c,0 
i2c apb write 1301c,ae 
i2c apb write 1301c,78 
i2c apb write 1301c,0 
i2c apb write 1301c,d5 
i2c apb write 1301c,78 
i2c apb write 1301c,0 
..etc

```
i2c_isr_handler_default
# 1306 Init sequence,
```
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  U8X8_C(0x0ae),		                /* display off */
  U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  U8X8_CA(0x0a8, 0x03f),		/* multiplex ratio */
  U8X8_CA(0x0d3, 0x000),		/* display offset */
  U8X8_C(0x040),		                /* set display start line to 0 */
  U8X8_CA(0x08d, 0x014),		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable */
  U8X8_CA(0x020, 0x000),		/* page addressing mode */
  
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // U8X8_C(0x0a0),				/* segment remap a0/a1*/
  // U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  U8X8_CA(0x081, 0x0cf), 		/* [2] set contrast control */
  U8X8_CA(0x0d9, 0x0f1), 		/* [2] pre-charge period 0x022/f1*/
  U8X8_CA(0x0db, 0x040), 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  U8X8_C(0x02e),				/* Deactivate scroll */ 
  U8X8_C(0x0a4),				/* output ram to display */
  U8X8_C(0x0a6),				/* none inverted normal display mode */
    
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
	
```



```
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
I (1) heap_alloc_caps: Initializing. RAM available for dynamic allocation:
I (1) heap_alloc_caps: At 3FFB5200 len 0002AE00 (171 KiB): DRAM
I (1) heap_alloc_caps: At 3FFE8000 len 00018000 (96 KiB): D/IRAM
I (1) heap_alloc_caps: At 40093DBC len 0000C244 (48 KiB): IRAM
I (2) cpu_start: Pro cpu up.
I (2) cpu_start: Single core mode
I (2) cpu_start: Pro cpu start user code
I (4) cpu_start: Starting scheduler on PRO CPU.
I (5) ssd1306: u8g2_InitDisplay
I (5) u8g2_hal: sda_io_num 21
I (5) u8g2_hal: scl_io_num 22
I (5) u8g2_hal: clk_speed 50000
I (5) u8g2_hal: i2c_param_config 1
I (5) u8g2_hal: i2c_driver_install 1
E (1305) err: esp_err_t = 263
assertion "0 && "i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS)"" failed: file "/home/olas/esp/qemu_esp32/examples/06_oledu8g2/main/./u8g2_esp32_hal.c", line 156, function: u8g2_esp32_msg_i2c_cb
abort() was called at PC 0x400de367
Guru Meditation Error: Core  0 panic'ed (abort)

Backtrace: 0x400de3ce:0x3ffb9950 0x40084001:0x3ffb9970 0x400eef15:0x3ffb99a0 0x400f3629:0x3ffb99e0 0x400f28ce:0x3ffb9a00 0x400f2280:0x3ffb9a30 0x400f2214:0x3ffb9a50 0x400f22e0:0x3ffb9a80 0x400f23e3:0x3ffb9aa0 0x400f3220:0x3ffb9ac0 0x400eeb9a:0x3ffb9ae0

Rebooting...
```


#ESP32 U8G2 library support
There is an excellent open source library called `u8g2` that can be found on Github here:

[https://github.com/olikraus/u8g2](https://github.com/olikraus/u8g2)

The purpose of the library is to provide a display independent driver layer for monochrome displays including LCD and OLED.
The library "knows" how to driver the underlying displays as well as providing drawing primitives including text, fonts, lines and
other geometrical shapes.

A version of the library is available in cleanly compiling C and compiles without incident on the ESP32-IDF framework.

However, since the library is agnostic of MCU environments and will work on a variety of boards, there has to be a mapping from
the functions expected by the library and the underlying MCU board hardware.  This includes driving GPIOs, I2C, SPI and more.

The code in this folder provides a mapping from U8g2 to the ESP32 ESP-IDF.  This should be included in your build of U8g2 applications.

To use with the ESP32, we must invoke the `u8g2_esp32_hal_init()` function before invoking any of the normal U8g2 functons.  What
this call does is tell the ESP32 what pins we wish to map.  Here is an example of use:

```
u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
u8g2_esp32_hal.clk   = PIN_CLK;
u8g2_esp32_hal.mosi  = PIN_MOSI;
u8g2_esp32_hal.cs    = PIN_CS;
u8g2_esp32_hal.dc    = PIN_DC;
u8g2_esp32_hal.reset = PIN_RESET;
u8g2_esp32_hal_init(u8g2_esp32_hal);
```

The function takes as input a `u8g2_esp32_hal` structure instance which has the logical pins that U8g2 needs mapped to the
physical pin on the ESP32 that we wish to use.  If we don't use a specific pin for our specific display, set the value to
be `U8G2_ESP32_HAL_UNDEFINED` which is the default initialization value.

Remember, ESP32 pins are not hard-coded to functions and as such, all the GPIO pins on the ESP32 are open for use.  Following
this initialization, we can use U8g2 as normal and described in the U8g2 documentation.

##Compiling U8G2
To use the actual U8g2 library in your ESP32 project, perform the following steps:

1. Create a directory called `components` in your main project directory.
2. Change into the `components` directory.
3. Run `git clone https://github.com/olikraus/u8g2.git` to bring in a the latest copy of u8g2 library.
4. Change into the `u8g2` directory.
5. Create a file called `component.mk`
6. Enter the following in the `component.mk` file:
```
COMPONENT_SRCDIRS:=csrc
COMPONENT_ADD_INCLUDEDIRS:=csrc
```


##Development
While in principal, there should be nothing specific needed beyond this addition to make U8g2 work on the ESP32, only a small
number of boards have been tested.  In addition, currently only SPI displays have been tested.  This will be remidied over time.


https://iotexpert.com/2016/12/26/pinball-oled-display/