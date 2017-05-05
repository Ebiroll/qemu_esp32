/* PPPoS Client Example with GSM (tested with Telit GL865-DUAL-V3)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/semphr.h"

#include "driver/uart.h"

#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/pppapi.h"


#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include "apps/sntp/sntp.h"
#include "cJSON.h"


static uint8_t conn_ok = 0;
QueueHandle_t http_mutex;
#define HTTP_SEMAPHORE_WAIT 120000

/* The examples use simple GSM configuration that you can set via
   'make menuconfig'.
 */
#define BUF_SIZE (1024)
// *** If not using hw flow control, set it to 38400
#define UART_BDRATE 115200
#define GSM_OK_Str "OK"


const char *PPP_User = CONFIG_GSM_INTERNET_USER;
const char *PPP_Pass = CONFIG_GSM_INTERNET_PASSWORD;

// UART
#define UART_GPIO_TX 21
#define UART_GPIO_RX 32

static int uart_num = UART_NUM_1;

// The PPP control block
ppp_pcb *ppp;

// The PPP IP interface
struct netif ppp_netif;

static const char *TAG = "[PPPOS CLIENT]";
static const char *TIME_TAG = "[SNTP]";

typedef struct
{
	char *cmd;
	uint16_t cmdSize;
	char *cmdResponseOnOk;
	uint32_t timeoutMs;
	uint32_t delayMs;
}GSM_Cmd;

//--------------------------
GSM_Cmd GSM_MGR_InitCmds[] =
{
		{
				.cmd = "AT\r\n",
				.cmdSize = sizeof("AT\r\n")-1,
				.cmdResponseOnOk = GSM_OK_Str,
				.timeoutMs = 3000,
		},
		{
				.cmd = "ATZ\r\n",
				.cmdSize = sizeof("ATZ\r\n")-1,
				.cmdResponseOnOk = GSM_OK_Str,
				.timeoutMs = 3000,
		},
		{
				.cmd = "AT+CFUN=4\r\n",
				.cmdSize = sizeof("ATCFUN=4\r\n")-1,
				.cmdResponseOnOk = GSM_OK_Str,
				.timeoutMs = 3000,
		},
		{
				.cmd = "AT+CFUN=1\r\n",
				.cmdSize = sizeof("ATCFUN=4,0\r\n")-1,
				.cmdResponseOnOk = GSM_OK_Str,
				.timeoutMs = 3000,
		},
		{
				.cmd = "ATE0\r\n",
				.cmdSize = sizeof("ATE0\r\n")-1,
				.cmdResponseOnOk = GSM_OK_Str,
				.timeoutMs = 3000,
		},
		{
				.cmd = "AT+CPIN?\r\n",
				.cmdSize = sizeof("AT+CPIN?\r\n")-1,
				.cmdResponseOnOk = "CPIN: READY",
				.timeoutMs = 3000,
		},
		{
				.cmd = "AT+CREG?\r\n",
				.cmdSize = sizeof("AT+CREG?\r\n")-1,
				.cmdResponseOnOk = "CREG: 0,1",
				.timeoutMs = 3000,
		},
		{
				.cmd = "AT+CGDCONT=1,\"IP\",\"myapn\"\r",
				.cmdSize = sizeof("AT+CGDCONT=1,\"IP\",\"myapn\"\r")-1,
				.cmdResponseOnOk = GSM_OK_Str,
				.timeoutMs = 8000,
		},
		/*{
				.cmd = "ATDT*99***1#\r\n",
				.cmdSize = sizeof("ATDT*99***1#\r\n")-1,
				.cmdResponseOnOk = "CONNECT",
				.timeoutMs = 30000,
		}*/
		{
				.cmd = "AT+CGDATA=\"PPP\",1\r\n",
				.cmdSize = sizeof("AT+CGDATA=\"PPP\",1\r\n")-1,
				.cmdResponseOnOk = "CONNECT",
				.timeoutMs = 30000,
		}
};

#define GSM_MGR_InitCmdsSize  (sizeof(GSM_MGR_InitCmds)/sizeof(GSM_Cmd))


