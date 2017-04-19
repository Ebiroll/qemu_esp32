/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "tftfunc.h"


/*
 This code tests accessing ILI9341 based display and prints some timings

 Some fancy graphics is displayed on the ILI9341-based 320x240 LCD

 Reading the display content is demonstrated.
 
 If Touch screen is available, reading the touch coordinates (non calibrated) is also demonstrated.
*/

// comment this if your display does not have touch screen
#define USE_TOUCH

// define which spi bus to use VSPI_HOST or HSPI_HOST
#define SPI_BUS VSPI_HOST

#define DELAY 0x80


// We can use pre_cb callback to activate DC, but we are handling DC in low-level functions, so it is not necessary

// This function is called just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
/*
//-----------------------------------------------------------------------------
static void IRAM_ATTR ili_spi_pre_transfer_callback(spi_nodma_transaction_t *t) 
{
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}
*/

// Init for ILI7341
// ------------------------------------
static const uint8_t ILI9341_init[] = {
  23,                   					        // 23 commands in list
  ILI9341_SWRESET, DELAY,   						//  1: Software reset, no args, w/delay
  200,												//     200 ms delay
  ILI9341_POWERA, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  ILI9341_POWERB, 3, 0x00, 0XC1, 0X30,
  0xEF, 3, 0x03, 0x80, 0x02,
  ILI9341_DTCA, 3, 0x85, 0x00, 0x78,
  ILI9341_DTCB, 2, 0x00, 0x00,
  ILI9341_POWER_SEQ, 4, 0x64, 0x03, 0X12, 0X81,
  ILI9341_PRC, 1, 0x20,
  ILI9341_PWCTR1, 1, 0x23,							//Power control VRH[5:0]
  ILI9341_PWCTR2, 1, 0x10,							//Power control SAP[2:0];BT[3:0]
  ILI9341_VMCTR1, 2, 0x3e, 0x28,					//VCM control
  ILI9341_VMCTR2, 1, 0x86,							//VCM control2
  TFT_MADCTL, 1,									// Memory Access Control (orientation)
  (MADCTL_MV | MADCTL_BGR),
  ILI9341_PIXFMT, 1, 0x55,
  ILI9341_FRMCTR1, 2, 0x00, 0x18,
  ILI9341_DFUNCTR, 3, 0x08, 0x82, 0x27,				// Display Function Control
  TFT_PTLAR, 4, 0x00, 0x00, 0x01, 0x3F,
  ILI9341_3GAMMA_EN, 1, 0x00,						// 3Gamma Function Disable (0x02)
  ILI9341_GAMMASET, 1, 0x01,						//Gamma curve selected
  ILI9341_GMCTRP1, 15,   							//Positive Gamma Correction
  0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
  ILI9341_GMCTRN1, 15,   							//Negative Gamma Correction
  0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  ILI9341_SLPOUT, DELAY, 							//  Sleep out
  120,			 									//  120 ms delay
  TFT_DISPON, 0,
};

//----------------------------------------------------
// Companion code to the above table. Reads and issues
// a series of LCD commands stored in byte array
//---------------------------------------------------------------------------
static void commandList(spi_nodma_device_handle_t spi, const uint8_t *addr) {
  uint8_t  numCommands, numArgs, cmd;
  uint16_t ms;

  numCommands = *addr++;         // Number of commands to follow
  while(numCommands--) {         // For each command...
    cmd = *addr++;               // save command
    numArgs  = *addr++;          //   Number of args to follow
    ms       = numArgs & DELAY;  //   If high bit set, delay follows args
    numArgs &= ~DELAY;           //   Mask out delay bit

	disp_spi_transfer_cmd_data(spi, cmd, (uint8_t *)addr, numArgs);

	addr += numArgs;

    if(ms) {
      ms = *addr++;              // Read post-command delay time (ms)
      if(ms == 255) ms = 500;    // If 255, delay for 500 ms
	  vTaskDelay(ms / portTICK_RATE_MS);
    }
  }
}

//Initialize the display
//-------------------------------------------------
static void ili_init(spi_nodma_device_handle_t spi) 
{
    esp_err_t ret;
    //Initialize non-SPI GPIOs
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
	/* Reset and backlit pins are not used
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    //Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_RATE_MS);
	*/

	//Send all the commands
	ret = spi_nodma_device_select(spi, 0);
	assert(ret==ESP_OK);

    commandList(spi, ILI9341_init);

	ret = spi_nodma_device_deselect(spi);
	assert(ret==ESP_OK);

    ///Enable backlight
    //gpio_set_level(PIN_NUM_BCKL, 0);
}

