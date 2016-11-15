/*
*********************************************************************************************************
*                                              lwIP TCP/IP Stack
*                                    	 port for uC/OS-II RTOS on TIC6711 DSK
*
* File : tcp_ip.c
* By   : ZengMing @ DEP,Tsinghua University,Beijing,China
* Reference: YangYe's source code for SkyEye project
*********************************************************************************************************
*/
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timers.h"
#include <lwip/stats.h>

#include "netif/etharp.h"

#include "ne2kif.h"

/* define this to use QDMA, which is much faster! */
//#define QDMA_Enabled

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 't'

struct ne2k_if {
  struct eth_addr *ethaddr; //MAC Address 
};

struct netif *ne2k_if_netif;   


/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/

/*
 * ethernetif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
err_t ne2k_init(struct netif *netif)
{
  struct ne2k_if *ne2k_if;

  ne2k_if = mem_malloc(sizeof(struct ne2k_if));//MAC Address
  
  if (ne2k_if == NULL)
  {
  		LWIP_DEBUGF(NETIF_DEBUG,("ne2k_init: out of memory!\n"));
  		return ERR_MEM;
  }
  
  netif->state = ne2k_if;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;
  
  ne2k_if->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  
  low_level_init(netif);
  
  etharp_init();
  
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
  
  return ERR_OK;
}

/**
 *  arp_timer.
 */
static void arp_timer(void *arg)
{
  etharp_tmr();
  sys_timeout(ARP_TMR_INTERVAL, (sys_timeout_handler)arp_timer, NULL);
}

/**
 * Initialize the ne2k ethernet chip, resetting the interface and getting the ethernet
 * address.
 */
static void low_level_init(struct netif * netif)
{
	u16_t i;
	struct ne2k_if *ne2k_if;
	
	ne2k_if = netif->state;
	// the meaning of "netif->state" can be defined in drivers, here for MAC address!
	
	netif->hwaddr_len=6;
	netif->mtu = 1500;	
  	netif->flags = NETIF_FLAG_BROADCAST;
  		
	// ---------- start -------------
	//*(u32_t *)EMIF_CE2 = 0x11D4C714; // Set CE2 to 16bits mode,
									 // AX88796 required no less than 160ns period

    //int test= * ((char *) (OC_BASE + MIISTATUS));
	//printf("%08X\n",test);
    //test=EN0_MIISTATUS;

	// intr_matrix_set(xPortGetCoreID(), ETS_TIMER1_INTR_SOURCE, ETS_FRC1_INUM);
    // xt_set_interrupt_handler(ETS_FRC1_INUM, &frc_timer_isr, NULL);                                                           
    // xt_ints_on(1 << ETS_FRC1_INUM);                                   
	
#if 0
	i = EN_RESET; //this instruction let NE2K chip soft reset

    for (i=0;i<DELAY_MS;i++); //wait

    EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP); 
    
    EN0_DCFG = (u8_t) 0x01;

    /* Clear the remote	byte count registers. */
    EN0_RCNTHI = (u8_t) 0x00; 								/* MSB remote byte count reg */
    EN0_RCNTLO = (u8_t) 0x00; 								/* LSB remote byte count reg */

	/* RX configuration reg    Monitor mode (no packet receive) */
	EN0_RXCR = (u8_t) ENRXCR_MON;
	/* TX configuration reg   set internal loopback mode  */
	EN0_TXCR = (u8_t) ENTXCR_LOOP;

    EN0_TPSR = (u8_t) 0x40;					//���ͻ����׵�ַ ��СΪ6ҳ���պ���1��������
    										//Ϊ0x40-0x46 
    
    EN0_STARTPG = (u8_t) 0x46 ;					    /* ���ջ��� 47��Starting page of ring buffer. First page of Rx ring buffer 46h*/
    EN0_BOUNDARY = (u8_t) 0x46 ;						/* Boundary page of ring buffer 0x46*/
    EN0_STOPPG = (u8_t) 0x80 ;    					/* Ending page of ring buffer ,0x80*/

    EN0_ISR = (u8_t) 0xff; 								/* clear the all flag bits in EN0_ISR */
    EN0_IMR = (u8_t) 0x00; 								/* Disable all Interrupt */

    EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_STOP);
    EN1_CURR = (u8_t) 0x47; 							/* keep curr=boundary+1 means no new packet */
           
    EN1_PAR0 = (u8_t)0x12;// MAC_addr.addr[0];	// mac
    EN1_PAR1 = (u8_t)0x34;// MAC_addr.addr[1];
    EN1_PAR2 = (u8_t)0x56;// MAC_addr.addr[2];
    EN1_PAR3 = (u8_t)0x78;// MAC_addr.addr[3];
    EN1_PAR4 = (u8_t)0x9a;// MAC_addr.addr[4];
    EN1_PAR5 = (u8_t)0xe0;// MAC_addr.addr[5];
    
  	/* make up an address. */
  	ne2k_if->ethaddr->addr[0] = (u8_t) 0x12;//MAC_addr.addr[0];
  	ne2k_if->ethaddr->addr[1] = (u8_t) 0x34;//MAC_addr.addr[1];
  	ne2k_if->ethaddr->addr[2] = (u8_t) 0x56;//MAC_addr.addr[2];
  	ne2k_if->ethaddr->addr[3] = (u8_t) 0x78;//MAC_addr.addr[3];
  	ne2k_if->ethaddr->addr[4] = (u8_t) 0x9a;//MAC_addr.addr[4];
  	ne2k_if->ethaddr->addr[5] = (u8_t) 0xe0;//MAC_addr.addr[5];
    
    /* Initialize the multicast list to reject-all.  
       If we enable multicast the higher levels can do the filtering. 
       <multicast filter mask array (8 bytes)> */
    EN1_MAR0 = (u8_t) 0x00;  
    EN1_MAR1 = (u8_t) 0x00;
    EN1_MAR2 = (u8_t) 0x00;
    EN1_MAR3 = (u8_t) 0x00;
    EN1_MAR4 = (u8_t) 0x00;
    EN1_MAR5 = (u8_t) 0x00;
    EN1_MAR6 = (u8_t) 0x00;
    EN1_MAR7 = (u8_t) 0x00;
    
    EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
 
    EN0_IMR = (u8_t) (ENISR_OVER + ENISR_RX + ENISR_RX_ERR);

    EN0_TXCR = (u8_t) 0x00; //E0			//TCR 				
	EN0_RXCR = (u8_t) 0x44;	//CC			//RCR
	    
    EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);
    
    EN0_ISR = (u8_t) 0xff; // clear the all flag bits in EN0_ISR
  #endif
 	ne2k_if_netif = netif;
}