// PPP status callback
//----------------------------------------------------------------
static void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx) {
	struct netif *pppif = ppp_netif(pcb);
	LWIP_UNUSED_ARG(ctx);

	switch(err_code) {
	case PPPERR_NONE: {
		ESP_LOGI(TAG,"status_cb: Connected\n");
#if PPP_IPV4_SUPPORT
		ESP_LOGI(TAG,"   ipaddr    = %s\n", ipaddr_ntoa(&pppif->ip_addr));
		ESP_LOGI(TAG,"   gateway   = %s\n", ipaddr_ntoa(&pppif->gw));
		ESP_LOGI(TAG,"   netmask   = %s\n", ipaddr_ntoa(&pppif->netmask));
#endif

#if PPP_IPV6_SUPPORT
		ESP_LOGI(TAG,"   ip6addr   = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif
		conn_ok = 1;
		break;
	}
	case PPPERR_PARAM: {
		ESP_LOGE(TAG,"status_cb: Invalid parameter\n");
		break;
	}
	case PPPERR_OPEN: {
		ESP_LOGE(TAG,"status_cb: Unable to open PPP session\n");
		break;
	}
	case PPPERR_DEVICE: {
		ESP_LOGE(TAG,"status_cb: Invalid I/O device for PPP\n");
		break;
	}
	case PPPERR_ALLOC: {
		ESP_LOGE(TAG,"status_cb: Unable to allocate resources\n");
		break;
	}
	case PPPERR_USER: {
		ESP_LOGE(TAG,"status_cb: User interrupt\n");
		break;
	}
	case PPPERR_CONNECT: {
		ESP_LOGE(TAG,"status_cb: Connection lost\n");
		conn_ok = 0;
		break;
	}
	case PPPERR_AUTHFAIL: {
		ESP_LOGE(TAG,"status_cb: Failed authentication challenge\n");
		break;
	}
	case PPPERR_PROTOCOL: {
		ESP_LOGE(TAG,"status_cb: Failed to meet protocol\n");
		break;
	}
	case PPPERR_PEERDEAD: {
		ESP_LOGE(TAG,"status_cb: Connection timeout\n");
		break;
	}
	case PPPERR_IDLETIMEOUT: {
		ESP_LOGE(TAG,"status_cb: Idle Timeout\n");
		break;
	}
	case PPPERR_CONNECTTIME: {
		ESP_LOGE(TAG,"status_cb: Max connect time reached\n");
		break;
	}
	case PPPERR_LOOPBACK: {
		ESP_LOGE(TAG,"status_cb: Loopback detected\n");
		break;
	}
	default: {
		ESP_LOGE(TAG,"status_cb: Unknown error code %d\n", err_code);
		break;
	}
	}

	/*
	 * This should be in the switch case, this is put outside of the switch
	 * case for example readability.
	 */

	if (err_code == PPPERR_NONE) {
		return;
	}

	/* ppp_close() was previously called, don't reconnect */
	if (err_code == PPPERR_USER) {
		/* ppp_free(); -- can be called here */
		return;
	}
}

//--------------------------------------------------------------------------------
static u32_t ppp_output_callback(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx) {
	// *** Handle sending to GSM modem ***
	uint32_t ret = uart_write_bytes(uart_num, (const char*)data, len);
    uart_wait_tx_done(uart_num, 10 / portTICK_RATE_MS);
    return ret;
}

//-----------------------------
static void pppos_client_task()
{
	uint8_t init_ok = 1;
	int pass = 0;
	char sresp[256] = {'\0'};

	gpio_set_direction(21, GPIO_MODE_OUTPUT);
    gpio_set_direction(32, GPIO_MODE_INPUT);
    gpio_set_pull_mode(32, GPIO_PULLUP_ONLY);

    char* data = (char*) malloc(BUF_SIZE);
	char PPP_ApnATReq[sizeof(CONFIG_GSM_APN)+8];
	
	uart_config_t uart_config = {
			.baud_rate = UART_BDRATE,
			.data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	//Configure UART1 parameters
	uart_param_config(uart_num, &uart_config);
	//Set UART1 pins(TX, RX, RTS, CTS)
	uart_set_pin(uart_num, UART_GPIO_TX, UART_GPIO_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);

	// Set APN from config
	sprintf(PPP_ApnATReq, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", CONFIG_GSM_APN);
	GSM_MGR_InitCmds[7].cmd = PPP_ApnATReq;
	GSM_MGR_InitCmds[7].cmdSize = strlen(PPP_ApnATReq);

	ESP_LOGI(TAG,"Gsm init start");
	// *** Disconnect if connected ***
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	while (uart_read_bytes(uart_num, (uint8_t*)data, BUF_SIZE, 100 / portTICK_RATE_MS)) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
	uart_write_bytes(uart_num, "+++", 3);
    uart_wait_tx_done(uart_num, 10 / portTICK_RATE_MS);
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	conn_ok = 98;
	while(1)
	{
		init_ok = 1;
		// Init gsm
		int gsmCmdIter = 0;
		while(1)
		{
			// ** Send command to GSM
			memset(sresp, 0, 256);
			for (int i=0; i<255;i++) {
				if ((GSM_MGR_InitCmds[gsmCmdIter].cmd[i] >= 0x20) && (GSM_MGR_InitCmds[gsmCmdIter].cmd[i] < 0x80)) {
					sresp[i] = GSM_MGR_InitCmds[gsmCmdIter].cmd[i];
					sresp[i+1] = 0;
				}
				if (GSM_MGR_InitCmds[gsmCmdIter].cmd[i] == 0) break;
			}
			printf("[GSM INIT] >Cmd: [%s]\r\n", sresp);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			while (uart_read_bytes(uart_num, (uint8_t*)data, BUF_SIZE, 100 / portTICK_RATE_MS)) {
				vTaskDelay(100 / portTICK_PERIOD_MS);
			}
			uart_write_bytes(uart_num, (const char*)GSM_MGR_InitCmds[gsmCmdIter].cmd,
					GSM_MGR_InitCmds[gsmCmdIter].cmdSize);
            uart_wait_tx_done(uart_num, 10 / portTICK_RATE_MS);

            // ** Wait for and check the response
            int timeoutCnt = 0;
			memset(sresp, 0, 256);
			int idx = 0;
			int tot = 0;
			while(1)
			{
				memset(data, 0, BUF_SIZE);
				int len = 0;
				len = uart_read_bytes(uart_num, (uint8_t*)data, BUF_SIZE, 10 / portTICK_RATE_MS);
				if (len > 0) {
					for (int i=0; i<len;i++) {
						if (idx < 255) {
							if ((data[i] >= 0x20) && (data[i] < 0x80)) {
								sresp[idx++] = data[i];
							}
							else sresp[idx++] = 0x2e;
							sresp[idx] = 0;
						}
					}
					tot += len;
				}
				else {
					if (tot > 0) {
						printf("[GSM INIT] <Resp: [%s], %d\r\n", sresp, tot);
						if (strstr(sresp, GSM_MGR_InitCmds[gsmCmdIter].cmdResponseOnOk) != NULL) {
							break;
						}
						else {
							printf("           Wrong response, expected [%s]\r\n", GSM_MGR_InitCmds[gsmCmdIter].cmdResponseOnOk);
							init_ok = 0;
							break;
						}
					}
				}
				timeoutCnt += 10;

				if (timeoutCnt > GSM_MGR_InitCmds[gsmCmdIter].timeoutMs)
				{
					printf("[GSM INIT] No response, Gsm Init Error\r\n");
					init_ok = 0;
					break;
				}
			}
			if (init_ok == 0) {
				// No response or not as expected
				vTaskDelay(5000 / portTICK_PERIOD_MS);
				init_ok = 1;
				gsmCmdIter = 0;
				continue;
			}

			if (gsmCmdIter == 2) vTaskDelay(500 / portTICK_PERIOD_MS);
			if (gsmCmdIter == 3) vTaskDelay(1500 / portTICK_PERIOD_MS);
			gsmCmdIter++;
			if (gsmCmdIter >= GSM_MGR_InitCmdsSize) break; // All init commands sent
			if ((pass) && (gsmCmdIter == 2)) gsmCmdIter = 4;
			if (gsmCmdIter == 6) pass++;
		}

		ESP_LOGI(TAG,"Gsm init end");

		if (conn_ok == 98) {
			// After first successful initialization
			ppp = pppapi_pppos_create(&ppp_netif,
					ppp_output_callback, ppp_status_cb, NULL);

			ESP_LOGI(TAG,"After pppapi_pppos_create");

			if(ppp == NULL)	{
				ESP_LOGE(TAG, "Error init pppos");
				return;
			}
		}
		conn_ok = 99;
		pppapi_set_default(ppp);
		pppapi_set_auth(ppp, PPPAUTHTYPE_PAP, PPP_User, PPP_Pass);
		//pppapi_set_auth(ppp, PPPAUTHTYPE_NONE, PPP_User, PPP_Pass);
		pppapi_connect(ppp, 0);

		// *** Handle GSM modem responses ***
		while(1) {
			memset(data, 0, BUF_SIZE);
			int len = uart_read_bytes(uart_num, (uint8_t*)data, BUF_SIZE, 30 / portTICK_RATE_MS);
			if(len > 0)	{
				pppos_input_tcpip(ppp, (u8_t*)data, len);
			}
			// Check if disconnected
			if (conn_ok == 0) {
				ESP_LOGE(TAG, "Disconnected, trying again...");
				pppapi_close(ppp, 0);
				gsmCmdIter = 0;
				conn_ok = 89;
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				break;
			}
		}
	}
}

// ===============================================================================================
// ==== Http/Https get request ===================================================================
// ===============================================================================================

// Constants that aren't configurable in menuconfig
#define WEB_SERVER "loboris.eu"
#define WEB_PORT 80
#define WEB_URL "http://loboris.eu/ESP32/info.txt"
#define SSL_WEB_SERVER "www.howsmyssl.com"
#define SSL_WEB_PORT "443"
#define SSL_WEB_URL "https://www.howsmyssl.com/a/check"

static const char *HTTP_TAG = "[HTTP]";
static const char *HTTPS_TAG = "[HTTPS]";

static const char *REQUEST = "GET " WEB_URL " HTTP/1.1\n"
    "Host: "WEB_SERVER"\n"
    "User-Agent: esp-idf/1.0 esp32\n"
    "\n";

static const char *SSL_REQUEST = "GET " SSL_WEB_URL " HTTP/1.1\n"
    "Host: "SSL_WEB_SERVER"\n"
    "User-Agent: esp-idf/1.0 esp32\n"
    "\n";

/* Root cert for howsmyssl.com, taken from server_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");


//-----------------------------------
static void parse_object(cJSON *item)
{
	cJSON *subitem=item->child;
	while (subitem)
	{
		printf("%s = ", subitem->string);
		if (subitem->type == cJSON_String) printf("%s\r\n", subitem->valuestring);
		else if (subitem->type == cJSON_Number) printf("%d\r\n", subitem->valueint);
		else if (subitem->type == cJSON_False) printf("False\r\n");
		else if (subitem->type == cJSON_True) printf("True\r\n");
		else if (subitem->type == cJSON_NULL) printf("NULL\r\n");
		else if (subitem->type == cJSON_Object) printf("{Object}\r\n");
		else if (subitem->type == cJSON_Array) {
			int arr_count = cJSON_GetArraySize(subitem);
			printf("[Array] of %d items\r\n", arr_count);
			int n;
			for (n = 0; n < 3; n++) {
				// Get the JSON element and then get the values as before
				cJSON *arritem = cJSON_GetArrayItem(subitem, n);
				if ((arritem) && (arritem->valuestring)) printf("   %s\n", arritem->valuestring);
				else break;
			}
			if (arr_count > 3 ) printf("   + %d more...\r\n", arr_count-3);
		}
		else printf("[?]\r\n");
		// handle subitem
		if (subitem->child) parse_object(subitem->child);

		subitem=subitem->next;
	}
}

//--------------------------------------------
static void https_get_task(void *pvParameters)
{
	while (conn_ok != 1) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    char buf[512];
    char *buffer;
    int ret, flags, len, rlen=0, totlen=0;

	buffer = malloc(8192);
	if (!buffer) {
		ESP_LOGI(HTTPS_TAG, "*** ERROR allocating receive buffer ***");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}
	
	if (!(xSemaphoreTake(http_mutex, HTTP_SEMAPHORE_WAIT))) {
		ESP_LOGI(HTTPS_TAG, "*** ERROR: CANNOT GET MUTEX ***n");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

	mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    ESP_LOGI(HTTPS_TAG, "Seeding the random number generator");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    NULL, 0)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }

    ESP_LOGI(HTTPS_TAG, "Loading the CA root certificate...");

    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
                                 server_root_cert_pem_end-server_root_cert_pem_start);

    if(ret < 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        abort();
    }

    ESP_LOGI(HTTPS_TAG, "Setting hostname for TLS session...");

    // Hostname set here should match CN in server certificate
    if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }

    ESP_LOGI(HTTPS_TAG, "Setting up the SSL/TLS structure...");

    if((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    // MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
    //   a warning if CA verification fails but it will continue to connect.
    //   You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }
	xSemaphoreGive(http_mutex);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    while(1) {
		while (conn_ok != 1) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

		if (!(xSemaphoreTake(http_mutex, HTTP_SEMAPHORE_WAIT))) {
			ESP_LOGI(HTTPS_TAG, "===== ERROR: CANNOT GET MUTEX ===================================\n");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
			continue;
		}
		
        ESP_LOGI(HTTPS_TAG, "===== HTTPS GET REQUEST =========================================\n");

        mbedtls_net_init(&server_fd);

        ESP_LOGI(HTTPS_TAG, "Connecting to %s:%s...", SSL_WEB_SERVER, SSL_WEB_PORT);

        if ((ret = mbedtls_net_connect(&server_fd, SSL_WEB_SERVER,
                                      SSL_WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
        {
            ESP_LOGE(HTTPS_TAG, "mbedtls_net_connect returned -%x", -ret);
            goto exit;
        }

        ESP_LOGI(HTTPS_TAG, "Connected.");

        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        ESP_LOGI(HTTPS_TAG, "Performing the SSL/TLS handshake...");

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
                goto exit;
            }
        }

        ESP_LOGI(HTTPS_TAG, "Verifying peer X.509 certificate...");

        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
        {
            // In real life, we probably want to close connection if ret != 0
            ESP_LOGW(HTTPS_TAG, "Failed to verify peer certificate!");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            ESP_LOGW(HTTPS_TAG, "verification info: %s", buf);
        }
        else {
            ESP_LOGI(HTTPS_TAG, "Certificate verified.");
        }

        ESP_LOGI(HTTPS_TAG, "Writing HTTP request...");

        while((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)SSL_REQUEST, strlen(SSL_REQUEST))) <= 0)
        {
            if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_write returned -0x%x", -ret);
                goto exit;
            }
        }

        len = ret;
        ESP_LOGI(HTTPS_TAG, "%d bytes written", len);
        ESP_LOGI(HTTPS_TAG, "Reading HTTP response...");

		rlen = 0;
		totlen = 0;
        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, sizeof(buf));
            ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

            if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
                continue;

            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                ret = 0;
                break;
            }

            if(ret < 0)
            {
                ESP_LOGE(HTTPS_TAG, "mbedtls_ssl_read returned -0x%x", -ret);
                break;
            }

            if(ret == 0)
            {
                ESP_LOGI(HTTPS_TAG, "connection closed");
                break;
            }

            len = ret;
            //ESP_LOGI(HTTPS_TAG, "%d bytes read", len);
			totlen += len;
			if ((rlen + len) < 8192) {
				memcpy(buffer+rlen, buf, len);
				rlen += len;
			}
        } while(1);

        mbedtls_ssl_close_notify(&ssl);

    exit:
        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&server_fd);

        ESP_LOGI(HTTPS_TAG, "%d bytes read, %d in buffer", totlen, rlen);
        if(ret != 0)
        {
            mbedtls_strerror(ret, buf, 100);
            ESP_LOGE(HTTPS_TAG, "Last error was: -0x%x - %s", -ret, buf);
        }

        buffer[rlen] = '\0';
        char *json_ptr = strstr(buffer, "{\"given_cipher_suites\":");
        char *hdr_end_ptr = strstr(buffer, "\r\n\r\n");
		if (hdr_end_ptr) {
			*hdr_end_ptr = '\0';
			printf("Header:\r\n-------\r\n%s\r\n-------\r\n", buffer);
		}
		if (json_ptr) {
			ESP_LOGI(HTTPS_TAG, "JSON data received.");
			cJSON *root = cJSON_Parse(json_ptr);
			if (root) {
				ESP_LOGI(HTTPS_TAG, "parsing JSON data:");
				parse_object(root);
				cJSON_Delete(root);
			}
		}

        ESP_LOGI(HTTPS_TAG, "Waiting 30 sec...");
        ESP_LOGI(HTTPS_TAG, "=================================================================\n\n");
		xSemaphoreGive(http_mutex);
        for(int countdown = 30; countdown >= 0; countdown--) {
            //ESP_LOGI(HTTP_TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


//-------------------------------------------
static void http_get_task(void *pvParameters)
{
	while (conn_ok != 1) {
		vTaskDelay(100 / portTICK_RATE_MS);
	}

	const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[128];
    char *buffer;
    int rlen=0, totlen=0;

	buffer = malloc(2048);
	if (!buffer) {
		ESP_LOGI(HTTPS_TAG, "*** ERROR allocating receive buffer ***");
		while (1) {
            vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}

    while(1) {
		while (conn_ok != 1) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (!(xSemaphoreTake(http_mutex, HTTP_SEMAPHORE_WAIT))) {
			ESP_LOGI(HTTP_TAG, "===== ERROR: CANNOT GET MUTEX ==================================\n");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
			continue;
		}

		ESP_LOGI(HTTP_TAG, "===== HTTP GET REQUEST =========================================\n");

        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(HTTP_TAG, "DNS lookup failed err=%d res=%p", err, res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(HTTP_TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(HTTP_TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(HTTP_TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            xSemaphoreGive(http_mutex);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(HTTP_TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(HTTP_TAG, "... socket send failed");
            close(s);
            xSemaphoreGive(http_mutex);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(HTTP_TAG, "... socket send success");
        ESP_LOGI(HTTP_TAG, "... reading HTTP response...");

        /* Read HTTP response */
		int opt = 500;
		int first_block = 1;
		rlen = 0;
		totlen = 0;
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
			totlen += r;
			if ((rlen + r) < 2048) {
				memcpy(buffer+rlen, recv_buf, r);
				rlen += r;
			}
			if (first_block) {
				lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(int));
			}
        } while(r > 0);

        buffer[rlen] = '\0';
        char *hdr_end_ptr = strstr(buffer, "\r\n\r\n");
		if (hdr_end_ptr) {
			*hdr_end_ptr = '\0';
			printf("Header:\r\n-------\r\n%s\r\n-------\r\n", buffer);
			printf("Data:\r\n-----\r\n%s\r\n-----\r\n", hdr_end_ptr+4);
		}
        ESP_LOGI(HTTP_TAG, "... done reading from socket. %d bytes read, %d in buffer, errno=%d\r\n", totlen, rlen, errno);
        close(s);
        ESP_LOGI(HTTP_TAG, "Waiting 30 sec...");
        ESP_LOGI(HTTP_TAG, "================================================================\n\n");
		xSemaphoreGive(http_mutex);
        for(int countdown = 30; countdown >= 0; countdown--) {
            //ESP_LOGI(HTTP_TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


static void initialize_sntp(void)
{
    ESP_LOGI(TIME_TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void app_main()
{
	http_mutex = xSemaphoreCreateMutex();

	tcpip_adapter_init();
	xTaskCreate(&pppos_client_task, "pppos_client_task", 4096, NULL, 10, NULL);

	while (conn_ok != 1) {
		vTaskDelay(10 / portTICK_RATE_MS);
	}
	while (1) {
        ESP_LOGI(TIME_TAG,"OBTAINING TIME");
        initialize_sntp();
        ESP_LOGI(TIME_TAG,"SNTP INITIALIZED");

        // wait for time to be set
        time_t now = 0;
        struct tm timeinfo = { 0 };
        int retry = 0;
        const int retry_count = 10;
        while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
            ESP_LOGI(TIME_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            time(&now);
            localtime_r(&now, &timeinfo);
        	if (conn_ok != 1) break;
        }
    	if (conn_ok != 1) {
            sntp_stop();
    		while (conn_ok != 1) {
    			vTaskDelay(10 / portTICK_RATE_MS);
    		}
    		continue;
    	}

        if (retry < retry_count) {
            ESP_LOGI(TIME_TAG, "TIME SET TO %s", asctime(&timeinfo));
            break;
        }
        else {
            ESP_LOGI(TIME_TAG, "ERROR OBTAINING TIME\n");
        }
        sntp_stop();
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
	vTaskDelay(5000 / portTICK_RATE_MS);
    xTaskCreate(&https_get_task, "https_get_task", 16384, NULL, 3, NULL);

	while(1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}
