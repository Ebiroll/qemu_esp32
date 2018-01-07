#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_attr.h>
#include <sys/time.h>
#include "esp_system.h"
#include <esp_crosscore_int.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include "rom/uart.h"
#include "soc/dport_reg.h" 
#include "soc/cpu.h" 
#include "rom/cache.h" 
#include "esp_newlib.h"
#include "driver/rtc_io.h"
#include "esp_task.h"
#include "esp_vfs_dev.h" 
#include "esp_spi_flash.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"
//#include "cache_err_int.h"
#include "soc/timer_group_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_ipc.h"
#include "esp_heap_alloc_caps.h"


//extern void heap_alloc_caps_init();
extern void esp_cache_err_int_init();
extern volatile int port_xSchedulerRunning[2];

#define STRINGIFY(a) #a

//#include "c_timeutils.h"
#include "sdkconfig.h"

static char tag[] = "tasks";

extern int _init_start;
extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);
extern void app_main(void);

static bool app_cpu_started = false;

//-----------------------------------------------------------------------------
// Read CCOUNT register.
//-----------------------------------------------------------------------------
static inline uint32_t  get_ccount(void)
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


#define MAX_TEST 10

uint32_t cycles[MAX_TEST];
uint32_t core[MAX_TEST];

bool g_my_app_init_done=false;
bool cpu1_scheduler_started=false;


void IRAM_ATTR start_cpu1(void)
{
    int i;
    int pos=0;
    // Wait for the app to initialze on CPU0
    //while (!g_my_app_init_done) {
    //    ;
    //}
    // Wait for FreeRTOS initialization to finish on PRO CPU
    app_cpu_started=1;


    while (port_xSchedulerRunning[0] == 0) {
        ;
    }
    //Take care putting stuff here: if asked, FreeRTOS will happily tell you the scheduler
    //has started, but it isn't active *on this CPU* yet.
    // Dont think we need this
    esp_crosscore_int_init();

      if (xPortGetCoreID()==1) {
	       //printf("Running on APP CPU!!!!!!!!\n");
      }

    //ESP_EARLY_LOGI(tag, "Starting scheduler on APP CPU.");
    cpu1_scheduler_started=true;
    // S 
    //xPortStartScheduler();

    // Run your code here, dont use any operating system calls.
    uint32_t start=get_ccount();

    uint32_t test;
    for(;;) {
       test=get_ccount();
       if (test<start) {
	        //printf("cpu1 wrap ccount\n");
	        //test=start;
       }
       start=test;
    }
}



void IRAM_ATTR call_start_cpu1()
{
    asm volatile (\
                  "wsr    %0, vecbase\n" \
                  ::"r"(&_init_start));

    cpu_configure_region_protection();

    uartAttach();
    ets_install_uart_printf();
    uart_tx_switch(CONFIG_CONSOLE_UART_NUM);

    //ESP_EARLY_LOGI(tag, "Undercover app cpu up.");
    start_cpu1();
}


static void do_global_ctors(void)
{
    void (**p)(void);
    for (p = &__init_array_end - 1; p >= &__init_array_start; --p) {
        (*p)();
    }
}

static void main_task(void* args)
{
    // Now that the application is about to start, disable boot watchdogs
    REG_CLR_BIT(TIMG_WDTCONFIG0_REG(0), TIMG_WDT_FLASHBOOT_MOD_EN_S);
    REG_CLR_BIT(RTC_CNTL_WDTCONFIG0_REG, RTC_CNTL_WDT_FLASHBOOT_MOD_EN);
    app_main();
    vTaskDelete(NULL);
}


