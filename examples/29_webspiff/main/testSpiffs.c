#if 0

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#ifdef CONFIG_EXAMPLE_USE_WIFI

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_attr.h"
#include <time.h>
#include <sys/time.h>
#include "lwip/err.h"
#include "apps/sntp/sntp.h"

#endif


#include <errno.h>
#include <sys/fcntl.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "spiffs_vfs.h"


static const char tag[] = "[SPIFFS example]";

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

//---------------------------
static void obtain_time(void)
{
    initialise_wifi();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(tag, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    if (timeinfo.tm_year < (2016 - 1900)) {
    	ESP_LOGI(tag, "System time NOT set.");
    }
    else {
    	ESP_LOGI(tag, "System time is set.");
    }

    ESP_ERROR_CHECK( esp_wifi_stop() );
}

#endif


// ============================================================================
#include <ctype.h>

// fnmatch defines
#define	FNM_NOMATCH	1	// Match failed.
#define	FNM_NOESCAPE	0x01	// Disable backslash escaping.
#define	FNM_PATHNAME	0x02	// Slash must be matched by slash.
#define	FNM_PERIOD		0x04	// Period must be matched by period.
#define	FNM_LEADING_DIR	0x08	// Ignore /<tail> after Imatch.
#define	FNM_CASEFOLD	0x10	// Case insensitive search.
#define FNM_PREFIX_DIRS	0x20	// Directory prefixes of pattern match too.
#define	EOS	        '\0'

//-----------------------------------------------------------------------
static const char * rangematch(const char *pattern, char test, int flags)
{
  int negate, ok;
  char c, c2;

  /*
   * A bracket expression starting with an unquoted circumflex
   * character produces unspecified results (IEEE 1003.2-1992,
   * 3.13.2).  This implementation treats it like '!', for
   * consistency with the regular expression syntax.
   * J.T. Conklin (conklin@ngai.kaleida.com)
   */
  if ( (negate = (*pattern == '!' || *pattern == '^')) ) ++pattern;

  if (flags & FNM_CASEFOLD) test = tolower((unsigned char)test);

  for (ok = 0; (c = *pattern++) != ']';) {
    if (c == '\\' && !(flags & FNM_NOESCAPE)) c = *pattern++;
    if (c == EOS) return (NULL);

    if (flags & FNM_CASEFOLD) c = tolower((unsigned char)c);

    if (*pattern == '-' && (c2 = *(pattern+1)) != EOS && c2 != ']') {
      pattern += 2;
      if (c2 == '\\' && !(flags & FNM_NOESCAPE)) c2 = *pattern++;
      if (c2 == EOS) return (NULL);

      if (flags & FNM_CASEFOLD) c2 = tolower((unsigned char)c2);

      if ((unsigned char)c <= (unsigned char)test &&
          (unsigned char)test <= (unsigned char)c2) ok = 1;
    }
    else if (c == test) ok = 1;
  }
  return (ok == negate ? NULL : pattern);
}

//--------------------------------------------------------------------
static int fnmatch(const char *pattern, const char *string, int flags)
{
  const char *stringstart;
  char c, test;

  for (stringstart = string;;)
    switch (c = *pattern++) {
    case EOS:
      if ((flags & FNM_LEADING_DIR) && *string == '/') return (0);
      return (*string == EOS ? 0 : FNM_NOMATCH);
    case '?':
      if (*string == EOS) return (FNM_NOMATCH);
      if (*string == '/' && (flags & FNM_PATHNAME)) return (FNM_NOMATCH);
      if (*string == '.' && (flags & FNM_PERIOD) &&
          (string == stringstart ||
          ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
              return (FNM_NOMATCH);
      ++string;
      break;
    case '*':
      c = *pattern;
      // Collapse multiple stars.
      while (c == '*') c = *++pattern;

      if (*string == '.' && (flags & FNM_PERIOD) &&
          (string == stringstart ||
          ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
              return (FNM_NOMATCH);

      // Optimize for pattern with * at end or before /.
      if (c == EOS)
        if (flags & FNM_PATHNAME)
          return ((flags & FNM_LEADING_DIR) ||
                    strchr(string, '/') == NULL ?
                    0 : FNM_NOMATCH);
        else return (0);
      else if ((c == '/') && (flags & FNM_PATHNAME)) {
        if ((string = strchr(string, '/')) == NULL) return (FNM_NOMATCH);
        break;
      }

      // General case, use recursion.
      while ((test = *string) != EOS) {
        if (!fnmatch(pattern, string, flags & ~FNM_PERIOD)) return (0);
        if ((test == '/') && (flags & FNM_PATHNAME)) break;
        ++string;
      }
      return (FNM_NOMATCH);
    case '[':
      if (*string == EOS) return (FNM_NOMATCH);
      if ((*string == '/') && (flags & FNM_PATHNAME)) return (FNM_NOMATCH);
      if ((pattern = rangematch(pattern, *string, flags)) == NULL) return (FNM_NOMATCH);
      ++string;
      break;
    case '\\':
      if (!(flags & FNM_NOESCAPE)) {
        if ((c = *pattern++) == EOS) {
          c = '\\';
          --pattern;
        }
      }
      break;
      // FALLTHROUGH
    default:
      if (c == *string) {
      }
      else if ((flags & FNM_CASEFOLD) && (tolower((unsigned char)c) == tolower((unsigned char)*string))) {
      }
      else if ((flags & FNM_PREFIX_DIRS) && *string == EOS && ((c == '/' && string != stringstart) ||
    		  (string == stringstart+1 && *stringstart == '/')))
              return (0);
      else return (FNM_NOMATCH);
      string++;
      break;
    }
  // NOTREACHED
  return 0;
}

// ============================================================================

//-----------------------------------------
static void list(char *path, char *match) {

    DIR *dir = NULL;
    struct dirent *ent;
    char type;
    char size[9];
    char tpath[255];
    char tbuffer[80];
    struct stat sb;
    struct tm *tm_info;
    char *lpath = NULL;
    int statok;

    printf("LIST of DIR [%s]\r\n", path);
    // Open directory
    dir = opendir(path);
    if (!dir) {
        printf("Error opening directory\r\n");
        return;
    }

    // Read directory entries
    uint64_t total = 0;
    int nfiles = 0;
    printf("T  Size      Date/Time         Name\r\n");
    printf("-----------------------------------\r\n");
    while ((ent = readdir(dir)) != NULL) {
    	sprintf(tpath, path);
        if (path[strlen(path)-1] != '/') strcat(tpath,"/");
        strcat(tpath,ent->d_name);
        tbuffer[0] = '\0';

        if ((match == NULL) || (fnmatch(match, tpath, (FNM_PERIOD)) == 0)) {
			// Get file stat
			statok = stat(tpath, &sb);

			if (statok == 0) {
				tm_info = localtime(&sb.st_mtime);
				strftime(tbuffer, 80, "%d/%m/%Y %R", tm_info);
			}
			else sprintf(tbuffer, "                ");

			if (ent->d_type == DT_REG) {
				type = 'f';
				nfiles++;
				if (statok) strcpy(size, "       ?");
				else {
					total += sb.st_size;
					if (sb.st_size < (1024*1024)) sprintf(size,"%8d", (int)sb.st_size);
					else if ((sb.st_size/1024) < (1024*1024)) sprintf(size,"%6dKB", (int)(sb.st_size / 1024));
					else sprintf(size,"%6dMB", (int)(sb.st_size / (1024 * 1024)));
				}
			}
			else {
				type = 'd';
				strcpy(size, "       -");
			}

			printf("%c  %s  %s  %s\r\n",
				type,
				size,
				tbuffer,
				ent->d_name
			);
        }
    }
    if (total) {
        printf("-----------------------------------\r\n");
    	if (total < (1024*1024)) printf("   %8d", (int)total);
    	else if ((total/1024) < (1024*1024)) printf("   %6dKB", (int)(total / 1024));
    	else printf("   %6dMB", (int)(total / (1024 * 1024)));
    	printf(" in %d file(s)\r\n", nfiles);
    }
    printf("-----------------------------------\r\n");

    closedir(dir);

    free(lpath);

	uint32_t tot, used;
	spiffs_fs_stat(&tot, &used);
	printf("SPIFFS: free %d KB of %d KB\r\n", (tot-used) / 1024, tot / 1024);
}

//----------------------------------------------------
static int file_copy(const char *to, const char *from)
{
    FILE *fd_to;
    FILE *fd_from;
    char buf[1024];
    ssize_t nread;
    int saved_errno;

    fd_from = fopen(from, "rb");
    //fd_from = open(from, O_RDONLY);
    if (fd_from == NULL) return -1;

    fd_to = fopen(to, "wb");
    if (fd_to == NULL) goto out_error;

    while (nread = fread(buf, 1, sizeof(buf), fd_from), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = fwrite(out_ptr, 1, nread, fd_to);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR) goto out_error;
        } while (nread > 0);
    }

    if (nread == 0) {
        if (fclose(fd_to) < 0) {
            fd_to = NULL;
            goto out_error;
        }
        fclose(fd_from);

        // Success!
        return 0;
    }

  out_error:
    saved_errno = errno;

    fclose(fd_from);
    if (fd_to) fclose(fd_to);

    errno = saved_errno;
    return -1;
}

//--------------------------------
static void writeTest(char *fname)
{
	printf("==== Write to file \"%s\" ====\r\n", fname);

	int n, res, tot, len;
	char buf[40];

	FILE *fd = fopen(fname, "wb");
    if (fd == NULL) {
    	printf("     Error opening file\r\n");
    	return;
    }
    tot = 0;
    for (n = 1; n < 11; n++) {
    	sprintf(buf, "ESP32 spiffs write to file, line %d\r\n", n);
    	len = strlen(buf);
		res = fwrite(buf, 1, len, fd);
		if (res != len) {
	    	printf("     Error writing to file(%d <> %d\r\n", res, len);
	    	break;
		}
		tot += res;
    }
	printf("     %d bytes written\r\n", tot);
	res = fclose(fd);
	if (res) {
    	printf("     Error closing file\r\n");
	}
    printf("\r\n");
}

//-------------------------------
static void readTest(char *fname)
{
	printf("==== Reading from file \"%s\" ====\r\n", fname);

	int res;
	char *buf;
	buf = calloc(1024, 1);
	if (buf == NULL) {
    	printf("     Error allocating read buffer\"\r\n");
    	return;
	}

	FILE *fd = fopen(fname, "rb");
    if (fd == NULL) {
    	printf("     Error opening file\r\n");
    	free(buf);
    	return;
    }
    res = 999;
    res = fread(buf, 1, 1023, fd);
    if (res <= 0) {
    	printf("     Error reading from file\r\n");
    }
    else {
    	printf("     %d bytes read [\r\n", res);
        buf[res] = '\0';
        printf("%s\r\n]\r\n", buf);
    }
	free(buf);

	res = fclose(fd);
	if (res) {
    	printf("     Error closing file\r\n");
	}
    printf("\r\n");
}

//----------------------------------
static void mkdirTest(char *dirname)
{
	printf("==== Make new directory \"%s\" ====\r\n", dirname);

	int res;
	struct stat st = {0};
	char nname[80];

	if (stat(dirname, &st) == -1) {
	    res = mkdir(dirname, 0777);
	    if (res != 0) {
	    	printf("     Error creating directory (%d)\r\n", res);
	        printf("\r\n");
	        return;
	    }
    	printf("     Directory created\r\n\r\n");
		list("/spiffs/", NULL);
		vTaskDelay(1000 / portTICK_RATE_MS);

    	printf("     Copy file from root to new directory...\r\n");
    	sprintf(nname, "%s/test.txt.copy", dirname);
    	res = file_copy(nname, "/spiffs/test.txt");
	    if (res != 0) {
	    	printf("     Error copying file (%d)\r\n", res);
	    }
    	printf("\r\n");
    	list(dirname, NULL);
		vTaskDelay(1000 / portTICK_RATE_MS);

    	printf("     Removing file from new directory...\r\n");
	    res = remove(nname);
	    if (res != 0) {
	    	printf("     Error removing directory (%d)\r\n", res);
	    }
    	printf("\r\n");
    	list(dirname, NULL);
		vTaskDelay(1000 / portTICK_RATE_MS);

    	printf("     Removing directory...\r\n");
	    res = remove(dirname);
	    if (res != 0) {
	    	printf("     Error removing directory (%d)\r\n", res);
	    }
    	printf("\r\n");
		list("/spiffs/", NULL);
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
	else {
		printf("     Directory already exists, removing\r\n");
	    res = remove(dirname);
	    if (res != 0) {
	    	printf("     Error removing directory (%d)\r\n", res);
	    }
	}

    printf("\r\n");
}

//================
int app_main(void)
{
    
#ifdef CONFIG_EXAMPLE_USE_WIFI
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(tag, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
#endif

#if 0
   int test_buff[4];

   if (spi_flash_read(0x1800fc, (void *)test_buff, 4) != 0) { 
         printf("Failed spi_flash_read\r\n\n");
   }

   printf("%08X\n\n",(unsigned int)test_buff[0]);
#endif

    printf("\r\n\n");
    ESP_LOGI(tag, "==== STARTING SPIFFS TEST ====\r\n");

    vfs_spiffs_register();
    printf("\r\n\n");

   	if (spiffs_is_mounted) {
		vTaskDelay(2000 / portTICK_RATE_MS);

		writeTest("/spiffs/test.txt");
		readTest("/spiffs/test.txt");
		readTest("/spiffs/spiffs.info");

		list("/spiffs/", NULL);
	    printf("\r\n");

		mkdirTest("/spiffs/newdir");

		printf("==== List content of the directory \"images\" ====\r\n\r\n");
        list("/spiffs/js", NULL);
	    printf("\r\n");
    }

    while (1) {
		vTaskDelay(1000 / portTICK_RATE_MS);
    }
    return 0;
}

#endif