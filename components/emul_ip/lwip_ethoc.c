/*
*********************************************************************************************************
*                                              lwIP TCP/IP Stack
*                                    	 port for RTOS on tesscilica open code
* Based on linux/drivers/net/ethernet/ethoc.c 
*
* Copyright (C) 2007-2008 Avionic Design Development GmbH
* Copyright (C) 2008-2009 Avionic Design GmbH
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* Written by Thierry Reding <thierry.reding@avionic-design.de>
*
* By   : ZengMing @ DEP,Tsinghua University,Beijing,China
* Reference: YangYe's source code for SkyEye project
*********************************************************************************************************
*/
// SKB
// http://amsekharkernel.blogspot.se/2014/08/what-is-skb-in-linux-kernel-what-are.html
// LWIP
// http://www.atmel.com/Images/Atmel-42233-Using-the-lwIP-Network-Stack_AP-Note_AT04055.pdf
// http://lwip.wikia.com/wiki/LwIP_Wiki

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include <lwip/stats.h>
#include "netif/etharp.h"
#include <string.h>
#include "rom/ets_sys.h"
#include "esp_intr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/xtensa_api.h"
#include "freertos/portmacro.h"
#include "netif/etharp.h"

u16_t lwip_standard_chksum(const void *dataptr, int len);

TaskHandle_t pollHandle = NULL;

SemaphoreHandle_t xSemaphore = NULL;

#include "lwip_ethoc.h"

/* define this to use QDMA, which is much faster! */
//#define QDMA_Enabled

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

static char hostname[16];


struct ethoc_if {
  struct eth_addr *ethaddr; //MAC Address 
};

struct netif *ethoc_if_netif;   

/**
 *  Driver functions.
 */
static void low_level_init(struct netif * netif);
static void arp_timer(void *arg);

static err_t ethoc_low_level_output(struct netif * netif,struct pbuf *p);
static struct pbuf * low_level_input(struct netif *netif);

static portMUX_TYPE lock_xmit_spinlock = portMUX_INITIALIZER_UNLOCKED;

//static portMUX_TYPE lock_rx_lock = portMUX_INITIALIZER_UNLOCKED;
#define LONG_TIME 0xffff

/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/

// Linux macros
#define u32 uint32_t
#define u16 unsigned short
#define u8 unsigned char
#define loff_t uint32_t

#define memcpy_fromio memcpy
//	memcpy_toio(dest, skb->data, skb->len);
#define memcpy_toio memcpy

#define likely(x) (x)
#define unlikely(x) (x)

/**
 * struct ethoc - driver-private device structure
 * @iobase:	pointer to I/O memory region
 * @membase:	pointer to buffer memory region
 * @dma_alloc:	dma allocated buffer size
 * @io_region_size:	I/O memory region size
 * @num_bd:	number of buffer descriptors
 * @num_tx:	number of send buffers
 * @cur_tx:	last send buffer written
 * @dty_tx:	last buffer actually sent
 * @num_rx:	number of receive buffers
 * @cur_rx:	current receive buffer
 * @vma:        pointer to array of virtual memory addresses for buffers
 * @netdev:	pointer to network device structure
 * @napi:	NAPI structure
 * @msg_enable:	device state flags
 * @lock:	device lock
 * @mdio:	MDIO bus for PHY access
 * @phy_id:	address of attached PHY
 */
typedef struct ethoc {
	void  *iobase;
	void  *membase;
	int dma_alloc;
	//resource_size_t io_region_size;
	//bool big_endian;

	unsigned int num_bd;
	unsigned int num_tx;
	unsigned int cur_tx;
	unsigned int dty_tx;

	unsigned int num_rx;
	unsigned int cur_rx;

	void *vma[20];

	struct netif *netdev;
	u32 msg_enable;
	//spinlock_t lock;

	struct mii_bus *mdio;
	struct clk *clk;
	signed char  phy_id;
} ethoc_t;

ethoc_t priv_ethoc;


/**
 * struct ethoc_bd - buffer descriptor
 * @stat:	buffer statistics
 * @addr:	physical memory address
 */
struct ethoc_bd {
	uint32_t stat;
	uint32_t addr;
};

static inline u32 ioread32(const volatile void  *addr)
{
         return *(const volatile u32  *)addr;
}

static inline void iowrite32(u32 value, volatile void  *addr)
{
          *(volatile u32  *)addr = value;
}


static inline uint32_t ethoc_read(struct ethoc *dev, loff_t offset)
{
		return ioread32(dev->iobase + offset);
}

static inline void ethoc_write(struct ethoc *dev, loff_t offset, uint32_t data)
{
		iowrite32(data, dev->iobase + offset);
}

static inline void ethoc_read_bd(struct ethoc *dev, int index,
		struct ethoc_bd *bd)
{
	loff_t offset = ETHOC_BD_BASE + (index * sizeof(struct ethoc_bd));
	bd->stat = ethoc_read(dev, offset + 0);
	bd->addr = ethoc_read(dev, offset + 4);
}

static inline void ethoc_write_bd(struct ethoc *dev, int index,
		const struct ethoc_bd *bd)
{
	loff_t offset = ETHOC_BD_BASE + (index * sizeof(struct ethoc_bd));
	ethoc_write(dev, offset + 0, bd->stat);
	ethoc_write(dev, offset + 4, bd->addr);
}

static inline void ethoc_enable_irq(struct ethoc *dev, u32 mask)
{
	u32 imask = ethoc_read(dev, INT_MASK);
	imask |= mask;
	ethoc_write(dev, INT_MASK, imask);
}

