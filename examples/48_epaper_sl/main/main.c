/* SL epaper

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#if 1

#include <time.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
//#include "spiffs_vfs.h"
//
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_attr.h"
#include <sys/time.h>
#include <unistd.h>
#include "lwip/err.h"
//#include "apps/sntp/sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "json.h"
#include "spi_master_lobo.h"
#include "EPD.h"
#include "esp_sntp.h"

extern color_t	_fg;
extern color_t _bg;


static const char tag[] = "[epaper-sl]";
static time_t time_now, time_last = 0;
static struct tm* tm_info;
static char tmp_buff[96];

int tempy ;

void resetPos() {

}


static void _dispTime()
{
	Font_t curr_font = cfont;
    if (_width < 240) EPD_setFont(SMALL_FONT, NULL);
	else EPD_setFont(DEFAULT_FONT, NULL);

    time(&time_now);
	time_last = time_now;
	tm_info = localtime(&time_now);
	sprintf(tmp_buff, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
	EPD_print(tmp_buff, CENTER, _height-EPD_getfontheight()-5);

    cfont = curr_font;
}

//---------------------------------
static void disp_header(char *info)
{
	EPD_DisplayClearPart();

	//EPD_Cls(0);
	EPD_fillScreen(_bg);
	_fg = 15;
	_bg = 0;


    //if (_width < 240) EPD_setFont(SMALL_FONT, NULL);
	//else EPD_setFont(DEFAULT_FONT, NULL);
	//EPD_setFont(DEJAVU18_FONT,NULL);
	//EPD_fillRect(0, 0, _width-1, EPD_getfontheight()+8,_fg);
	//EPD_drawRect(0, 0, _width-1, EPD_getfontheight()+8,_fg);

	//EPD_fillRect(0, _height-EPD_getfontheight()-9, _width-1, EPD_getfontheight()+8,_bg);
	//EPD_drawRect(0, _height-EPD_getfontheight()-9, _width-1, EPD_getfontheight()+8,_bg);

	//EPD_print(info, CENTER, 4);
	_dispTime();

	//EPD_setclipwin(0,EPD_getfontheight()+9, _width-1, _height-EPD_getfontheight()-10);
}


void drawBusHeader() {
	//_EPD_setRotation(PORTRAIT);
	//EPD_pushColorRep(0, 0, _width-1, _height-1, (color_t){0,0,0}, (uint32_t)(_height*_width));

	//EPD_setFont(UBUNTU16_FONT, NULL);
	tempy = 8;  // EPD_getfontheight() +
	//EPD_print("JAKOBSBERG", CENTER,  tempy); // (dispWin.y2-dispWin.y1)/2
	//EPD_setFont(UBUNTU16_FONT, NULL);
}


void drawBusInfo(const char *info,bool isTime) {
	EPD_setFont(UBUNTU16_FONT, NULL);
	if (!isTime) {
	   printf("%s,%d,%d",info, 40, tempy);
  	   EPD_print(info, 40, tempy);
	} else {
  	   EPD_print(info, 150, tempy);
	   printf("%s,%d,%d",info, 40, tempy);
  	   tempy += EPD_getfontheight() + 4;
	}
}

void train_parse(json_stream *json) {
	bool is_train_to_sumpan=false;
	  while ( json_peek(json) != JSON_ARRAY_END && !json_get_error(json)) {
      enum json_type type = json_next(json);
      switch (type) {
		case JSON_STRING:
		// JourneyDirection== 1
			if (strcmp(json_get_string(json, NULL),"JourneyDirection")==0) {
				type = json_next(json);
				if (strcmp(json_get_string(json, NULL),"1")==0) {
					is_train_to_sumpan=true;
				}
			    if (is_train_to_sumpan) {
				   drawBusInfo("J",false);
			    }
			    printf("\"%s\"\t", json_get_string(json, NULL));
			}
			if (strcmp(json_get_string(json, NULL),"DisplayTime")==0) {
			    type = json_next(json);
			    if (is_train_to_sumpan) {
				 drawBusInfo(json_get_string(json, NULL),true);
				 is_train_to_sumpan=false;
			} 	
			
			printf("\"%s\" \n", json_get_string(json, NULL));
			}
	    break;
		default:
		break;
      }
   }
}

void bus_parse(json_stream *json) {
   drawBusHeader();
   bool is_bus_to_viksjo=false;
   while ( json_peek(json) != JSON_ARRAY_END && !json_get_error(json)) {
      enum json_type type = json_next(json);
      switch (type) {
        case JSON_STRING:
			if (strcmp(json_get_string(json, NULL),"LineNumber")==0) {
				type = json_next(json);
				if (strcmp(json_get_string(json, NULL),"591")==0) {
					is_bus_to_viksjo=true;
				}
				if (strcmp(json_get_string(json, NULL),"553V")==0) {
					is_bus_to_viksjo=true;
				}
				if (strcmp(json_get_string(json, NULL),"553H")==0) {
					is_bus_to_viksjo=true;
				}
				if (strcmp(json_get_string(json, NULL),"553")==0) {
					is_bus_to_viksjo=true;
				}

				if (strcmp(json_get_string(json, NULL),"551V")==0) {
					is_bus_to_viksjo=true;
				}
				if (strcmp(json_get_string(json, NULL),"551H")==0) {
					is_bus_to_viksjo=true;
				}
				if (strcmp(json_get_string(json, NULL),"551")==0) {
					is_bus_to_viksjo=true;
				}

				if (strcmp(json_get_string(json, NULL),"552")==0) {
					is_bus_to_viksjo=true;
				}
				is_bus_to_viksjo=true;

			    if (is_bus_to_viksjo) {
				   drawBusInfo(json_get_string(json, NULL),false);
			    }
			    printf("\"%s\"\t", json_get_string(json, NULL));
			}
			if (strcmp(json_get_string(json, NULL),"DisplayTime")==0) {
			    type = json_next(json);
			    if (is_bus_to_viksjo) {
				 	drawBusInfo(json_get_string(json, NULL),true);
				 	is_bus_to_viksjo=false;
				} 	
			
			printf("\"%s\" \n", json_get_string(json, NULL));
			}
	    break;
		default:
		break;
      }
   }
}




void test_parse(json_stream *json)
{
  // ResponseData
  // json_peek(json) != JSON_OBJECT_END
   while ( json_peek(json) != JSON_DONE && !json_get_error(json)) {
      enum json_type type = json_next(json);
		switch (type) {
			case JSON_STRING:
				if (json_peek(json)==JSON_ARRAY) {
					//if (strcmp(json_get_string(json, NULL),"Buses")==0) {
					//  bus_parse(json);
					//}
					//printf("\"%s\"\n", json_get_string(json, NULL));
				}
				if (json_peek(json)==JSON_ARRAY) {
					if (strcmp(json_get_string(json, NULL),"Trains")==0) {
				       train_parse(json);
					}
				}
			break;
		default:
			break;
		}
    }
	EPD_UpdateScreen();
}






/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "api.sl.se"
#define WEB_PORT 80
#define WEB_URL "http://api.sl.se/api2/realtimedeparturesV4.json?key=KEY&siteid=9702&timewindow=50"


//static const char *TAG = "example";

static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";


static int _demo_pass = 0;

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


static void initialize_sntp(void)
{
    ESP_LOGI(tag, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
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
            .ssid = "ssid" , //CONFIG_WIFI_SSID,
            .password = "password"  //CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_LOGI(tag, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}


static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

	char *reply=malloc(8*4096);
	int num_received=0;

    while(1) {
        /* Wait for the callback to set the CONNECTED in the
           event group.
		*/
		ESP_LOGI(tag, "Waiting for connect to AP");		

        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(tag, "Connected to AP");



        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(tag, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(tag, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(tag, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(tag, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(tag, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(tag, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(tag, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(tag, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(tag, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(tag, "... set socket receiving timeout success");

        /* Read HTTP response */
		bool started_json;
		started_json=false;
		num_received=0;
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);

			//char *reply=malloc(4096);
			//int num_received=0;

            for(int i = 0; i < r; i++) {

				if (started_json==false) {
					if (recv_buf[i]=='{') {
						started_json=true;
					}
				}
				if (started_json) {
					reply[num_received]=recv_buf[i];
					num_received++;
				}
                //putchar(recv_buf[i]);
            }
        } while(r > 0);
		reply[num_received]=0;

		//EPD_DisplayClearPart();
	    //EPD_DisplayClearFull();

		//EPD_Cls(0);
		//EPD_fillScreen(_bg);
		disp_header("Jakobsberg");

        json_stream json;
        json_open_string(&json, reply);
    	json_set_streaming(&json, false);
    	test_parse(&json);

    	EPD_PowerOff();
		EPD_wait(8000);

        ESP_LOGI(tag, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(tag, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(tag, "Starting again!");
    }
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
		sprintf(tmp_buff, "Wait %0d/%d", retry, retry_count);
    	EPD_print(tmp_buff, CENTER, LASTY);
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

	// Dont stop wifi yet
    //ESP_ERROR_CHECK( esp_wifi_stop() );
    return res;
}

//==================================================================================


//----------------------
static void _checkTime()
{
	time(&time_now);
	if (time_now > time_last) {
		color_t last_fg, last_bg;
		time_last = time_now;
		tm_info = localtime(&time_now);
		sprintf(tmp_buff, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

		//EPD_saveClipWin();
		//EPD_resetclipwin();

		Font_t curr_font = cfont;
		last_bg = _bg;
		last_fg = _fg;
		EPD_setFont(DEFAULT_FONT, NULL);

		EPD_fillRect(1, _height-EPD_getfontheight()-8, _width-3, EPD_getfontheight()+6, _bg);
		EPD_print(tmp_buff, CENTER, _height-EPD_getfontheight()-5);

		cfont = curr_font;

		//EPD_restoreClipWin();
	}
}


//---------------------
static int Wait(int ms)
{
	uint8_t tm = 1;
	if (ms < 0) {
		tm = 0;
		ms *= -1;
	}
	if (ms <= 50) {
		vTaskDelay(ms / portTICK_RATE_MS);
		//if (_checkTouch()) return 0;
	}
	else {
		for (int n=0; n<ms; n += 50) {
			vTaskDelay(50 / portTICK_RATE_MS);
			if (tm) _checkTime();
			//if (_checkTouch()) return 0;
		}
	}
	return 1;
}


//---------------------


//---------------------------------------------
static void update_header(char *hdr, char *ftr)
{
	color_t last_fg, last_bg;

	Font_t curr_font = cfont;
    if (_width < 240) EPD_setFont(SMALL_FONT, NULL);
	else EPD_setFont(DEFAULT_FONT, NULL);

	if (hdr) {
		EPD_fillRect(1, 1, _width-3, EPD_getfontheight()+6, _bg);
		EPD_print(hdr, CENTER, 4);
	}

	if (ftr) {
		EPD_fillRect(1, _height-EPD_getfontheight()-8, _width-3, EPD_getfontheight()+6, _bg);
		if (strlen(ftr) == 0) _dispTime();
		else EPD_print(ftr, CENTER, _height-EPD_getfontheight()-5);
	}

	cfont = curr_font;

}


void paper_demo_main();


//=============
void app_main()
{

    _fg = EPD_BLACK; 
    _bg = EPD_WHITE;

    ESP_ERROR_CHECK( nvs_flash_init() );

	//paper_demo_main();

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


	//EPD_DisplayClearFull();



    // ===== Set time zone ======
	setenv("TZ", "CET-1CEST", 0);
	tzset();
	// ==========================

	//disp_header("GET NTP TIME");
	printf("------- obtain_time() -----\r\n");	
	obtain_time();

    time(&time_now);
	tm_info = localtime(&time_now);
	EPD_DisplayClearPart();


	// Is time set? If not, tm_year will be (1970 - 1900).
    if (tm_info->tm_year < (2016 - 1900)) {
        ESP_LOGI(tag, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    	EPD_print("Time is not set yet", CENTER, CENTER);
    	EPD_print("Connecting to WiFi", CENTER, LASTY+EPD_getfontheight()+2);
    	EPD_print("Getting time over NTP", CENTER, LASTY+EPD_getfontheight()+2);
		EPD_print("Wait", CENTER, LASTY+EPD_getfontheight()+2);

        time(&time_now);
    	update_header(NULL, "");
		EPD_UpdateScreen();

		EPD_PowerOff();
 	    //EPD_wait(2000);

    	//Wait(-2000);
    }

  	//EPD_setRotation(disp_rot);
	disp_header("ESP32 SL");

	EPD_UpdateScreen();
 	//EPD_wait(1000);

	//EPD_Cls(0);
	EPD_fillScreen(_bg);
	_fg = 15;
	_bg = 0;

	int y = 16;
	time(&time_now);
	tm_info = localtime(&time_now);
	int _sec = -1, y2, _day = -1;

	EPD_setFont(FONT_7SEG, NULL);
	set_7seg_font_atrib(10, 2, 0, 15);
	y2 = y + EPD_getfontheight() + 10;

	for (int t=0; t<4; t++) {
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
		EPD_wait(1000);
	}


	EPD_PowerOff();
	//EPD_wait(8000);



	//xTaskCreate(&http_get_task, "http_get_task", 2*4096, NULL, 5, NULL);

	http_get_task(NULL);

	//=========
    // Run demo
	//=========
	//while(true) {
	//	Wait(2000);
	//}
}

#endif