/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/

/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
static err_t low_level_output(struct netif * netif, struct pbuf *p)
{
	struct pbuf *q;
	u16_t packetLength,remote_Addr,Count;
	u8_t *buf;
	
	packetLength = p->tot_len - ETH_PAD_SIZE; //05 01 millin
    
	if ((packetLength) < 64) packetLength = 64; //add pad by the AX88796 automatically

	printf("low_level_output\n");

	// turn off RX int	
	//EN0_IMR = (u8_t) (ENISR_OVER);

	/* We should already be in page 0, but to be safe... */
	//EN_CMD = (u8_t) (EN_PAGE0 + EN_START + EN_NODMA);
	
	// clear the RDC bit	
	//EN0_ISR = (u8_t) ENISR_RDC;
	
	//remote_Addr = (u16_t)(TX_START_PG<<8); 
	
	/*
	 * Write packet to ring buffers.
	 */
   for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    	Count = q->len;
		buf = q->payload;
		
		if (q == p){
           	buf += ETH_PAD_SIZE;
		    Count -= ETH_PAD_SIZE;//Pad in Eth_hdr struct 
        }
		
		// Write data to AX88796
		//remote_Addr = write_AX88796(buf, remote_Addr, Count);	
	} //for

	/* Just send it, and does not check */
	//while (EN_CMD & EN_TRANS);

	//EN0_TPSR   = (u8_t)  TX_START_PG;
	//EN0_TCNTLO = (u8_t) (packetLength & 0xff);
	//EN0_TCNTHI = (u8_t) (packetLength >> 8);
	//EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_TRANS + EN_START);
	
	//EN0_IMR = (u8_t) (ENISR_OVER + ENISR_RX + ENISR_RX_ERR);
	
	#if LINK_STATS
		lwip_stats.link.xmit++;
	#endif /* LINK_STATS */
		
	return ERR_OK;
}

/**
 *  write_AX88796.
 */