static inline void ethoc_disable_irq(struct ethoc *dev, u32 mask)
{
	u32 imask = ethoc_read(dev, INT_MASK);
	imask &= ~mask;
	ethoc_write(dev, INT_MASK, imask);
}


static inline void ethoc_ack_irq(struct ethoc *dev, u32 mask)
{
	ethoc_write(dev, INT_SOURCE, mask);
}

static inline void ethoc_enable_rx_and_tx(struct ethoc *dev)
{
	u32 mode = ethoc_read(dev, MODER);
	mode |= MODER_RXEN | MODER_TXEN;
	ethoc_write(dev, MODER, mode);
}

static inline void ethoc_disable_rx_and_tx(struct ethoc *dev)
{
	u32 mode = ethoc_read(dev, MODER);
	mode &= ~(MODER_RXEN | MODER_TXEN);
	ethoc_write(dev, MODER, mode);
}

static int ethoc_init_ring(struct ethoc *dev, unsigned long mem_start)
{
	struct ethoc_bd bd;
	int i;
	void *vma;

	dev->cur_tx = 0;
	dev->dty_tx = 0;
	dev->cur_rx = 0;

	ethoc_write(dev, TX_BD_NUM, dev->num_tx);

	/* setup transmission buffers */
	bd.addr = mem_start;
	bd.stat = TX_BD_IRQ | TX_BD_CRC;
	vma = dev->membase;

	for (i = 0; i < dev->num_tx; i++) {
		if (i == dev->num_tx - 1)
			bd.stat |= TX_BD_WRAP;

		ethoc_write_bd(dev, i, &bd);
		bd.addr += ETHOC_BUFSIZ;

		dev->vma[i] = vma;
		vma += ETHOC_BUFSIZ;
	}

	bd.stat = RX_BD_EMPTY | RX_BD_IRQ;

	for (i = 0; i < dev->num_rx; i++) {
		if (i == dev->num_rx - 1)
			bd.stat |= RX_BD_WRAP;

		ethoc_write_bd(dev, dev->num_tx + i, &bd);
		bd.addr += ETHOC_BUFSIZ;

		dev->vma[dev->num_tx + i] = vma;
		vma += ETHOC_BUFSIZ;
	}

	return 0;
}

static int ethoc_reset(struct ethoc *dev)
{
	u32 mode;

	/* TODO: reset controller? */

	ethoc_disable_rx_and_tx(dev);

	/* TODO: setup registers */

	/* enable FCS generation and automatic padding */
	mode = ethoc_read(dev, MODER);
	mode |= MODER_CRC | MODER_PAD;
	ethoc_write(dev, MODER, mode);

	/* set full-duplex mode */
	mode = ethoc_read(dev, MODER);
	mode |= MODER_FULLD;
	ethoc_write(dev, MODER, mode);
	ethoc_write(dev, IPGT, 0x15);

	ethoc_ack_irq(dev, INT_MASK_ALL);
	ethoc_enable_irq(dev, INT_MASK_ALL);
	ethoc_enable_rx_and_tx(dev);
	return 0;
}
static unsigned int ethoc_update_rx_stats(struct ethoc *dev,
		struct ethoc_bd *bd)
{
	//struct netif *netdev = dev->netdev;
	unsigned int ret = 0;

	if (bd->stat & RX_BD_TL) {
		printf( "RX: frame too long\n");
		//netdev->stats.rx_length_errors++;
		ret++;
	}

	if (bd->stat & RX_BD_SF) {
		printf( "RX: frame too short\n");
		//netdev->stats.rx_length_errors++;
		ret++;
	}

	if (bd->stat & RX_BD_DN) {
		printf( "RX: dribble nibble\n");
		//netdev->stats.rx_frame_errors++;
	}

	if (bd->stat & RX_BD_CRC) {
		printf( "RX: wrong CRC\n");
		//netdev->stats.rx_crc_errors++;
		ret++;
	}

	if (bd->stat & RX_BD_OR) {
		printf( "RX: overrun\n");
		//netdev->stats.rx_over_errors++;
		ret++;
	}

	//if (bd->stat & RX_BD_MISS)
	//	netdev->stats.rx_missed_errors++;

	if (bd->stat & RX_BD_LC) {
		printf("RX: late collision\n");
		//netdev->stats.collisions++;
		ret++;
	}

	return ret;
}