void IRAM_ATTR start_cpu0(void)
{

   // UNSTALL cpu1 (app cpu)
   // This would normally be done for multicore only
    //ESP_EARLY_LOGI(tag, "Starting app cpu, entry point is %p", call_start_cpu1);
    //Flush and enable icache for APP CPU
    Cache_Flush(1);
    Cache_Read_Enable(1);
    esp_cpu_unstall(1);
    //Enable clock gating and reset the app cpu.
    ets_set_appcpu_boot_addr((uint32_t)call_start_cpu1);
    SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);
    CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_C_REG, DPORT_APPCPU_RUNSTALL);
    SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);
    CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);

    while (!app_cpu_started) {
        ets_delay_us(100);
    }
    // TODO!! Already done but needs to be redone after cpu1 is proprly setup.
    // heap_alloc_caps_init();


        esp_setup_syscall_table();
        //esp_set_cpu_freq();     // set CPU frequency configured in menuconfig        esp_clk_init();
       //esp_perip_clk_init();
       //intr_matrix_clear();
         
        uart_div_modify(CONFIG_CONSOLE_UART_NUM, (APB_CLK_FREQ << 4) / CONFIG_CONSOLE_UART_BAUDRATE);          
	//esp_brownout_init();
       rtc_gpio_force_hold_dis_all();
       //esp_setup_time_syscalls();
       esp_vfs_dev_uart_register();
       esp_reent_init(_GLOBAL_REENT);


        const char* default_uart_dev = "/dev/uart/" STRINGIFY(CONFIG_CONSOLE_UART_NUM);
        _GLOBAL_REENT->_stdin  = fopen(default_uart_dev, "r");
        _GLOBAL_REENT->_stdout = fopen(default_uart_dev, "w");
        _GLOBAL_REENT->_stderr = fopen(default_uart_dev, "w");

       do_global_ctors();
       esp_cache_err_int_init();
       esp_crosscore_int_init();
       //esp_ipc_init();
       spi_flash_init();
       /* init default OS-aware flash access critical section */
       spi_flash_guard_set(&g_flash_guard_default_ops);


       xTaskCreatePinnedToCore(&main_task, "main",
			       ESP_TASK_MAIN_STACK, NULL,
			       ESP_TASK_MAIN_PRIO, NULL, 0);
       //ESP_LOGI(tag, "Starting scheduler on PRO CPU.");
       //vTaskStartScheduler();
       // Dont start scheduler.
       int i=0;

       for (;;) {
         for (i=0;i<MAX_TEST;i++) {
	   //printf("cycles %d %u %u\n",i,cycles[i],core[i]);
	 }
       }
}



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
	printf("Running on APP CPU!\n");
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
   xTaskCreate(&test1, "tk0", 2048, &param[0], 5, NULL);
   //xTaskCreatePinnedToCore(&test1, "tk0", 2048, &param[0], 5, NULL,1);
   xTaskCreate(&test1, "tk1", 2048, &param[1], 5, NULL);
   xTaskCreate(&test1, "tk2", 2048, &param[2], 5, NULL);
   xTaskCreate(&test1, "tk3", 2048, &param[3], 5, NULL);
   xTaskCreate(&test1, "tk4", 2048,&param[4], 5, NULL);
   xTaskCreate(&test1, "tk5", 2048,&param[5], 5, NULL);
   xTaskCreate(&test1, "tk6", 2048,&param[6], 5, NULL);
   xTaskCreate(&test1, "tk7", 2048,&param[7], 5, NULL);
   xTaskCreate(&test1, "tk8", 2048,&param[8], 5, NULL);
   xTaskCreate(&test1, "tk9", 2048,&param[9], 5, NULL);
   vTaskDelete(NULL);
}

void pinned_mixed_tests(void *ignore) {
   init_param();
   xTaskCreatePinnedToCore(&test1, "task0", 2048, &param[0], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task1", 2048, &param[1], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task2", 2048, &param[2], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task3", 2048, &param[3], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task4", 2048,&param[4], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task5", 2048,&param[5], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task6", 2048,&param[6], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task7", 2048,&param[7], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task8", 2048,&param[8], 5, NULL,1);
   xTaskCreatePinnedToCore(&test1, "task9", 2048,&param[9], 5, NULL,0);
   vTaskDelete(NULL);
}

void pinned_to_pro_tests(void *ignore) {
   init_param();
   xTaskCreatePinnedToCore(&test1, "task0", 2048, &param[0], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task1", 2048, &param[1], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task2", 2048, &param[2], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task3", 2048, &param[3], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task4", 2048,&param[4], 5, NULL,0);
   xTaskCreatePinnedToCore(&test1, "task5", 2048,&param[5], 5, NULL,0);
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

    //xTaskCreate(&task_tests, "test", 2048, "test", 5, NULL);
    xTaskCreate(&pinned_mixed_tests, "test", 2048, "test", 5, NULL);
    //xTaskCreate(&pinned_to_pro_tests, "test", 2048, "test", 5, NULL);
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
