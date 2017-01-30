#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_attr.h>
#include <sys/time.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include <stdio.h>

//#include "c_timeutils.h"
#include "sdkconfig.h"

static char tag[] = "tasks";



//-----------------------------------------------------------------------------
// Read CCOUNT register.
//-----------------------------------------------------------------------------
static inline uint32_t get_ccount(void)
{
  uint32_t ccount;

  asm volatile ( "rsr     %0, ccount" : "=a" (ccount) );
  return ccount;
}


/**
 * Subtract one timeval from another.
 */
struct timeval timeval_sub(struct timeval *a, struct timeval *b) {
	struct timeval result;
	result.tv_sec = a->tv_sec - b->tv_sec;
	result.tv_usec = a->tv_usec - b->tv_usec;
	if (a->tv_usec < b->tv_usec) {
		result.tv_sec -= 1;
		result.tv_usec += 1000000;
	}
	return result;
} // timeval_sub



/**
 * Convert a timeval into the number of msecs.
 */
uint32_t timeval_toMsecs(struct timeval *a) {
	return a->tv_sec * 1000 + a->tv_usec/1000;
} // timeval_toMsecs



/**
 * Return the number of milliseconds the historic time value was before now.
 */
uint32_t timeval_durationBeforeNow(struct timeval *a) {
	struct timeval b;
	gettimeofday(&b, NULL);
	struct timeval delta = timeval_sub(&b, a);
	if (delta.tv_sec < 0) {
		return 0;
	}
	return timeval_toMsecs(&delta);
} // timeval_durationBeforeNow


#define MAX_TEST 32

uint32_t cycles[MAX_TEST];
uint32_t delta[MAX_TEST];

bool g_my_app_init_done=false;

#if 0

void IRAM_ATTR start_cpu1(void)
{
    int i;
    int pos=0;
    // Wait for the app to initialze on CPU0
    while (!g_my_app_init_done) {
        ;
    }
    // do things
    for (i=0;i<MAX_TEST;i++) {
      cycles[i]=0;
    }

    // Busy doing nothing
    while(1) {
      //struct timeval start;
      //gettimeofday(&start, NULL);
      int volatile j=0;
      for (i=0; i<9000000+pos*1000; i++) {
         j=j+1;
      }
      cycles[pos]=get_ccount();
      pos++;
      if (pos>=MAX_TEST) {
	pos=0;
      }
   }

}

#endif

int taskno=0;

static void test1(void *param) {
   char *id = (char *)param;
   ESP_LOGD(tag, ">> %s", id);
   int i;
   //while(1) 
   {
      struct timeval start;
      gettimeofday(&start, NULL);
      int volatile j=0;
      for (i=0; i<9000000; i++) {
         j=j+1;
      }
      printf( "%s - tick: %d\n", id, timeval_durationBeforeNow(&start));
      ESP_LOGD(tag, "%s - tick: %d", id, timeval_durationBeforeNow(&start));
   }
   cycles[taskno++]=get_ccount();
   vTaskDelay(10000 / portTICK_PERIOD_MS);
   vTaskDelete(NULL);
}

void task_tests(void *ignore) {
   xTaskCreate(&test1, "task1", 2048, "Task1", 5, NULL);
   xTaskCreate(&test1, "task2", 2048, "Task2", 5, NULL);
   xTaskCreate(&test1, "task3", 2048, "Task3", 5, NULL);
   xTaskCreate(&test1, "task4", 2048, "Task4", 5, NULL);
   xTaskCreate(&test1, "task5", 2048, "Task5", 5, NULL);
   xTaskCreate(&test1, "task6", 2048, "Task6", 5, NULL);
   vTaskDelete(NULL);
}


void app_main()
{
    int i=0;
    nvs_flash_init();
    g_my_app_init_done=true;
    
    printf("starting\n");

    xTaskCreate(&task_tests, "test", 2048, "test", 5, NULL);
    //task_tests(NULL);

    for (i=0;i<MAX_TEST;i++) {
      printf("cycles %d %u\n",i,cycles[i]);
    }

    for(;;) {
      for (i=1;i<MAX_TEST;i++) {
	delta[i]=cycles[i]-cycles[i-1];
	printf("cycles %d %u %u\t%u\n",i,cycles[i],delta[i],delta[i]-delta[i-1]);
      }
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}