static int ethoc_rx(struct netif *dev, int limit)
{
	struct ethoc *priv = &priv_ethoc;
	int count;
	struct eth_hdr *ethhdr;
	struct ethoc_bd bd;
	struct ethoc_bd bd_last;  // Memory corruption?? Need this?


	for (count = 0; count < limit; ++count) {
		unsigned int entry;
		portENTER_CRITICAL(&lock_xmit_spinlock);

		entry = priv->num_tx + priv->cur_rx ;

    	portEXIT_CRITICAL(&lock_xmit_spinlock);


		for (count = 8; count < 16; ++count) {
			if (bd.stat & RX_BD_EMPTY) {
			}
			else {
                //printf("rx_data in %d entry %d\n",count,entry);	
				//entry=count;
				//bd.stat &= ~RX_BD_STATS;
				//bd.stat |=  RX_BD_EMPTY;
				//ethoc_write_bd(priv, entry, &bd);
			}
		}



		//printf(". %d\n",entry);

		ethoc_read_bd(priv, entry, &bd);
		if (bd.stat & RX_BD_EMPTY) {
			ethoc_ack_irq(priv, INT_MASK_RX);
			/* If packet (interrupt) came in between checking
			 * BD_EMTPY and clearing the interrupt source, then we
			 * risk missing the packet as the RX interrupt won't
			 * trigger right away when we reenable it; hence, check
			 * BD_EMTPY here again to make sure there isn't such a
			 * packet waiting for us...
			 */
			ethoc_read_bd(priv, entry, &bd);
			if (bd.stat & RX_BD_EMPTY)
				break;
		}

		if (ethoc_update_rx_stats(priv, &bd) == 0) {
			//printf("ethoc_rx\n");
			int size = bd.stat >> 16;
			//struct sk_buff *skb;
			struct pbuf *skb;

			if (size>0)
			{
				//if (size>4)
				//  size -= 4; /* strip the CRC */
				//printf("size %d\n",size);

				//skb = netdev_alloc_skb_ip_align(dev, size);
				skb = pbuf_alloc(PBUF_RAW, size, PBUF_POOL); /* length of buf */ // PBUF_REF

				if (likely(skb)) {
					struct eth_hdr* ethhdr;

					void *src = priv->vma[entry];
  				    //memcpy_fromio(skb_put(skb, size), src, size);
					//printf("trace_open_eth_receive_desc(%x,%x)\n",(unsigned int)src, size);

					ethhdr = (struct eth_hdr *)src;
#if 0
    printf("ethernet_driver: dest:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", src:%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F":%"X8_F", type:%"X16_F"\n",
     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
     (unsigned)ethhdr->src.addr[0], (unsigned)ethhdr->src.addr[1], (unsigned)ethhdr->src.addr[2],
     (unsigned)ethhdr->src.addr[3], (unsigned)ethhdr->src.addr[4], (unsigned)ethhdr->src.addr[5],
     (unsigned)htons(ethhdr->type));
#endif



					memcpy_fromio(skb->payload,src,size);
					skb->len=size;
					//skb->protocol = eth_type_trans(skb, dev);
					ethhdr = skb->payload;

					dev->input(skb, dev);

					//dev->stats.rx_packets++;
					//dev->stats.rx_bytes += size;
				} else {
					printf("No available pbuffers!!\n");
					/*
					if (net_ratelimit())
						dev_warn(&dev->dev,
							"low on memory - packet dropped\n");

					dev->stats.rx_dropped++;
					*/
					break;
				}
			}
		}

		/* clear the buffer descriptor so it can be reused */
		bd.stat &= ~RX_BD_STATS;
		bd.stat |=  RX_BD_EMPTY;
		ethoc_write_bd(priv, entry, &bd);

		portENTER_CRITICAL(&lock_xmit_spinlock);

		if (++priv->cur_rx == priv->num_rx)
			priv->cur_rx = 0;

		portEXIT_CRITICAL(&lock_xmit_spinlock);
	
	}

	return count;
}

static void ethoc_update_tx_stats(struct ethoc *dev, struct ethoc_bd *bd)
{
	//struct netif *netdev = dev->netdev;

	if (bd->stat & TX_BD_LC) {
		printf("TX: late collision\n");
		//netdev->stats.tx_window_errors++;
	}

	if (bd->stat & TX_BD_RL) {
		printf( "TX: retransmit limit\n");
		//netdev->stats.tx_aborted_errors++;
	}

	if (bd->stat & TX_BD_UR) {
		printf( "TX: underrun\n");
		//netdev->stats.tx_fifo_errors++;
	}

	if (bd->stat & TX_BD_CS) {
		printf( "TX: carrier sense lost\n");
		//netdev->stats.tx_carrier_errors++;
	}

	//if (bd->stat & TX_BD_STATS)
	//	netdev->stats.tx_errors++;

	//netdev->stats.collisions += (bd->stat >> 4) & 0xf;
	//netdev->stats.tx_bytes += bd->stat >> 16;
	//netdev->stats.tx_packets++;
}

static int ethoc_tx(int limit)
{
	struct ethoc *priv = &priv_ethoc;
	int count;
	struct ethoc_bd bd;

	for (count = 0; count < limit; ++count) {
		unsigned int entry;

		entry = priv->dty_tx & (priv->num_tx-1);

		ethoc_read_bd(priv, entry, &bd);

		if (bd.stat & TX_BD_READY || (priv->dty_tx == priv->cur_tx)) {
			ethoc_ack_irq(priv, INT_MASK_TX);
			/* If interrupt came in between reading in the BD
			 * and clearing the interrupt source, then we risk
			 * missing the event as the TX interrupt won't trigger
			 * right away when we reenable it; hence, check
			 * BD_EMPTY here again to make sure there isn't such an
			 * event pending...
			 */
			ethoc_read_bd(priv, entry, &bd);
			if (bd.stat & TX_BD_READY ||
			    (priv->dty_tx == priv->cur_tx))
				break;
		}

		ethoc_update_tx_stats(priv, &bd);
		priv->dty_tx++;
	}

	// TODO, no wake queue
	//if ((priv->cur_tx - priv->dty_tx) <= (priv->num_tx / 2))
	//	netif_wake_queue(dev);

	return count;
}

int ethoc_poll(int budget);