#ifdef USE_TOUCH
//Send a command to the Touch screen
//---------------------------------------------------------------
uint16_t ts_cmd(spi_nodma_device_handle_t spi, const uint8_t cmd) 
{
    esp_err_t ret;
	spi_nodma_transaction_t t;
	uint8_t rxdata[2] = {0};

	// send command byte & receive 2 byte response
	memset(&t, 0, sizeof(t));            //Zero out the transaction
    t.rxlength=8*2;
    t.rx_buffer=&rxdata;
	t.command = cmd;

	ret = spi_nodma_transfer_data(spi, &t);    // Transmit using direct mode
    assert(ret==ESP_OK);                 //Should have had no issues.
	//printf("TS: %02x,%02x\r\n",rxdata[0],rxdata[1]);

	return (((uint16_t)(rxdata[0] << 8) | (uint16_t)(rxdata[1])) >> 4);
}
#endif

/**
 * Converts the components of a color, as specified by the HSB
 * model, to an equivalent set of values for the default RGB model.
 * The _sat and _brightness components
 * should be floating-point values between zero and one (numbers in the range 0.0-1.0)
 * The _hue component can be any floating-point number.  The floor of this number is
 * subtracted from it to create a fraction between 0 and 1.
 * This fractional number is then multiplied by 360 to produce the hue
 * angle in the HSB color model.
 * The integer that is returned by HSBtoRGB encodes the
 * value of a color in bits 0-15 of an integer value
*/
//-------------------------------------------------------------------
static uint16_t HSBtoRGB(float _hue, float _sat, float _brightness) {
 float red = 0.0;
 float green = 0.0;
 float blue = 0.0;

 if (_sat == 0.0) {
   red = _brightness;
   green = _brightness;
   blue = _brightness;
 } else {
   if (_hue == 360.0) {
     _hue = 0;
   }

   int slice = (int)(_hue / 60.0);
   float hue_frac = (_hue / 60.0) - slice;

   float aa = _brightness * (1.0 - _sat);
   float bb = _brightness * (1.0 - _sat * hue_frac);
   float cc = _brightness * (1.0 - _sat * (1.0 - hue_frac));

   switch(slice) {
     case 0:
         red = _brightness;
         green = cc;
         blue = aa;
         break;
     case 1:
         red = bb;
         green = _brightness;
         blue = aa;
         break;
     case 2:
         red = aa;
         green = _brightness;
         blue = cc;
         break;
     case 3:
         red = aa;
         green = bb;
         blue = _brightness;
         break;
     case 4:
         red = cc;
         green = aa;
         blue = _brightness;
         break;
     case 5:
         red = _brightness;
         green = aa;
         blue = bb;
         break;
     default:
         red = 0.0;
         green = 0.0;
         blue = 0.0;
         break;
   }
 }

 uint8_t ired = (uint8_t)(red * 31.0);
 uint8_t igreen = (uint8_t)(green * 63.0);
 uint8_t iblue = (uint8_t)(blue * 31.0);

 return (uint16_t)((ired << 11) | (igreen << 5) | (iblue & 0x001F));
}

