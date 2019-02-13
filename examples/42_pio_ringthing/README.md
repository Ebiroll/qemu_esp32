# TTGO-T9-RGB_LED-WM8978

Parts of this was borrowed from here https://github.com/MisterRager/RingThing & https://github.com/LilyGO/TTGO-TAudio


![image](https://github.com/LilyGO/TTGO-TAudio/blob/master/Images/T9V1.5.jpg)

![image](https://github.com/LilyGO/TTGO-T9-RGB_LED-WM8978/blob/master/Images/image1.jpg)

# The code in here is based on the V1.0 hardware

Sound is not right, but at least we have some noice.

# Buttons

Name | type  | pin
-----| ------ | ----- 
P_BTNB1 | u8 |	RST
P_BTNB2	| u8 |	39
P_BTNB3	| u8 |	36
P_BTNB4	| u8 |	34

# Sound WM8978

Name | type  | pin
-----| ------ | ----- 
P_I2S_BCK	| u8 |	33
P_I2S_WS	| u8 |	25
P_I2S_IN	| u8 |	27
P_I2S_OUT	| u8 |	26

Not sure if the i2c pins are connected

Name | type  | pin
-----| ------ | ----- 
P_I2C_SDA	| u8 |	19
P_I2C_SCL	| u8 |	18


# SD Card

Name | type  | pin
-----| ------ | -----
P_SD_CS_DAT3  | u8 |	13
P_SD_CMD_MOSI	| u8 |	15
P_SD_CLK	| u8 |	14
P_SD_DAT0_MISO	| u8 |	02
P_SD_DAT1	| u8 |	04
P_SD_DAT2	| u8 |	12



# Info regarding timing of the neopixel
This code uses the remote device to get exact timings.


https://github.com/adafruit/Adafruit_NeoPixel/issues/139

https://hackaday.com/2017/01/20/cheating-at-5v-ws2812-control-to-use-a-3-3v-data-line/


  
