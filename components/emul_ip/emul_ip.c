/*
*********************************************************************************************************
*                                              lwIP TCP/IP Stack
*                                    	 port for uC/OS-II RTOS on TIC6711 DSK
*
* File : tcp_ip.c
* By   : ZengMing @ DEP,Tsinghua University,Beijing,China
*********************************************************************************************************
*/
#include    <stdio.h>
#include    <string.h>
#include    <ctype.h>
#include    <stdlib.h>
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
//#include "lwip/timers.h"
#include <lwip/stats.h>
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/init.h"
#include "esp_event.h"
#include "esp_event_loop.h"


#include "arch/sys_arch.h"
//#include "port/arch/sys_arch.h"
// -- Generic network interface --


//QueueHandle_t esp_event_loop_get_queue(void);

extern err_t ethoc_init(struct netif *netif);

int is_running_qemu() {
    int *quemu_test=(int *)  0x3ff005f0;
    int ret_val;

    if (*quemu_test==0x42) {
        printf("Running in qemu\n");
        ret_val=1;
    } else {
      ret_val=0;
    }

    return ret_val;
}


struct netif ethoc_if;
//struct netif loop_if;

void ethernet_hardreset(void);	//These reset codes are built for C6711 DSP
void tcpip_init_done_ok(void * arg);


void task_lwip_init(void * pParam)
{
  ip4_addr_t ipaddr, netmask, gw;
  sys_sem_t sem;

  // From esp-idf
  //lwip_init();
  //sys_init();
  tcpip_adapter_init();
  //ethernet_hardreset();//hard reset of EthernetDaughterCard
  // This should be done in lwip_init
# if 0  
  #if LWIP_STATS
  stats_init();
  #endif
  
  // initial lwIP stack
  sys_init();
  mem_init();
  memp_init();
  pbuf_init();
  netif_init(); 
#endif  // The initiation above is done in tcpip_init

  
  //add loop interface //set local loop-interface 127.0.0.1
  /*
  IP4_ADDR(&gw, 127,0,0,1);
  IP4_ADDR(&ipaddr, 127,0,0,1);
  IP4_ADDR(&netmask, 255,0,0,0);
  netif_add(&loop_if, &ipaddr, &netmask, &gw, NULL, loopif_init,
	    tcpip_input);
  */
  //add interface
  IP4_ADDR(&gw, 192,168,4,1);
  IP4_ADDR(&ipaddr, 192,168,4,3);
  IP4_ADDR(&netmask, 255,255,255,0);

  netif_add(&ethoc_if, &ipaddr, &netmask, &gw, NULL, ethoc_init, tcpip_input);
  netif_set_default(&ethoc_if);
  

  printf("TCP/IP initializing...\n");  
  //if (sys_sem_new(&sem,0)!=ERR_OK) {
  //  printf("Failed creating semaphore\n");
  //}
  // OLAS 
  //tcpip_init(tcpip_init_done_ok, &sem);
  //sys_sem_wait(sem);
  //sys_sem_free(sem);
  printf("TCP/IP initialized.\n");

  netif_set_up(&ethoc_if); 

 // Fake got_ip event
  
 //if (esp_event_loop_get_queue()!=NULL) {
   system_event_t evt;

   //ip4_addr_set(&ip_info->ip, ip_2_ip4(&netif->ip_addr));
   //ip4_addr_set(&ip_info->netmask, ip_2_ip4(&netif->netmask));
   //ip4_addr_set(&ip_info->gw, ip_2_ip4(&netif->gw));

   //notify event
   evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
   memcpy(&evt.event_info.got_ip.ip_info.ip, &ipaddr, sizeof(ipaddr));
   memcpy(&evt.event_info.got_ip.ip_info.netmask, &netmask, sizeof(netmask));
   memcpy(&evt.event_info.got_ip.ip_info.gw, &gw, sizeof(gw));
 
   //memcpy(&evt.event_info.got_ip.ip_info, &ipaddr, sizeof(tcpip_adapter_ip_info_t));

   esp_event_send(&evt);
   //}


  printf("Applications started.\n");

  vTaskDelete(NULL);
  
  //---------------------------------------------------------------------
  //All thread(task) of lwIP must have their PRI between 10 and 14.
  //------------------------------------------------------------  
  //sys_thread_new(httpd_init, (void*)"httpd",10);
  //httpd_init();
  //---------------------------------------------------------------------
  

  //DSP_C6x_TimerInit(); // Timer interrupt enabled
  // Ethernet interrupt
  //DSP_C6x_Int4Init();  // Int4(Ethernet Chip int) enabled


  /* Block for ever. */
  //sem = sys_sem_new(0);
  //sys_sem_wait(sem);
  //printf("Finished\n");
}

//---------------------------------------------------------
void tcpip_init_done_ok(void * arg)
{
  sys_sem_t *sem;
  sem = arg;
  sys_sem_signal(*sem);
}


/*-----------------------------------------------------------*/
/*  This function do the hard reset of EthernetDaughterCard  *
 *   through the DaughterBoardControl0 signal in DB-IF		 */  
/*-----------------------------------------------------------*/
void ethernet_hardreset(void)	//These reset codes are built for C6711 DSK
{

}



