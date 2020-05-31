/* ePaper demo
 *
 *  Author: LoBo (loboris@gmail.com, loboris.github)
*/

#include <time.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
//#include "esp_heap_alloc_caps.h"
//#include "spiffs_vfs.h"
#include "esp_log.h"

#ifdef CONFIG_EXAMPLE_USE_WIFI

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_attr.h"
#include <sys/time.h>
#include <unistd.h>
#include "lwip/err.h"
#include "apps/sntp/sntp.h"
#include "nvs_flash.h"

#endif


#include "spi_master_lobo.h"
#include "img1.h"
#include "img2.h"
#include "img3.h"
#include "img_hacking.c"
#include "EPD.h"
//#include "EPDspi.h"

#define DELAYTIME 1500



static struct tm* tm_info;
static char tmp_buff[128];
static time_t time_now; //, time_last = 0;
//static const char *file_fonts[3] = {"/spiffs/fonts/DotMatrix_M.fon", "/spiffs/fonts/Ubuntu.fon", "/spiffs/fonts/Grotesk24x48.fon"};
//static const char tag[] = "[Eink Demo]";

//==================================================================================
#ifdef CONFIG_EXAMPLE_USE_WIFI


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = 0x00000001;

//------------------------------------------------------------
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

//-------------------------------
static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_LOGI(tag, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

//-------------------------------
static void initialize_sntp(void)
{
    ESP_LOGI(tag, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

//--------------------------
static int obtain_time(void)
{
	int res = 1;
    initialise_wifi();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    initialize_sntp();

    // wait for time to be set
    int retry = 0;
    const int retry_count = 20;

    time(&time_now);
	tm_info = localtime(&time_now);

    while(tm_info->tm_year < (2016 - 1900) && ++retry < retry_count) {
        //ESP_LOGI(tag, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
		vTaskDelay(500 / portTICK_RATE_MS);
        time(&time_now);
    	tm_info = localtime(&time_now);
    }
    if (tm_info->tm_year < (2016 - 1900)) {
    	ESP_LOGI(tag, "System time NOT set.");
    	res = 0;
    }
    else {
    	ESP_LOGI(tag, "System time is set.");
    }

    ESP_ERROR_CHECK( esp_wifi_stop() );
    return res;
}

#endif  //CONFIG_EXAMPLE_USE_WIFI
//==================================================================================


//=============
void app_main()
{

    // ========  PREPARE DISPLAY INITIALIZATION  =========

    esp_err_t ret;

	disp_buffer = heap_caps_calloc(EPD_DISPLAY_WIDTH * (EPD_DISPLAY_HEIGHT/8),1, MALLOC_CAP_DMA);
	assert(disp_buffer);
	drawBuff = disp_buffer;

	gs_disp_buffer = heap_caps_calloc(EPD_DISPLAY_WIDTH * EPD_DISPLAY_HEIGHT,1, MALLOC_CAP_DMA);
	assert(gs_disp_buffer);
	gs_drawBuff = gs_disp_buffer;

	// ====  CONFIGURE SPI DEVICES(s)  ====================================================================================

	gpio_set_direction(DC_Pin, GPIO_MODE_OUTPUT);
	gpio_set_level(DC_Pin, 1);
	gpio_set_direction(RST_Pin, GPIO_MODE_OUTPUT);
	gpio_set_level(RST_Pin, 0);
	gpio_set_direction(BUSY_Pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUSY_Pin, GPIO_PULLUP_ONLY);

#if POWER_Pin
	gpio_set_direction(POWER_Pin, GPIO_MODE_OUTPUT);
	gpio_set_level(POWER_Pin, 1);
#endif

    spi_lobo_bus_config_t buscfg={
        .miso_io_num = -1,				// set SPI MISO pin
        .mosi_io_num = MOSI_Pin,		// set SPI MOSI pin
        .sclk_io_num = SCK_Pin,			// set SPI CLK pin
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
		.max_transfer_sz = 5*1024,		// max transfer size is 4736 bytes
    };
    spi_lobo_device_interface_config_t devcfg={
        //.clock_speed_hz=40000000,		// SPI clock is 40 MHz
        .clock_speed_hz=6000000,        // SPI clock is 6 MHz/////////////////////////////
        .mode=0,						// SPI mode 0
        .spics_io_num=-1,				// we will use external CS pin
		.spics_ext_io_num = CS_Pin,		// external CS pin
		.flags=SPI_DEVICE_HALFDUPLEX,	// ALWAYS SET  to HALF DUPLEX MODE for display spi !!
    };

    // ====================================================================================================================


	vTaskDelay(500 / portTICK_RATE_MS);
	printf("\r\n=================================\r\n");
    printf("ePaper display DEMO, LoBo 06/2017\r\n");
	printf("=================================\r\n\r\n");

	// ==================================================================
	// ==== Initialize the SPI bus and attach the EPD to the SPI bus ====

	ret=spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &disp_spi);
    assert(ret==ESP_OK);
	printf("SPI: display device added to spi bus\r\n");

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(disp_spi, 1);
    assert(ret==ESP_OK);
	ret = spi_lobo_device_deselect(disp_spi);
    assert(ret==ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(disp_spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(disp_spi) ? "true" : "false");

	printf("\r\n-------------------\r\n");
	printf("ePaper demo started\r\n");
	printf("-------------------\r\n");


	EPD_DisplayClearFull();

#ifdef CONFIG_EXAMPLE_USE_WIFI

    ESP_ERROR_CHECK( nvs_flash_init() );

    EPD_DisplayClearPart();
	EPD_fillScreen(_bg);
	EPD_setFont(DEFAULT_FONT, NULL);
	sprintf(tmp_buff, "Waiting for NTP time...");
	EPD_print(tmp_buff, CENTER, CENTER);
	EPD_drawRect(10,10,274,108, EPD_BLACK);
	EPD_drawRect(12,12,270,104, EPD_BLACK);
	EPD_UpdateScreen();

	// ===== Set time zone ======
	setenv("TZ", "CET-1CEST", 0);
	tzset();
	// ==========================

    time(&time_now);
	tm_info = localtime(&time_now);

	// Is time set? If not, tm_year will be (1970 - 1900).
    if (tm_info->tm_year < (2016 - 1900)) {
        ESP_LOGI(tag, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        if (obtain_time()) {
        }
        else {
        }
        time(&time_now);
    }
#endif

    // ==== Initialize the file system ====
    printf("\r\n\n");
	/*
	vfs_spiffs_register();
    if (spiffs_is_mounted) {
        ESP_LOGI(tag, "File system mounted.");
    }
    else {
        ESP_LOGE(tag, "Error mounting file system.");
    }
	*/

	//=========
    // Run demo
    //=========

	/*
	EPD_DisplayClearFull();
    EPD_DisplayClearPart();
	EPD_fillScreen(_bg);
	//EPD_DisplaySetPart(0x00);
	//EPD_DisplaySetPart(0xFF);


    uint8_t LUTTest1[31]	= {0x32, 0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    uint8_t LUTTest2[31]	= {0x32, 0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	_gs = 0;
	_fg = 1;
	_bg = 0;
	int n = 0;
	while (1) {
		//EPD_DisplayClearFull();
		EPD_fillRect(14, 14, 100, 100, ((n&1) ? 0 : 1));
		EPD_fillRect(_width/2+14, 14, 100, 100, ((n&1) ? 1 : 0));
		//LUT_part = LUTTest1;
		EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
		//EPD_wait(2000);
		//LUT_part = LUTTest2;
		//EPD_DisplayFull(disp_buffer);
		printf("Updated\r\n");
		EPD_wait(4000);
		n++;


		n = 0;
		printf("\r\n==== FULL UPDATE TEST ====\r\n\n");
		EPD_DisplayClearFull();
		while (n < 2) {
			EPD_fillScreen(_bg);
			printf("Black\r\n");
			EPD_fillRect(0,0,_width/2,_height-1, EPD_BLACK);

			EPD_fillRect(20,20,_width/2-40,_height-1-40, EPD_WHITE);
			EPD_DisplayFull(disp_buffer);
			EPD_wait(4000);

			printf("White\r\n");
			EPD_fillRect(0,0,_width/2,_height-1, EPD_WHITE);
			EPD_DisplayFull(disp_buffer);
			EPD_wait(2000);
			n++;
		}

		printf("\r\n==== PARTIAL UPDATE TEST ====\r\n\n");
		EPD_DisplayClearFull();
		n = 0;
		while (n < 2) {
			EPD_fillScreen(_bg);
			printf("Black\r\n");
			EPD_fillRect(0,0,_width/2,_height-1, EPD_BLACK);

			EPD_fillRect(20,20,_width/2-40,_height-1-40, EPD_WHITE);
			EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
			EPD_wait(4000);

			printf("White\r\n");
			EPD_fillRect(0,0,_width/2,_height-1, EPD_WHITE);
			EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
			EPD_wait(2000);
			n++;
		}

		printf("\r\n==== PARTIAL UPDATE TEST - gray scale ====\r\n\n");
		EPD_DisplayClearFull();
		n = 0;
		while (n < 3) {
			EPD_fillScreen(_bg);
			LUT_part = LUT_gs;
			for (uint8_t sh=1; sh<16; sh++) {
				LUT_gs[21] = sh;
				printf("Black (%d)\r\n", LUT_gs[21]);
				EPD_fillRect((sh-1)*19,0,19,_height, EPD_BLACK);
				EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
			}
			EPD_wait(4000);

			//LUT_part = LUTDefault_part;
			printf("White\r\n");
			//EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
			//EPD_DisplaySetPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, 0xFF);
			LUT_gs[21] = 15;
			LUT_gs[1] = 0x28;
			EPD_fillRect(190,0,76,_height, EPD_WHITE);
			EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
			EPD_wait(2000);

			EPD_fillRect(0,0,_width,_height, EPD_WHITE);
			EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
			LUT_gs[1] = 0x18;
			EPD_wait(2000);
			n++;
		}
		LUT_part = LUTDefault_part;
	}
*/


	printf("==== START ====\r\n\n");

	_gs = 1;
	uint32_t tstart;
	int pass = 0, ftype = 9;
    while (1) {
    	ftype++;
    	if (ftype > 10) {
    		ftype = 1;
    		for (int t=4; t>0; t--) {
				printf("Wait %d seconds ...  \r", t);
				fflush(stdout);
				EPD_wait(1000);
    		}
			printf("                      \r");
			fflush(stdout);
			_gs ^= 1;
    	}
    	printf("\r\n-- Test %d\r\n", ftype);
    	EPD_DisplayClearPart();

    	//EPD_Cls(0);
		EPD_fillScreen(_bg);
		_fg = 15;
		_bg = 0;

		EPD_drawRect(1,1,200,200, EPD_BLACK);

		int y = 4;
		tstart = clock();
		if (ftype == 1) {
			for (int f=0; f<4; f++) {
				if (f == 0) _fg = 15;
				else if (f == 1) _fg = 9;
				else if (f == 2) _fg = 5;
				else if (f == 2) _fg = 3;
				EPD_setFont(f, NULL);
				if (f == 3) {
					EPD_print("Welcome to ", 4, y);
					font_rotate = 90;
					EPD_print("ESP32", EPD_getStringWidth("Welcome to ")+EPD_getfontheight()+4, y);
					font_rotate = 0;
				}
				else if (f == 1) {
					EPD_print("HR chars: \xA6\xA8\xB4\xB5\xB0", 4, y);
				}
				else {
					EPD_print("Welcome to ESP32", 4, y);
				}
				y += EPD_getfontheight() + 2;
			}
			font_rotate = 45;
			EPD_print("ESP32", LASTX+8, LASTY);
			font_rotate = 0;
			_fg = 15;

			EPD_setFont(DEFAULT_FONT, NULL);
			sprintf(tmp_buff, "Pass: %d", pass+1);
			EPD_print(tmp_buff, 4, 128-EPD_getfontheight()-2);
			EPD_UpdateScreen();
		}
		else if (ftype == 2) {
			orientation = LANDSCAPE_180;
			for (int f=4; f<FONT_7SEG; f++) {
				if (f == 4) _fg = 15;
				else if (f == 5) _fg = 9;
				else if (f == 6) _fg = 5;
				else if (f == 7) _fg = 3;
				EPD_setFont(f, NULL);
				EPD_print("Welcome to ESP32", 4, y);
				y += EPD_getfontheight() + 1;
			}
			_fg = 15;
			EPD_setFont(DEFAULT_FONT, NULL);
			sprintf(tmp_buff, "Pass: %d (rotated 180)", pass+1);
			EPD_print(tmp_buff, 4, 128-EPD_getfontheight()-2);
			orientation = LANDSCAPE_0;
			EPD_UpdateScreen();
		}
		else if (ftype == 3) {
			/*
			for (int f=0; f<3; f++) {
				if (f == 0) _fg = 15;
				else if (f == 1) _fg = 9;
				else if (f == 2) _fg = 5;
				EPD_setFont(USER_FONT, file_fonts[f]);
				if (f == 0) font_line_space = 4;
				EPD_print("Welcome to ESP32", 4, y);
				font_line_space = 0;
				y += EPD_getfontheight() + 2;
			}
			*/
			_fg = 15;
			EPD_setFont(DEFAULT_FONT, NULL);
			sprintf(tmp_buff, "Pass: %d (Fonts from file)", pass+1);
			EPD_print(tmp_buff, 4, 128-EPD_getfontheight()-2);
			EPD_UpdateScreen();
		}
		else if (ftype == 4) {
			y = 16;
			time(&time_now);
			tm_info = localtime(&time_now);
			int _sec = -1, y2, _day = -1;

			EPD_setFont(FONT_7SEG, NULL);
			set_7seg_font_atrib(10, 2, 0, 15);
			y2 = y + EPD_getfontheight() + 10;

			for (int t=0; t<100; t++) {
				time(&time_now);
				tm_info = localtime(&time_now);
				if (tm_info->tm_sec != _sec) {
					_sec = tm_info->tm_sec;
					sprintf(tmp_buff, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
					_fg = 15;							// fill = 15
					set_7seg_font_atrib(10, 2, 0, 15);	// outline = 15
					EPD_print(tmp_buff, CENTER, y);
					_fg = 15;
					if (tm_info->tm_mday != _day) {
						sprintf(tmp_buff, "%02d.%02d.%04d", tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year+1900);
						_fg = 7;							// fill = 7
						set_7seg_font_atrib(8, 2, 1, 15);	// outline = 15
						EPD_print(tmp_buff, CENTER, y2);
						_fg = 15;
					}
					EPD_UpdateScreen();
				}
				EPD_wait(100);
			}
			tstart = clock();
			_fg = 15;
			EPD_setFont(DEFAULT_FONT, NULL);
			font_rotate = 90;
			sprintf(tmp_buff, "%02d:%02d:%02d %02d/%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tm_info->tm_mday, tm_info->tm_mon + 1);
			EPD_print(tmp_buff, 20, 4);
			font_rotate = 0;
			sprintf(tmp_buff, "Pass: %d", pass+1);
			EPD_print(tmp_buff, 4, 128-EPD_getfontheight()-2);
			EPD_UpdateScreen();
		}
		else if (ftype == 5) {
			uint8_t old_gs = _gs;
			_gs = 1;
			EPD_drawRect(4,4,20,20, 15);

			EPD_fillRect(27,5,18,18, 1);
			EPD_drawRect(26,4,20,20, 15);

			EPD_drawCircle(66,16,10, 15);

			EPD_fillCircle(92,16,10, 2);
			EPD_drawCircle(92,16,11, 15);

			EPD_fillRect(185,4,80,80, 3);
			EPD_drawRect(185,4,80,80, 15);

			EPD_fillCircle(225,44,35, 0);
			EPD_drawCircle(225,44,35, 15);
			EPD_fillCircle(225,44,35, 5);

			EPD_fillCircle(225,44,20, 0);
			EPD_drawCircle(225,44,20, 15);

			orientation = LANDSCAPE_180;
			EPD_drawRect(4,4,20,20, 15);

			EPD_fillRect(27,5,18,18, 1);
			EPD_drawRect(26,4,20,20, 15);

			EPD_drawCircle(66,16,10, 15);

			EPD_fillCircle(92,16,10, 2);
			EPD_drawCircle(92,16,11, 15);

			EPD_fillRect(185,4,80,80, 3);
			EPD_drawRect(185,4,80,80, 15);

			EPD_fillCircle(225,44,35, 0);
			EPD_drawCircle(225,44,35, 15);
			EPD_fillCircle(225,44,35, 5);

			EPD_fillCircle(225,44,20, 0);
			EPD_drawCircle(225,44,20, 15);
			orientation = LANDSCAPE_0;

			EPD_setFont(DEFAULT_FONT, NULL);
			font_rotate = 90;
			sprintf(tmp_buff, "Pass: %d", pass+1);
			EPD_print("Gray scale demo", _width/2+EPD_getfontheight()+2, 4);
			EPD_print(tmp_buff, _width/2, 4);
			font_rotate = 0;

			EPD_UpdateScreen();
			_gs = old_gs;
		}
		else if (ftype == 6) {
			uint8_t old_gs = _gs;
			_gs = 0;
			memcpy(disp_buffer, (unsigned char *)gImage_img1, sizeof(gImage_img1));

			EPD_setFont(DEFAULT_FONT, NULL);
			sprintf(tmp_buff, "Pass: %d", pass+1);
			EPD_print(tmp_buff, 4, 128-EPD_getfontheight()-2);

			EPD_UpdateScreen();
			_gs = old_gs;
		}
		else if (ftype == 7) {
			uint8_t old_gs = _gs;
			_gs = 0;
			memcpy(disp_buffer, (unsigned char *)gImage_img3, sizeof(gImage_img3));

			EPD_setFont(DEFAULT_FONT, NULL);
			_fg = 0;
			_bg = 1;
			sprintf(tmp_buff, "Pass: %d", pass+1);
			EPD_print(tmp_buff, 4, 128-EPD_getfontheight()-2);

			EPD_UpdateScreen();
			_fg = 15;
			_bg = 0;
			_gs = old_gs;
		}
		else if (ftype == 8) {
			uint8_t old_gs = _gs;
			_gs = 1;
			int i, x, y;
	        uint8_t last_lvl = 0;
		    for (i=0; i<16; i++) {
		        for (x = 0; x < EPD_DISPLAY_WIDTH; x++) {
		          for (y = 0; y < EPD_DISPLAY_HEIGHT; y++) {
		        	  uint8_t pix = img_hacking[(x * EPD_DISPLAY_HEIGHT) + (EPD_DISPLAY_HEIGHT-y-1)];
		        	  if ((pix > last_lvl) && (pix <= lvl_buf[i])) {
		        		  gs_disp_buffer[(y * EPD_DISPLAY_WIDTH) + x] = i;
		        		  gs_used_shades |= (1 << i);
		        	  }
		          }
		        }
		        last_lvl = lvl_buf[i];
		    }

			EPD_setFont(DEFAULT_FONT, NULL);
			sprintf(tmp_buff, "Pass: %d (Gray scale image)", pass+1);
			EPD_print(tmp_buff, 4, 128-EPD_getfontheight()-2);

			EPD_UpdateScreen();
			_gs = old_gs;
		}
		else if (ftype == 9) {
			uint8_t old_gs = _gs;
			_gs = 0;
			memcpy(disp_buffer, gImage_img2, sizeof(gImage_img2));

			EPD_setFont(DEFAULT_FONT, NULL);
			sprintf(tmp_buff, "Pass: %d", pass+1);
			EPD_print(tmp_buff, 4, 4);

			EPD_UpdateScreen();
			_gs = old_gs;
		}
		else if (ftype == 10) {
#if 0
			if (spiffs_is_mounted) {
				// ** Show scaled (1/8, 1/4, 1/2 size) JPG images
					uint8_t old_gs = _gs;
					_gs = 1;
					EPD_Cls();
					EPD_jpg_image(CENTER, CENTER, 0, SPIFFS_BASE_PATH"/images/evolution-of-human.jpg", NULL, 0);
					EPD_UpdateScreen();
					EPD_wait(5000);

					EPD_Cls();
					EPD_jpg_image(CENTER, CENTER, 0, SPIFFS_BASE_PATH"/images/people_silhouettes.jpg", NULL, 0);
					EPD_UpdateScreen();
					EPD_wait(5000);

					EPD_Cls();
					EPD_jpg_image(CENTER, CENTER, 0, SPIFFS_BASE_PATH"/images/silhouettes-dancing.jpg", NULL, 0);
					EPD_UpdateScreen();
					EPD_wait(5000);

					EPD_Cls();
					EPD_jpg_image(CENTER, CENTER, 0, SPIFFS_BASE_PATH"/images/girl_silhouettes.jpg", NULL, 0);
					EPD_UpdateScreen();
					EPD_wait(5000);

					EPD_Cls();
					EPD_jpg_image(CENTER, CENTER, 0, SPIFFS_BASE_PATH"/images/animal-silhouettes.jpg", NULL, 0);
					EPD_UpdateScreen();
					EPD_wait(5000);

					EPD_Cls();
					EPD_jpg_image(CENTER, CENTER, 0, SPIFFS_BASE_PATH"/images/Flintstones.jpg", NULL, 0);
					EPD_UpdateScreen();
					EPD_wait(5000);

					_gs = old_gs;
			}
#endif
		}

		//EPD_DisplayPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, disp_buffer);
		tstart = clock() - tstart;
		pass++;
    	printf("-- Type: %d Pass: %d Time: %u ms\r\n", ftype, pass, tstart);

    	EPD_PowerOff();
		EPD_wait(8000);
    }

}