u16_t write_AX88796(u8_t * buf, u16_t remote_Addr, u16_t Count)
{
#if 0
	#ifndef QDMA_Enabled
	u16_t loop;
	#endif

	/* AX88796. */
	EN0_RCNTLO = (u8_t) ( Count &  0xff);
	EN0_RCNTHI = (u8_t) ( Count >> 8);
	EN0_RSARLO = (u8_t) ( remote_Addr &  0xff);
	EN0_RSARHI = (u8_t) ( remote_Addr >> 8);
	EN_CMD     = (u8_t) (EN_RWRITE + EN_START + EN_PAGE0);

	// Add for next loop...
	remote_Addr += Count;

	Count = (Count + 1) >> 1; // Turn to 16bits count. <Must add 1 first!>		

	#ifdef QDMA_Enabled
		*(u32_t *)QDMA_SRC   = (u32_t) buf;
		*(u32_t *)QDMA_DST   = (u32_t) &EN_DATA;
		*(u32_t *)QDMA_CNT   = (u32_t) Count;
		*(u32_t *)QDMA_IDX   = 0x00000000;
		*(u32_t *)QDMA_S_OPT = 0x29000001;
	#else
		for (loop=0;loop < Count ;loop++){
			EN_DATA = *(u16_t *)buf;
			buf += 2;
    	}
	#endif //QDMA_Enabled

	while ((EN0_ISR & ENISR_RDC) == 0);

	EN0_ISR = (u8_t) ENISR_RDC;
#endif	
	return remote_Addr;
}


/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/

/*
 * ethernetif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
static void 
ne2k_input(struct netif *netif)
{
  struct ne2k_if *ne2k_if;
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  ne2k_if = netif->state;
  
  /* move received packet into a new pbuf */
  p = low_level_input(netif);
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = p->payload;

#if LINK_STATS
  lwip_stats.link.recv++;
#endif /* LINK_STATS */  

  switch(htons(ethhdr->type)) {
  /* IP packet? */
	case ETHTYPE_IP:
    	/* update ARP table */
    	// OLAS etharp_ip_input(netif, p);
		netif->input(p,netif);
    	/* skip Ethernet header */
    	pbuf_header(p, -(14+ETH_PAD_SIZE));
    	/* pass to network layer */
    	netif->input(p, netif);
    	break;
  case ETHTYPE_ARP:
	    /* pass p to ARP module */
   		etharp_arp_input(netif, ne2k_if->ethaddr, p);
    	break;
  default:
		pbuf_free(p);
		p = NULL;
		break;
  }
}