static void ethoc_interrupt()
{
	struct ethoc *priv = &priv_ethoc;
	u32 pending;
	u32 mask;

    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/* Figure out what triggered the interrupt...
	 * The tricky bit here is that the interrupt source bits get
	 * set in INT_SOURCE for an event regardless of whether that
	 * event is masked or not.  Thus, in order to figure out what
	 * triggered the interrupt, we need to remove the sources
	 * for all events that are currently masked.  This behaviour
	 * is not particularly well documented but reasonable...
	 */
	mask = ethoc_read(priv, INT_MASK);
	pending = ethoc_read(priv, INT_SOURCE);
	pending &= mask;


	// No interrupt to handle....
	if (unlikely(pending == 0))
	{
		//printf("No pendig irq, spurious.\n");
		ethoc_ack_irq(priv, INT_MASK_ALL);		
		return;
	}
    //printf("IRQ\n");

	ethoc_ack_irq(priv, pending);

	/* We always handle the dropped packet interrupt */
	if (pending & INT_MASK_BUSY) {
		//printf("packet dropped\n");
		//dev->stats.rx_dropped++;
	}

	/* Handle receive/transmit event by switching to polling */ 
	// INT_MASK_TX 
	if (pending & ( INT_MASK_RX)) {
		//ethoc_disable_irq(priv, INT_MASK_TX | INT_MASK_RX);
		//napi_schedule(&priv->napi);
		//xTaskResumeFromISR(pollHandle);

		ethoc_ack_irq(priv, INT_MASK_ALL);		
		xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );

        if( xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR(); // this wakes up sample_timer_task immediately
        }
	}

	return;
}

#ifdef NOT_USED
static int ethoc_get_mac_address(struct netif *dev, void *addr)
{
	struct ethoc *priv =  &priv_ethoc;
	u8 *mac = (u8 *)addr;
	u32 reg;

	reg = ethoc_read(priv, MAC_ADDR0);
	mac[2] = (reg >> 24) & 0xff;
	mac[3] = (reg >> 16) & 0xff;
	mac[4] = (reg >>  8) & 0xff;
	mac[5] = (reg >>  0) & 0xff;

	reg = ethoc_read(priv, MAC_ADDR1);
	mac[0] = (reg >>  8) & 0xff;
	mac[1] = (reg >>  0) & 0xff;

	return 0;
}
#endif

int ethoc_poll(int budget)
{
	struct ethoc *priv = &priv_ethoc;
	int rx_work_done = 0;
	int tx_work_done = 0;

	rx_work_done = ethoc_rx(priv->netdev, budget);
	// TODO, Dont understand the driver... This transmits again?!
	tx_work_done = ethoc_tx( budget);

	if (rx_work_done < budget && tx_work_done < budget) {
		//napi_complete(napi);
		ethoc_enable_irq(priv, INT_MASK_TX | INT_MASK_RX);
	}

	return rx_work_done;
}

