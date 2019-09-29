#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_attr.h>
#include <sys/time.h>
//#include <esp_crosscore_int.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>


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


// ADC1_CHANNEL_6

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


#define MAX_TEST 10

uint32_t cycles[MAX_TEST];
uint32_t core[MAX_TEST];

bool g_my_app_init_done=false;
bool cpu1_scheduler_started=false;

#if 0
void IRAM_ATTR start_cpu1(void)
{
    int i;
    int pos=0;
    // Wait for the app to initialze on CPU0
    while (!g_my_app_init_done) {
        ;
    }
    // Wait for FreeRTOS initialization to finish on PRO CPU
    //while (port_xSchedulerRunning[0] == 0) {
    //    ;
    //}
    //Take care putting stuff here: if asked, FreeRTOS will happily tell you the scheduler
    //has started, but it isn't active *on this CPU* yet.
    esp_crosscore_int_init();

      if (xPortGetCoreID()==1) {
	printf("Running on APP CPU!!!!!!!!\n");
      }

    ESP_EARLY_LOGI(tag, "Starting scheduler on APP CPU.");
    cpu1_scheduler_started=true;
    // Scheduler never returns... 
    xPortStartScheduler();
}
#endif


typedef struct taskParam {
  char name[32];
  int id;
} taskParam;

static void test1(void *param) {
   taskParam* task_par=(taskParam *)param;
   char *id = task_par->name;
   int taskno=task_par->id;
   ESP_LOGD(tag, ">> %s", id);
   printf("%s started\n",id);
   int i;
   int times=2;
   //vTaskDelay(1 / portTICK_PERIOD_MS);
   while(times-->0) 
   {
      struct timeval start;
      gettimeofday(&start, NULL);
      int volatile j=0;
      core[taskno]=xPortGetCoreID();
      if (xPortGetCoreID()==1) {
	printf("Running on APP CPU!!!!!\n");
      } else {
	printf("Running on PRO CPU!!!!\n");
      }
      for (i=0; i<9000000; i++) {
         j=j+1;
      }
      printf( "%s - tick: %d\n", id, timeval_durationBeforeNow(&start));
      ESP_LOGD(tag, "%s - tick: %d", id, timeval_durationBeforeNow(&start));
      cycles[taskno]=get_ccount();
      vTaskDelay(6000 / portTICK_PERIOD_MS);
   }
   printf("%s finished\n",id);
   vTaskDelete(NULL);
}

taskParam param[10];

void init_param() {
  int i=0;
  for (i=0;i<10;i++) {
    char Buff[32];
    sprintf(Buff,"Task%d",i);
    param[i].id=i;
    strcpy(param[i].name,Buff);
  }
}

void task_tests(void *ignore) {
   init_param();
   xTaskCreate(&test1, "tk0", 4096, &param[0], 5, NULL);
   //xTaskCreatePinnedToCore(&test1, "tk0", 4096, &param[0], 5, NULL,1);
   xTaskCreate(&test1, "tk1", 4096, &param[1], 5, NULL);
   xTaskCreate(&test1, "tk2", 4096, &param[2], 5, NULL);
   xTaskCreate(&test1, "tk3", 4096, &param[3], 5, NULL);
   xTaskCreate(&test1, "tk4", 4096,&param[4], 5, NULL);
   xTaskCreate(&test1, "tk5", 4096,&param[5], 5, NULL);
   xTaskCreate(&test1, "tk6", 4096,&param[6], 5, NULL);
   xTaskCreate(&test1, "tk7", 4096,&param[7], 5, NULL);
   xTaskCreate(&test1, "tk8", 4096,&param[8], 5, NULL);
   xTaskCreate(&test1, "tk9", 4096,&param[9], 5, NULL);
   vTaskDelete(NULL);
}

void pinned_mixed_tests(void *ignore) {
   init_param();
   xTaskCreatePinnedToCore(&test1, "task0", 4096, &param[0], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task1", 4096, &param[1], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task2", 4096, &param[2], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task3", 4096, &param[3], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task4", 4096,&param[4], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task5", 4096,&param[5], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task6", 4096,&param[6], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task7", 4096,&param[7], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task8", 4096,&param[8], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task9", 4096,&param[9], 5, NULL,0);
   vTaskDelete(NULL);
}

void pinned_to_pro_tests(void *ignore) {
   init_param();
   xTaskCreatePinnedToCore(&test1, "task0", 4096, &param[0], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task1", 4096, &param[1], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task2", 4096, &param[2], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task3", 4096, &param[3], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task4", 4096,&param[4], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task5", 4096,&param[5], 5, NULL,0);
   vTaskDelete(NULL);
}

void app_main()
{
    int i=0;
    // flash uses ipc between cores.
    g_my_app_init_done=true;
    vTaskDelay(500 / portTICK_PERIOD_MS);
    //nvs_flash_init();

    
    printf("starting\n");

    //while (!cpu1_scheduler_started) {
    //  ;
    //}
    //printf("started\n");

    //xTaskCreate(&task_tests, "test", 4096, "test", 5, NULL);
    xTaskCreate(&pinned_mixed_tests, "test", 4096, "test", 5, NULL);
    //xTaskCreate(&pinned_to_pro_tests, "test", 4096, "test", 5, NULL);
    //pinned_to_pro_tests(NULL);

    for (i=0;i<MAX_TEST;i++) {
      printf("cycles %d %u %u\n",i,cycles[i],core[i]);
    }

    for(;;) {
      for (i=0;i<MAX_TEST;i++) {
	printf("cycles%d %u\t%u\n",i,cycles[i],core[i]);
      }
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}