/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
static struct pbuf * 
low_level_input(struct netif *netif)
{
#if 0
	u16_t packetLength, Count, remote_Addr;
	u8_t  *buf, PDHeader[4];
	u8_t  curr, this_frame, next_frame;
	struct pbuf *p, *q, *r;
	
	EN_CMD     = (u8_t) (EN_PAGE1 + EN_NODMA + EN_START);
	curr       = (u8_t)  EN1_CURR;
	EN_CMD     = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);
	this_frame = (u8_t)  EN0_BOUNDARY + 1;
	
	if (this_frame >= RX_STOP_PG)
		this_frame = RX_START_PG;

	//---------- get the first 4 bytes from AX88796 ---------
	(void) read_AX88796(PDHeader, (u16_t)(this_frame<<8), 4);
	
	
	//----- Store real length, set len to packet length - header ---------
	packetLength = ((unsigned) PDHeader[2] | (PDHeader[3] << 8 )) - 4; // minus PDHeader[4]
	next_frame = (u8_t) (this_frame + 1 + ((packetLength + 4) >> 8));
	
	// Bad frame!
	if ((PDHeader[1] != (u8_t)next_frame) && (PDHeader[1] != (u8_t)(next_frame + 1))
		&& (PDHeader[1] != (u8_t)(next_frame - RX_STOP_PG + RX_START_PG))
		&& (PDHeader[1] != (u8_t)(next_frame + 1 - RX_STOP_PG + RX_START_PG)))
		{
			EN0_BOUNDARY = (u8_t) (curr - 1); 
			return NULL;
		}
	
	// Bogus Packet Size
	if (packetLength > MAX_PACKET_SIZE || packetLength < MIN_PACKET_SIZE) 
		{
			next_frame = PDHeader[1];
			EN0_BOUNDARY = (u8_t) (next_frame-1);
			return NULL;
		}

		    		    	
	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);
	
	EN0_ISR = (u8_t) ENISR_RDC;	// clear the RDC bit	
	
	
	remote_Addr = (u16_t)((this_frame << 8) + 4);
	
	if ((remote_Addr + packetLength + ETH_PAD_SIZE) > (u16_t)(RX_STOP_PG<<8)) {
		p = pbuf_alloc(PBUF_RAW, (u16_t)(RX_STOP_PG<<8) - remote_Addr, PBUF_POOL); /* length of buf */
		packetLength -= (u16_t)(RX_STOP_PG<<8) - remote_Addr - ETH_PAD_SIZE;
	} else {
		p = pbuf_alloc(PBUF_RAW, packetLength+ETH_PAD_SIZE, PBUF_POOL); /* length of buf */
		packetLength = 0;
	}
	
	if(p != NULL) { /* We iterate over the pbuf chain until we have read the entire packet into the pbuf. */
		for(q = p; q != NULL; q= q->next){ /* Read enough bytes to fill this pbuf in the chain. The avaliable data in the pbuf is given by the q->len variable. */
		  	buf = q->payload;		  	
		  	Count = q->len;
		  	if (q == p){                // if it's the first pbuf in chain...
           		buf += ETH_PAD_SIZE;
		    	Count -= ETH_PAD_SIZE;  // pad in Eth_hdr struct 
			}
			remote_Addr = read_AX88796(buf, remote_Addr, Count);
 	    	 	    	
 	    	#if LINK_STATS
    			lwip_stats.link.recv++;
			#endif /* LINK_STATS */  
		} //for(q = p; q != NULL; q= q->next)
	} //if(p != NULL)
	else 
	{   // p == NULL
 	    	#if LINK_STATS
    			lwip_stats.link.memerr++;
    			lwip_stats.link.drop++;
			#endif /* LINK_STATS */      
  	}
  	
  	
  	if (packetLength) // ring buffer cycled
  	{
  		remote_Addr =  (u16_t)((RX_START_PG) << 8);
		r = pbuf_alloc(PBUF_RAW, packetLength, PBUF_POOL); /* length of buf */
		
		if(r != NULL) { /* We iterate over the pbuf chain until we have read the entire packet into the pbuf. */
			for(q = r; q != NULL; q= q->next){ /* Read enough bytes to fill this pbuf in the chain. The avaliable data in the pbuf is given by the q->len variable. */
		  		buf = q->payload;		  	
		  		Count = q->len;
				remote_Addr = read_AX88796(buf, remote_Addr, Count);
			} 	//for
			// link pbuf p & r	
			pbuf_cat(p, r); 
		} 
		else // r == NULL
		{   
 	    	#if LINK_STATS
    			lwip_stats.link.memerr++;
    			lwip_stats.link.drop++;
			#endif      
  		}
	} // if (packetLength)
  	
  	
  	next_frame = PDHeader[1];
	
	EN0_BOUNDARY = (u8_t) (next_frame-1);
	
	return p;
#endif
  return NULL;
}

/**
 *  read_AX88796.
 */
u16_t read_AX88796(u8_t * buf, u16_t remote_Addr, u16_t Count)
{
#if 0
	u8_t  flagOdd=0;
#ifndef QDMA_Enabled
	u16_t loop;
#endif
	
	flagOdd = (Count & 0x0001); // set Flag if Count is odd.
			
	Count -= flagOdd;
			
	EN0_RCNTLO = (u8_t) (Count & 0xff);
	EN0_RCNTHI = (u8_t) (Count >> 8);
	EN0_RSARLO = (u8_t) (remote_Addr & 0xff);
	EN0_RSARHI = (u8_t) (remote_Addr >> 8);
	EN_CMD = (u8_t) (EN_PAGE0 + EN_RREAD + EN_START);

	remote_Addr += Count;

	Count = Count>>1;

	#ifdef QDMA_Enabled
		*(u32_t *)QDMA_SRC   = (u32_t) &EN_DATA;
		*(u32_t *)QDMA_DST   = (u32_t) buf;
		*(u32_t *)QDMA_CNT   = (u32_t) Count;
		*(u32_t *)QDMA_IDX   = 0x00000000;
		*(u32_t *)QDMA_S_OPT = 0x28200001;
		buf += Count*2;
	#else
		for (loop=0;loop < Count ;loop++){
	   		*(u16_t *)buf = EN_DATA ;
			buf += 2;
	   		}
	#endif //QDMA_Enabled
				
	while ((EN0_ISR & ENISR_RDC) == 0);
	
	EN0_ISR = (u8_t) ENISR_RDC;
			
	if (flagOdd) {
		EN0_RCNTLO = 0x01;
		EN0_RCNTHI = 0x00;
		EN0_RSARLO = (u8_t) (remote_Addr & 0xff);
		EN0_RSARHI = (u8_t) (remote_Addr >> 8);
		EN_CMD = (u8_t) (EN_PAGE0 + EN_RREAD + EN_START);
	
		remote_Addr += 1;

		*(u8_t *)buf = *(u8_t *)(Base_ADDR+0x10) ;
		
		while ((EN0_ISR & ENISR_RDC) == 0);
		
		EN0_ISR = (u8_t) ENISR_RDC;		
	}
	
#endif
	return remote_Addr;
}