//Simple routine to test display functions in DMA/transactions & direct mode
//--------------------------------------------------------------------------------------
static void display_test(spi_nodma_device_handle_t spi, spi_nodma_device_handle_t tsspi) 
{
    uint32_t speeds[7] = {5000000,8000000,16000000,20000000,30000000,40000000,80000000};
    int speed_idx = 0, max_speed=6, max_rdspeed=99;
    uint32_t change_speed, rd_clk;
	esp_err_t ret;
    uint16_t line[2][320];
    int x, y, ry;
	int line_check=0;
	uint16_t color;
    uint32_t t1=0,t2=0,t3=0, tstart;
	float hue_inc;
#ifdef USE_TOUCH
	int tx, ty, tz=0;
#endif

	while(1) {
		// *** Clear screen
		color = 0x0000;
		ret = spi_nodma_device_select(spi, 0);
		assert(ret==ESP_OK);
		disp_spi_transfer_addrwin(spi, 0, 319, 0, 239);
		disp_spi_transfer_cmd(spi, TFT_RAMWR);
		disp_spi_transfer_color_rep(spi, (uint8_t *)&color,  320*240, 1);

		color = 0xFFFF;
		for (y=0;y<240;y+=20) {
			disp_spi_transfer_addrwin(spi, 0, 319, y, y);
			disp_spi_transfer_color_rep(spi, (uint8_t *)&color,  320, 1);
		}
		for (x=0;x<320;x+=20) {
			disp_spi_transfer_addrwin(spi, x, x, 0, 239);
			disp_spi_transfer_color_rep(spi, (uint8_t *)&color,  240, 1);
		}
		disp_spi_transfer_addrwin(spi, 0, 319, 239, 239);
		disp_spi_transfer_color_rep(spi, (uint8_t *)&color,  320, 1);
		disp_spi_transfer_addrwin(spi, 319, 319, 0, 239);
		disp_spi_transfer_color_rep(spi, (uint8_t *)&color,  240, 1);
		vTaskDelay(1000 / portTICK_RATE_MS);

		ret = spi_nodma_device_deselect(spi);
		assert(ret==ESP_OK);

		// *** Send color lines
		ry = rand() % 239;
		tstart = clock();
		ret = spi_nodma_device_select(spi, 0);
		assert(ret==ESP_OK);
		line_check=-9999;
		for (y=0; y<240; y++) {
			hue_inc = (float)(((float)y / 239.0) * 360.0);
            for (x=0; x<320; x++) {
				color = HSBtoRGB(hue_inc, 1.0, (float)(x/640.0) + 0.5);
				line[0][x] = color; //(uint16_t)((color>>8) | (color & 0xFF));
			}
			disp_spi_transfer_addrwin(spi, 0, 319, y, y);
			disp_spi_transfer_color_rep(spi, (uint8_t *)(line[0]),  320, 0);
            if (y == ry) memcpy(line[1], line[0], 320*2);
		}
		t1 = clock() - tstart;
		// Check line
		if (speed_idx > max_rdspeed) {
			// Set speed to max read speed
			change_speed = spi_nodma_set_speed(spi, speeds[max_rdspeed]);
			assert(change_speed > 0 );
			ret = spi_nodma_device_select(spi, 0);
			assert(ret==ESP_OK);
			rd_clk = speeds[max_rdspeed];
		}
		else rd_clk = speeds[speed_idx];

		ret = disp_spi_read_data(spi, 0, ry, 319, ry, 320, (uint8_t *)(line[0]));
		if (ret == ESP_OK) {
            line_check = memcmp((uint8_t *)(line[0]), (uint8_t *)(line[1]), 320*2);
        }
		if (speed_idx > max_rdspeed) {
			// Restore spi speed
			change_speed = spi_nodma_set_speed(spi, speeds[speed_idx]);
			assert(change_speed > 0 );
		}

		ret =spi_nodma_device_deselect(spi);
		assert(ret==ESP_OK);
		if (line_check) {
			// Error checking line
			if (max_rdspeed > speed_idx) {
				// speed probably too high for read
				if (speed_idx) max_rdspeed = speed_idx - 1;
				else max_rdspeed = 0;
				printf("### MAX READ SPI CLOCK = %d ###\r\n", speeds[max_rdspeed]);
			}
			else {
				// Set new max spi clock, reinitialize the display
				if (speed_idx) max_speed = speed_idx - 1;
				else max_speed = 0;
				printf("### MAX SPI CLOCK = %d ###\r\n", speeds[max_speed]);
				speed_idx = 0;
				spi_nodma_set_speed(spi, speeds[speed_idx]);
				ili_init(spi);
				continue;
			}
        }
		vTaskDelay(1000 / portTICK_RATE_MS);

		// *** Display pixels
		uint8_t clidx = 0;
		color = 0xF800;
		tstart = clock();
		ret = spi_nodma_device_select(spi, 0);
		assert(ret==ESP_OK);
		for (y=0; y<240; y++) {
			for (x=0; x<320; x++) {
				disp_spi_set_pixel(spi, x, y, color);
			}
			if ((y > 0) && ((y % 40) == 0)) {
				clidx++;
				if (clidx > 5) clidx = 0;

				if (clidx == 0) color = 0xF800;
				else if (clidx == 1) color = 0x07E0;
				else if (clidx == 2) color = 0x001F;
				else if (clidx == 3) color = 0xFFE0;
				else if (clidx == 4) color = 0xF81F;
				else color = 0x07FF;
			}
		}
		ret = spi_nodma_device_deselect(spi);
		assert(ret==ESP_OK);
		t2 = clock() - tstart;
		vTaskDelay(1000 / portTICK_RATE_MS);
		
		// *** Clear screen
		color = 0xFFE0;
		tstart = clock();
		ret = spi_nodma_device_select(spi, 0);
		assert(ret==ESP_OK);
		disp_spi_transfer_addrwin(spi, 0, 319, 0, 239);
		disp_spi_transfer_cmd(spi, TFT_RAMWR);
		disp_spi_transfer_color_rep(spi, (uint8_t *)&color,  320*240, 1);
		ret = spi_nodma_device_deselect(spi);
		assert(ret==ESP_OK);
		t3 = clock() - tstart;
		vTaskDelay(1000 / portTICK_RATE_MS);

#ifdef USE_TOUCH
		// Get toush status
		//ret = spi_nodma_device_deselect(tsspi);
		//assert(ret==ESP_OK);
		tz = ts_cmd(tsspi, 0xB0);
		if (tz > 100) {
			// Touched, get coordinates
			tx = ts_cmd(tsspi, 0xD0);
			ty = ts_cmd(tsspi, 0x90);
		}
		//ret = spi_nodma_device_deselect(tsspi);
		//assert(ret==ESP_OK);
#endif

		// *** Print info
		printf("-------------\r\n");
		printf(" Disp clock = %5.2f MHz (requested: %5.2f)\r\n", (float)((float)(spi_nodma_get_speed(spi))/1000000.0), (float)(speeds[speed_idx])/1000000.0);
		printf("      Lines = %5d  ms (240 lines of 320 pixels)\r\n",t1);
		printf(" Read check   ");
		if (line_check == 0) printf("   OK, line %d", ry);
		else printf("  Err, line %d", ry);
		if (speed_idx > max_rdspeed) {
			printf(" (Read clock = %5.2f MHz)", (float)(rd_clk/1000000.0));
		}
		printf("\r\n");
		printf("     Pixels = %5d  ms (320x240)\r\n",t2);
		printf("        Cls = %5d  ms (320x240)\r\n",t3);
#ifdef USE_TOUCH
		if (tz > 100) {
			printf(" Touched at (%d,%d) [row TS values]\r\n",tx,ty);
		}
#endif
		printf("-------------\r\n");

		// Change SPI speed
		speed_idx++;
		if (speed_idx > max_speed) speed_idx = 0;
		change_speed = spi_nodma_set_speed(spi, speeds[speed_idx]);
		assert(change_speed > 0 );
    }
}