#ifdef NOT_UESD
static int ethoc_mdio_read(int phy, int reg)
{
	struct ethoc *priv = &priv_ethoc;
	int i;

	ethoc_write(priv, MIIADDRESS, MIIADDRESS_ADDR(phy, reg));
	ethoc_write(priv, MIICOMMAND, MIICOMMAND_READ);

	for (i = 0; i < 5; i++) {
		u32 status = ethoc_read(priv, MIISTATUS);
		if (!(status & MIISTATUS_BUSY)) {
			u32 data = ethoc_read(priv, MIIRX_DATA);
			/* reset MII command register */
			ethoc_write(priv, MIICOMMAND, 0);
			return data;
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		//usleep_range(100, 200);
	}

	return -EBUSY;
}

static int ethoc_mdio_write( int phy, int reg, u16 val)
{
	struct ethoc *priv = &priv_ethoc;
	int i;

	ethoc_write(priv, MIIADDRESS, MIIADDRESS_ADDR(phy, reg));
	ethoc_write(priv, MIITX_DATA, val);
	ethoc_write(priv, MIICOMMAND, MIICOMMAND_WRITE);

	for (i = 0; i < 5; i++) {
		u32 stat = ethoc_read(priv, MIISTATUS);
		if (!(stat & MIISTATUS_BUSY)) {
			/* reset MII command register */
			ethoc_write(priv, MIICOMMAND, 0);
			return 0;
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		//usleep_range(100, 200);
	}

	return -EBUSY;
}

static void ethoc_mdio_poll(struct netif *dev)
{
}

static int ethoc_mdio_probe(struct netif *dev)
{
	//struct ethoc *priv = &priv_ethoc;
	struct phy_device *phy;
	int err;

    phy=NULL;
	//if (priv->phy_id != -1)
	//	phy = mdiobus_get_phy(priv->mdio, priv->phy_id);
	//else
	//	phy = phy_find_first(priv->mdio);

	if (!phy) {
		printf( "no PHY found\n");
		return -ENXIO;
	}

	//err = phy_connect_direct(dev, phy, ethoc_mdio_poll,
	//			 PHY_INTERFACE_MODE_GMII);

	if (err) {
		printf("could not attach to PHY\n");
		return err;
	}

	//phy->advertising &= ~(ADVERTISED_1000baseT_Full |
    //			      ADVERTISED_1000baseT_Half);
	//phy->supported &= ~(SUPPORTED_1000baseT_Full |
	//		    SUPPORTED_1000baseT_Half);

	return 0;
}
#endif

// Cant get interrupts to work so we have to poll for packets
void poll_task(void *pvParameter) {
    struct ethoc *priv = &priv_ethoc;

    //ethoc_reset(priv);

    for(;;) {
	  //if( xSemaphoreTake( xSemaphore, LONG_TIME ) == pdTRUE ) {
         ethoc_poll(8);
	  //}
	  //vTaskSuspend(NULL);

       vTaskDelay(1);
	  
    }
}


int ethoc_open(struct netif *dev)
{
    struct ethoc *priv = &priv_ethoc;
    int ret=0;

    priv->iobase=(void  *)OC_BASE;
    priv->membase=(void  *)OC_BUF_START;

    //static void IRAM_ATTR frc_timer_isr()
    // intr_matrix_set(xPortGetCoreID(), ETS_TIMER1_INTR_SOURCE, ETS_FRC1_INUM);
    // xt_set_interrupt_handler(ETS_FRC1_INUM, &frc_timer_isr, NULL);
    // xt_ints_on(1 << ETS_FRC1_INUM);                                   

    /*
    //SET_PERI_REG_MASK(FRC_TIMER_CTRL_REG(0),
    //                FRC_TIMER_ENABLE | \   
    //                FRC_TIMER_INT_ENABLE);  
    */
    // Cant get this to work with qemu
    xSemaphore = xSemaphoreCreateBinary();


    //intr_matrix_set(xPortGetCoreID(), ETS_RWBLE_NMI_SOURCE /*ETS_ETH_MAC_INTR_SOURCE*/, 9);
    //xt_set_interrupt_handler(9, &ethoc_interrupt, NULL);                                                           
    //xt_ints_on(1 << 9);                                   

   //ret = request_irq(dev->irq, ethoc_interrupt, IRQF_SHARED,
   //		dev->name, dev);

   //if (ret)
   //	return ret;

    ethoc_init_ring(priv, OC_BUF_START);
    ethoc_reset(priv);

    // Poll for input
	// Interrupts work now, Should not need to do polling 
    xTaskCreate(&poll_task,"poll_task",2048, NULL, 25, &pollHandle);


	//if (netif_queue_stopped(dev)) {
	//	dev_dbg(&dev->dev, " resuming queue\n");
	//	netif_wake_queue(dev);
	//} else {
	//	dev_dbg(&dev->dev, " starting queue\n");
	//	netif_start_queue(dev);
	//}

	//phy_start(dev->phydev);
	//napi_enable(&priv->napi);

	//if (netif_msg_ifup(priv)) {
	//	dev_info(&dev->dev, "I/O: %08lx Memory: %08lx-%08lx\n",
	//			dev->base_addr, dev->mem_start, dev->mem_end);
	//}


	return 0;
}



unsigned char packet[ETHOC_BUFSIZ+40];

static int ethoc_start_xmit( struct pbuf *skb, struct netif *dev)
{
	struct ethoc *priv = &priv_ethoc;
	struct ethoc_bd bd;
	unsigned int entry;
	void *dest;
	int len=skb->len;

	//printf("pbuf len %d TODO?, pad to 64\n",skb->len);
	// pad to length 64

	if (skb->len>ETHOC_BUFSIZ) {
		printf("ETHERNET PACKET TOO BIG\n");
	} 
	else
    {
	   memcpy(packet,skb->payload,skb->len);
	}

	if (skb->len<ETHOC_ZLEN) {
		u16_t crc=lwip_standard_chksum(packet,skb->len);
		// TODO, qemu does not use the TX_BD_PAD bit..
		//printf("<%x>",crc); , does not work.

		// Dont write 
		packet[skb->len+1]= crc & 0xff;
		packet[skb->len+2]= (crc & 0xff00) << 8 ;

		len=ETHOC_ZLEN;
	}
	//if (skb_put_padto(skb, ETHOC_ZLEN)) {
	//	dev->stats.tx_errors++;
	//	goto out_no_free;
	//}

	if (unlikely(skb->len > ETHOC_BUFSIZ)) {
		//dev->stats.tx_errors++;
		goto out;
	}

	entry = priv->cur_tx % priv->num_tx;
	//spin_lock_irq(&priv->lock);
	portENTER_CRITICAL(&lock_xmit_spinlock);
	priv->cur_tx++;

	// Our qemu emulated hw, does not handle TX_BD_PAD
	ethoc_read_bd(priv, entry, &bd);
	if (unlikely(len < ETHOC_ZLEN))
		bd.stat |=  TX_BD_PAD;
	else
		bd.stat &= ~TX_BD_PAD;

	dest = priv->vma[entry];
	memcpy_toio(dest, packet,len);
	bd.stat &= ~(TX_BD_STATS | TX_BD_LEN_MASK);
	bd.stat |= TX_BD_LEN(len);

	ethoc_write_bd(priv, entry, &bd);

	bd.stat |= TX_BD_READY;
	ethoc_write_bd(priv, entry, &bd);

	if (priv->cur_tx == (priv->dty_tx + priv->num_tx)) {
	        //printf("stopping queue\n");
		//netif_stop_queue(dev);
	}

	//spin_unlock_irq(&priv->lock);
	 portEXIT_CRITICAL(&lock_xmit_spinlock);
	//skb_tx_timestamp(skb);
out:
        //dev_kfree_skb(skb);
        //pbuf_free(skb);
        //printf("not needed pbuf_free? %p\n",skb);
        //out_no_free:
	return 0;
}

//
static int ethoc_set_ringparam(struct netif *dev)
{
	struct ethoc *priv = &priv_ethoc;

	priv->num_tx = 8;
	priv->num_rx = 8;
	priv->num_bd = 16;
	/* num_tx must be a power of two */
	//priv->num_tx = rounddown_pow_of_two(num_bd >> 1);
	//priv->num_rx = num_bd - priv->num_tx;

	printf("ethoc: num_tx: %d num_rx: %d\n",priv->num_tx, priv->num_rx);

	//priv->vma = devm_kzalloc(&pdev->dev, num_bd*sizeof(void *), GFP_KERNEL);

	return 0;
}


static void ethoc_do_set_mac_address(struct netif *dev)
{
	struct ethoc *priv = &priv_ethoc;
	unsigned char *mac =   (unsigned char *) &(dev->hwaddr[0]);


	ethoc_write(priv, MAC_ADDR0, (mac[2] << 24) | (mac[3] << 16) |
				     (mac[4] <<  8) | (mac[5] <<  0));
	ethoc_write(priv, MAC_ADDR1, (mac[0] <<  8) | (mac[1] <<  0));
}


#if 0

static int ethoc_stop(struct netif *dev)
{
	struct ethoc *priv = netdev_priv(dev);

	napi_disable(&priv->napi);

	if (dev->phydev)
		phy_stop(dev->phydev);

	ethoc_disable_rx_and_tx(priv);
	free_irq(dev->irq, dev);

	if (!netif_queue_stopped(dev))
		netif_stop_queue(dev);

	return 0;
}

static int ethoc_set_mac_address(struct netif *dev, void *p)
{
	const struct sockaddr *addr = p;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;
	memcpy(dev->dev_addr, addr->sa_data, ETH_ALEN);
	ethoc_do_set_mac_address(dev);
	return 0;
}

static void ethoc_set_multicast_list(struct netif *dev)
{
	struct ethoc *priv = netdev_priv(dev);
	u32 mode = ethoc_read(priv, MODER);
	struct netdev_hw_addr *ha;
	u32 hash[2] = { 0, 0 };

	/* set loopback mode if requested */
	if (dev->flags & IFF_LOOPBACK)
		mode |=  MODER_LOOP;
	else
		mode &= ~MODER_LOOP;

	/* receive broadcast frames if requested */
	if (dev->flags & IFF_BROADCAST)
		mode &= ~MODER_BRO;
	else
		mode |=  MODER_BRO;

	/* enable promiscuous mode if requested */
	if (dev->flags & IFF_PROMISC)
		mode |=  MODER_PRO;
	else
		mode &= ~MODER_PRO;

	ethoc_write(priv, MODER, mode);

	/* receive multicast frames */
	if (dev->flags & IFF_ALLMULTI) {
		hash[0] = 0xffffffff;
		hash[1] = 0xffffffff;
	} else {
		netdev_for_each_mc_addr(ha, dev) {
			u32 crc = ether_crc(ETH_ALEN, ha->addr);
			int bit = (crc >> 26) & 0x3f;
			hash[bit >> 5] |= 1 << (bit & 0x1f);
		}
	}

	ethoc_write(priv, ETH_HASH0, hash[0]);
	ethoc_write(priv, ETH_HASH1, hash[1]);
}

static int ethoc_change_mtu(struct netif *dev, int new_mtu)
{
	return -ENOSYS;
}

static void ethoc_tx_timeout(struct netif *dev)
{
	struct ethoc *priv = netdev_priv(dev);
	u32 pending = ethoc_read(priv, INT_SOURCE);
	if (likely(pending))
		ethoc_interrupt(dev->irq, dev);
}


static int ethoc_get_regs_len(struct netif *netdev)
{
	return ETH_END;
}

static void ethoc_get_regs(struct netif *dev, struct ethtool_regs *regs,
			   void *p)
{
	struct ethoc *priv = netdev_priv(dev);
	u32 *regs_buff = p;
	unsigned i;

	regs->version = 0;
	for (i = 0; i < ETH_END / sizeof(u32); ++i)
		regs_buff[i] = ethoc_read(priv, i * sizeof(u32));
}

static void ethoc_get_ringparam(struct netif *dev,
				struct ethtool_ringparam *ring)
{
	struct ethoc *priv = netdev_priv(dev);

	ring->rx_max_pending = priv->num_bd - 1;
	ring->rx_mini_max_pending = 0;
	ring->rx_jumbo_max_pending = 0;
	ring->tx_max_pending = priv->num_bd - 1;

	ring->rx_pending = priv->num_rx;
	ring->rx_mini_pending = 0;
	ring->rx_jumbo_pending = 0;
	ring->tx_pending = priv->num_tx;
}


/**
 * ethoc_probe - initialize OpenCores ethernet MAC
 * pdev:	platform device
 */
static int ethoc_probe(struct platform_device *pdev)
{
	struct netif *netdev = NULL;
	struct resource *res = NULL;
	struct resource *mmio = NULL;
	struct resource *mem = NULL;
	struct ethoc *priv = NULL;
	int num_bd;
	int ret = 0;
	bool random_mac = false;
	struct ethoc_platform_data *pdata = dev_get_platdata(&pdev->dev);
	u32 eth_clkfreq = pdata ? pdata->eth_clkfreq : 0;

	/* allocate networking device */
	netdev = alloc_etherdev(sizeof(struct ethoc));
	if (!netdev) {
		ret = -ENOMEM;
		goto out;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);
	platform_set_drvdata(pdev, netdev);

	/* obtain I/O memory space */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot obtain I/O memory space\n");
		ret = -ENXIO;
		goto free;
	}

	mmio = devm_request_mem_region(&pdev->dev, res->start,
			resource_size(res), res->name);
	if (!mmio) {
		dev_err(&pdev->dev, "cannot request I/O memory space\n");
		ret = -ENXIO;
		goto free;
	}

	netdev->base_addr = mmio->start;

	/* obtain buffer memory space */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {
		mem = devm_request_mem_region(&pdev->dev, res->start,
			resource_size(res), res->name);
		if (!mem) {
			dev_err(&pdev->dev, "cannot request memory space\n");
			ret = -ENXIO;
			goto free;
		}

		netdev->mem_start = mem->start;
		netdev->mem_end   = mem->end;
	}


	/* obtain device IRQ number */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot obtain IRQ\n");
		ret = -ENXIO;
		goto free;
	}

	netdev->irq = res->start;

	/* setup driver-private data */
	priv = netdev_priv(netdev);
	priv->netdev = netdev;
	priv->dma_alloc = 0;
	priv->io_region_size = resource_size(mmio);

	priv->iobase = devm_ioremap_nocache(&pdev->dev, netdev->base_addr,
			resource_size(mmio));
	if (!priv->iobase) {
		dev_err(&pdev->dev, "cannot remap I/O memory space\n");
		ret = -ENXIO;
		goto free;
	}

	if (netdev->mem_end) {
		priv->membase = devm_ioremap_nocache(&pdev->dev,
			netdev->mem_start, resource_size(mem));
		if (!priv->membase) {
			dev_err(&pdev->dev, "cannot remap memory space\n");
			ret = -ENXIO;
			goto free;
		}
	} else {
		/* Allocate buffer memory */
		priv->membase = dmam_alloc_coherent(&pdev->dev,
			buffer_size, (void *)&netdev->mem_start,
			GFP_KERNEL);
		if (!priv->membase) {
			dev_err(&pdev->dev, "cannot allocate %dB buffer\n",
				buffer_size);
			ret = -ENOMEM;
			goto free;
		}
		netdev->mem_end = netdev->mem_start + buffer_size;
		priv->dma_alloc = buffer_size;
	}

	priv->big_endian = pdata ? pdata->big_endian :
		of_device_is_big_endian(pdev->dev.of_node);

	/* calculate the number of TX/RX buffers, maximum 128 supported */
	num_bd = min_t(unsigned int,
		128, (netdev->mem_end - netdev->mem_start + 1) / ETHOC_BUFSIZ);
	if (num_bd < 4) {
		ret = -ENODEV;
		goto free;
	}
	priv->num_bd = num_bd;
	/* num_tx must be a power of two */
	priv->num_tx = rounddown_pow_of_two(num_bd >> 1);
	priv->num_rx = num_bd - priv->num_tx;

	dev_dbg(&pdev->dev, "ethoc: num_tx: %d num_rx: %d\n",
		priv->num_tx, priv->num_rx);

	priv->vma = devm_kzalloc(&pdev->dev, num_bd*sizeof(void *), GFP_KERNEL);
	if (!priv->vma) {
		ret = -ENOMEM;
		goto free;
	}

	/* Allow the platform setup code to pass in a MAC address. */
	if (pdata) {
		memcpy(netdev->dev_addr, pdata->hwaddr, IFHWADDRLEN);
		priv->phy_id = pdata->phy_id;
	} else {
		const uint8_t *mac;

		mac = of_get_property(pdev->dev.of_node,
				      "local-mac-address",
				      NULL);
		if (mac)
			memcpy(netdev->dev_addr, mac, IFHWADDRLEN);
		priv->phy_id = -1;
	}

	/* Check that the given MAC address is valid. If it isn't, read the
	 * current MAC from the controller.
	 */
	if (!is_valid_ether_addr(netdev->dev_addr))
		ethoc_get_mac_address(netdev, netdev->dev_addr);

	/* Check the MAC again for validity, if it still isn't choose and
	 * program a random one.
	 */
	if (!is_valid_ether_addr(netdev->dev_addr)) {
		eth_random_addr(netdev->dev_addr);
		random_mac = true;
	}

	ethoc_do_set_mac_address(netdev);

	if (random_mac)
		netdev->addr_assign_type = NET_ADDR_RANDOM;

	/* Allow the platform setup code to adjust MII management bus clock. */
	if (!eth_clkfreq) {
		struct clk *clk = devm_clk_get(&pdev->dev, NULL);

		if (!IS_ERR(clk)) {
			priv->clk = clk;
			clk_prepare_enable(clk);
			eth_clkfreq = clk_get_rate(clk);
		}
	}
	if (eth_clkfreq) {
		u32 clkdiv = MIIMODER_CLKDIV(eth_clkfreq / 2500000 + 1);

		if (!clkdiv)
			clkdiv = 2;
		dev_dbg(&pdev->dev, "setting MII clkdiv to %u\n", clkdiv);
		ethoc_write(priv, MIIMODER,
			    (ethoc_read(priv, MIIMODER) & MIIMODER_NOPRE) |
			    clkdiv);
	}

	/* register MII bus */
	priv->mdio = mdiobus_alloc();
	if (!priv->mdio) {
		ret = -ENOMEM;
		goto free2;
	}

	priv->mdio->name = "ethoc-mdio";
	snprintf(priv->mdio->id, MII_BUS_ID_SIZE, "%s-%d",
			priv->mdio->name, pdev->id);
	priv->mdio->read = ethoc_mdio_read;
	priv->mdio->write = ethoc_mdio_write;
	priv->mdio->priv = priv;

	ret = mdiobus_register(priv->mdio);
	if (ret) {
		dev_err(&netdev->dev, "failed to register MDIO bus\n");
		goto free2;
	}

	ret = ethoc_mdio_probe(netdev);
	if (ret) {
		dev_err(&netdev->dev, "failed to probe MDIO bus\n");
		goto error;
	}

	/* setup the netif structure */
	netdev->netdev_ops = &ethoc_netdev_ops;
	netdev->watchdog_timeo = ETHOC_TIMEOUT;
	netdev->features |= 0;
	netdev->ethtool_ops = &ethoc_ethtool_ops;

	/* setup NAPI */
	netif_napi_add(netdev, &priv->napi, ethoc_poll, 64);

	spin_lock_init(&priv->lock);

	ret = register_netdev(netdev);
	if (ret < 0) {
		dev_err(&netdev->dev, "failed to register interface\n");
		goto error2;
	}

	goto out;

error2:
	netif_napi_del(&priv->napi);
error:
	mdiobus_unregister(priv->mdio);
	mdiobus_free(priv->mdio);
free2:
	if (priv->clk)
		clk_disable_unprepare(priv->clk);
free:
	free_netdev(netdev);
out:
	return ret;
}