/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/

/**
 *  ne2k_rx_err.
 */
void ne2k_rx_err(void)
{
#if 0
		u8_t  curr;
		EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_STOP);
		curr = (u8_t) EN1_CURR;
		EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
		EN0_BOUNDARY = (u8_t) curr-1;
#endif
}

/**
 *  ne2k_rx.
 */
void ne2k_rx(void)
{
#if 0
	u8_t  curr,bnry,loopCnt = 0;
		
	while(loopCnt < 10) {
		
		EN_CMD = (u8_t) (EN_PAGE1 + EN_NODMA + EN_STOP);
		curr = (u8_t) EN1_CURR;
		EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
		bnry = (u8_t) EN0_BOUNDARY + 1;
		
		if (bnry >= RX_STOP_PG)
			bnry = RX_START_PG;
		
		if (curr == bnry) break;
		
		ne2k_input(ne2k_if_netif);
		
		loopCnt++;
		}
#endif
}

/*---*---*---*---*---*---*---*
 *     void ne2k_isr(void)
 *    can be int 4 5 6 or 7 
 *---*---*---*---*---*---*---*/
void ne2k_isr(void)
{
#if 0	
	DSP_C6x_Save();

	OSIntEnter();
	
	if (OSIntNesting == 1)
		{
			OSTCBCur->OSTCBStkPtr = (OS_STK *) DSP_C6x_GetCurrentSP();
		}
			
	/* You can enable Interrupt again here, 
		if want to use nested interrupt..... */
	//------------------------------------------------------------

	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
	//outb(CMD_PAGE0 | CMD_NODMA | CMD_STOP,NE_CR);
	
	EN0_IMR = (u8_t) 0x00;//close
	
	// ram overflow interrupt
	if (EN0_ISR & ENISR_OVER) {
		EN0_ISR = (u8_t) ENISR_OVER;		// clear interrupt
	}
	
	// error transfer interrupt ,NIC abort tx due to excessive collisions	
	if (EN0_ISR & ENISR_TX_ERR) {
		EN0_ISR = (u8_t) ENISR_TX_ERR;		// clear interrupt
	 	//temporarily do nothing
	}

	// Rx error , reset BNRY pointer to CURR (use SEND PACKET mode)
	if (EN0_ISR & ENISR_RX_ERR) {
		EN0_ISR = (u8_t) ENISR_RX_ERR;		// clear interrupt
		ne2k_rx_err();
	}

	//got packet with no errors
	if (EN0_ISR & ENISR_RX) {
		EN0_ISR = (u8_t) ENISR_RX;
		ne2k_rx();		
	}
		
	//Transfer complelte, do nothing here
	if (EN0_ISR & ENISR_TX){
		EN0_ISR = (u8_t) ENISR_TX;		// clear interrupt
	}
	
	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_STOP);
	
	EN0_ISR = (u8_t) 0xff;			// clear ISR	
	
	EN0_IMR = (u8_t) (ENISR_OVER + ENISR_RX + ENISR_RX_ERR);
	
	//open nic for next packet
	EN_CMD = (u8_t) (EN_PAGE0 + EN_NODMA + EN_START);

	//if (led_stat & 0x04) {LED3_on;}
	//else {LED3_off;}
		
	//--------------------------------------------------------
		
	OSIntExit();
	
	DSP_C6x_Resume();
	
	// this can avoid a stack error when compile with the optimization!
#endif
}