//-------------
void app_main()
{
    esp_err_t ret;
	
    spi_nodma_device_handle_t spi;
    spi_nodma_device_handle_t tsspi = NULL;
	
    spi_nodma_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };
    spi_nodma_device_interface_config_t devcfg={
        .clock_speed_hz=5000000,                //Initial clock out at 5 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=-1,                       //we will use external CS pin
		.spics_ext_io_num=PIN_NUM_CS,           //external CS pin
		//.flags=SPI_DEVICE_HALFDUPLEX,           //Set half duplex mode (Full duplex mode can also be set by commenting this line
												// but we don't need full duplex in  this example
		// We can use pre_cb callback to activate DC, but we are handling DC in low-level functions, so it is not necessary
        //.pre_cb=ili_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };

#ifdef USE_TOUCH
	spi_nodma_device_interface_config_t tsdevcfg={
        .clock_speed_hz=2500000,                //Clock out at 2.5 MHz
        .mode=0,                                //SPI mode 2
        .spics_io_num=PIN_NUM_TCS,              //Touch CS pin
		.spics_ext_io_num=-1,                   //Not using the external CS
		.command_bits=8,                        //1 byte command
    };
#endif

	vTaskDelay(500 / portTICK_RATE_MS);
	printf("\r\n===================================\r\n");
	printf("spi_master_nodma demo, LoBo 04/2017\r\n");
	printf("===================================\r\n\r\n");

	//Initialize the SPI bus and attach the LCD to the SPI bus
    ret=spi_nodma_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(ret==ESP_OK);
	printf("SPI: bus initialized\r\n");

	// Test select/deselect
	ret = spi_nodma_device_select(spi, 1);
    assert(ret==ESP_OK);
	ret = spi_nodma_device_deselect(spi);
    assert(ret==ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_nodma_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_nodma_uses_native_pins(spi) ? "true" : "false");

#ifdef USE_TOUCH
    //Attach the touch screen to the same SPI bus
    ret=spi_nodma_bus_add_device(SPI_BUS, &buscfg, &tsdevcfg, &tsspi);
    assert(ret==ESP_OK);

	// Test select/deselect
	ret = spi_nodma_device_select(tsspi, 1);
    assert(ret==ESP_OK);
	ret = spi_nodma_device_deselect(tsspi);
    assert(ret==ESP_OK);

	printf("SPI: attached TS device, speed=%u\r\n", spi_nodma_get_speed(tsspi));
#endif

	//Initialize the LCD
	printf("SPI: display init...\r\n");
    ili_init(spi);
	printf("OK\r\n");
	vTaskDelay(500 / portTICK_RATE_MS);
	
	display_test(spi, tsspi);
}