#endif

err_t ethoc_output(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
   //printf("sending message to etharp_output\n");

   etharp_output(netif,q,ipaddr);

   return ERR_OK;
}



/*
 * ethernetif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
err_t ethoc_init(struct netif *netif)
{
  struct ethoc_if *ethoc_if;
  struct ethoc *priv = &priv_ethoc;


  ethoc_if = mem_malloc(sizeof(struct ethoc_if));//MAC Address
  
  if (ethoc_if == NULL)
  {
  		LWIP_DEBUGF(NETIF_DEBUG,("ethoc_init: out of memory!\n"));
  		return ERR_MEM;
  }
  
  netif->state = ethoc_if;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->output = ethoc_output;
  netif->linkoutput = ethoc_low_level_output;

  sprintf(hostname, "esp_qemu");
  netif->hostname = hostname;
  
  ethoc_if->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  // TODO, move this to a more logical place..
  priv->netdev=netif;
  low_level_init(netif);
  
  etharp_init();
  
  // Timeout to arp alsoadded elsewhere??
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
 * Initialize the ethoc ethernet chip, resetting the interface and getting the ethernet
 * address.
 */
static void low_level_init(struct netif * netif)
{
	//struct ethoc_if *ethoc_if;	
	//ethoc_if = netif->state;
	
	netif->hwaddr_len=6;
	netif->mtu = 1500;	
  	//netif->flags = NETIF_FLAG_BROADCAST;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
  		
		  // 24:0a:c4:00:c8:70
		  
    netif->hwaddr[0]=0x24;
    netif->hwaddr[1]=0x0a;
    netif->hwaddr[2]=0xc4;
    netif->hwaddr[3]=0x00;
    netif->hwaddr[4]=0xc8;
    netif->hwaddr[5]=0x70;
/*
    netif->hwaddr[0]=0x70;
    netif->hwaddr[1]=0xc8;
    netif->hwaddr[2]=0x00;
    netif->hwaddr[3]=0xc4;
    netif->hwaddr[4]=0x0a;
    netif->hwaddr[5]=0x24;
*/
	// ---------- start -------------
	ethoc_set_ringparam(netif);
	ethoc_open(netif);              
	ethoc_do_set_mac_address(netif);

	// TODO , set MAC

    //static void IRAM_ATTR frc_timer_isr()
	// Interrupt source not important for qemu

	//intr_matrix_set(xPortGetCoreID(), ETS_RWBT_INTR_SOURCE, 8);
    //xt_set_interrupt_handler(9, &ethoc_interrupt, NULL);                                                           
    //xt_ints_on(1 << 8);                     
	
	// memcpy 0x3FFE_8000~0x3FFE_FFFF to 
	// 0x4000_0000  0x4000_7FFF
	// Normal  0x400B_0000~0x400B_7FFF
    // New range 0x3FFE_8000~0x3FFE_FFFF
 	ethoc_if_netif = netif;
	// qemu fake interrupt test.
	//unsigned int*test=(unsigned int *) 0x3ff00092;  // (unsigned int *)0x3ff4ffff;
	//*test=0xffff;
	
}


