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
#include "lwip/timers.h"
#include <lwip/stats.h>
#include "lwip/netif.h"
#include "netif/etharp.h"
#include "lwip/tcpip.h"


#include "port/arch/sys_arch.h"
// -- Generic network interface --

extern err_t ethoc_init(struct netif *netif);



struct netif ethoc_if;
//struct netif loop_if;

void ethernet_hardreset(void);	//These reset codes are built for C6711 DSP
void tcpip_init_done_ok(void * arg);


void Task_lwip_init(void * pParam)
{
  ip4_addr_t ipaddr, netmask, gw;
  sys_sem_t sem;
  
  //ethernet_hardreset();//hard reset of EthernetDaughterCard
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

  printf("TCP/IP initializing...\n");  
  if (sys_sem_new(&sem,0)!=ERR_OK) {
    printf("Failed creating semaphore\n");
  }
  // OLAS 
  tcpip_init(tcpip_init_done_ok, &sem);
  sys_sem_wait(sem);
  sys_sem_free(sem);
  printf("TCP/IP initialized.\n");
  
  //add loop interface //set local loop-interface 127.0.0.1
  /*
  IP4_ADDR(&gw, 127,0,0,1);
  IP4_ADDR(&ipaddr, 127,0,0,1);
  IP4_ADDR(&netmask, 255,0,0,0);
  netif_add(&loop_if, &ipaddr, &netmask, &gw, NULL, loopif_init,
	    tcpip_input);
  */
  //add ne2k interface
  IP4_ADDR(&gw, 192,168,1,1);
  IP4_ADDR(&ipaddr, 192,168,1,3);
  IP4_ADDR(&netmask, 255,255,255,0);

  netif_add(&ethoc_if, &ipaddr, &netmask, &gw, NULL, ethoc_init, tcpip_input);
  netif_set_default(&ethoc_if);
  netif_set_up(&ethoc_if); 
  
  printf("Applications started.\n");
  
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
  printf("Finished\n");
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