/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/

/*
 * ethoc_low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */

struct pbuf *q;
u16_t Count;  // packetLength
u8_t *buf;
u16_t pbuf_x_len = 0;
struct pbuf *tmp;

static err_t ethoc_low_level_output(struct netif * netif, struct pbuf *p)
{
	
	//packetLength = p->tot_len - ETH_PAD_SIZE; //05 01 millin    
	//if ((packetLength) < 64) packetLength = 64; //add pad by the AX88796 automatically

	//printf("ethoc_low_level_output %d\n",p->tot_len);
	/*
	 * Write packet to ring buffers.
	 */
   // From wlanif.c , looks strange, however
   // Note that LWIP_NETIF_TX_SINGLE_PBUF is set to 1..
   // @todo: TCP and IP-frag do not work with this, yet 
    q = p;
    pbuf_x_len = q->len;
    if(q->next !=NULL)
    {
		//printf("Assembling packet, this should not happen if LWIP_NETIF_TX_SINGLE_PBUF is set");
        //char cnt = 0;
        tmp = q->next;
        while(tmp != NULL)
        {
            memcpy( (u8_t *)( (u8_t *)(q->payload) + pbuf_x_len), (u8_t *)tmp->payload , tmp->len );
            pbuf_x_len += tmp->len;
            //cnt++;
            tmp = tmp->next;
        }
    }

	ethoc_start_xmit(q,netif);

	#if LINK_STATS
		lwip_stats.link.xmit++;
	#endif /* LINK_STATS */
		
	return ERR_OK;
}


/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/

#if 0
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
ethoc_input(struct netif *netif)
{
  struct ethoc_if *ethoc_if;
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  ethoc_if = netif->state;
  
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
		// Declared static, no header
    	//etharp_ip_input(netif,ethoc_if->ethaddr,p);
    	/* skip Ethernet header */
		// This does not look right... 
    	//pbuf_header(p, -(14+ETH_PAD_SIZE));
    	/* pass to network layer */
    	netif->input(p, netif);
    	break;
  case ETHTYPE_ARP:
	    /* pass p to ARP module */
   		etharp_arp_input(netif, ethoc_if->ethaddr, p);
    	break;
  default:
		pbuf_free(p);
		p = NULL;
		break;
  }
}
#endif

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

/*----------------------------------------------------------------------------------------
  ****************************************************************************************
  ----------------------------------------------------------------------------------------*/


