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

#ifndef _NE2K_H_
#define _NE2K_H_

#include "lwip/netif.h"


#define     MIN_PACKET_SIZE 60      	/* smallest legal size packet, no fcs    */
#define     MAX_PACKET_SIZE 1514		/* largest legal size packet, no fcs     */


#define		DELAY							0x590b2  //0.5s test by ming
#define		DELAY_2S						0xbf000  //2s test 
#define     DELAY_MS						0x38F4   //20ms test 


/**
 *  Driver functions.
 */
err_t ne2k_init(struct netif *netif);
static void low_level_init(struct netif * netif);
static void arp_timer(void *arg);

static err_t low_level_output(struct netif * netif,struct pbuf *p);
u16_t write_AX88796(u8_t * buf, u16_t remote_Addr, u16_t Count);

static void ne2k_input(struct netif *netif);
static struct pbuf * low_level_input(struct netif *netif);
u16_t read_AX88796(u8_t * buf, u16_t remote_Addr, u16_t Count);


// Open core registers
#define	OC_BASE    0x3FF76000

/* register offsets */
#define	MODER		0x00
#define	INT_SOURCE	0x04
#define	INT_MASK	0x08
#define	IPGT		0x0c
#define	IPGR1		0x10
#define	IPGR2		0x14
#define	PACKETLEN	0x18
#define	COLLCONF	0x1c
#define	TX_BD_NUM	0x20
#define	CTRLMODER	0x24
#define	MIIMODER	0x28
#define	MIICOMMAND	0x2c
#define	MIIADDRESS	0x30
#define	MIITX_DATA	0x34
#define	MIIRX_DATA	0x38
#define	MIISTATUS	0x3c
#define	MAC_ADDR0	0x40
#define	MAC_ADDR1	0x44
#define	ETH_HASH0	0x48
#define	ETH_HASH1	0x4c
#define	ETH_TXCTRL	0x50
#define	ETH_END		0x54


#define     EN0_MIISTATUS     *(unsigned char *)(OC_BASE+0x3c)	/*  WR Starting page of ring buffer      */



/* mode register */
#define	MODER_RXEN	(1 <<  0) /* receive enable */
#define	MODER_TXEN	(1 <<  1) /* transmit enable */
#define	MODER_NOPRE	(1 <<  2) /* no preamble */
#define	MODER_BRO	(1 <<  3) /* broadcast address */
#define	MODER_IAM	(1 <<  4) /* individual address mode */
#define	MODER_PRO	(1 <<  5) /* promiscuous mode */
#define	MODER_IFG	(1 <<  6) /* interframe gap for incoming frames */
#define	MODER_LOOP	(1 <<  7) /* loopback */
#define	MODER_NBO	(1 <<  8) /* no back-off */
#define	MODER_EDE	(1 <<  9) /* excess defer enable */
#define	MODER_FULLD	(1 << 10) /* full duplex */
#define	MODER_RESET	(1 << 11) /* FIXME: reset (undocumented) */
#define	MODER_DCRC	(1 << 12) /* delayed CRC enable */
#define	MODER_CRC	(1 << 13) /* CRC enable */
#define	MODER_HUGE	(1 << 14) /* huge packets enable */
#define	MODER_PAD	(1 << 15) /* padding enabled */
#define	MODER_RSM	(1 << 16) /* receive small packets */

/* interrupt source and mask registers */
#define	INT_MASK_TXF	(1 << 0) /* transmit frame */
#define	INT_MASK_TXE	(1 << 1) /* transmit error */
#define	INT_MASK_RXF	(1 << 2) /* receive frame */
#define	INT_MASK_RXE	(1 << 3) /* receive error */
#define	INT_MASK_BUSY	(1 << 4)
#define	INT_MASK_TXC	(1 << 5) /* transmit control frame */
#define	INT_MASK_RXC	(1 << 6) /* receive control frame */

#define	INT_MASK_TX	(INT_MASK_TXF | INT_MASK_TXE)
#define	INT_MASK_RX	(INT_MASK_RXF | INT_MASK_RXE)

#define	INT_MASK_ALL ( \
		INT_MASK_TXF | INT_MASK_TXE | \
		INT_MASK_RXF | INT_MASK_RXE | \
		INT_MASK_TXC | INT_MASK_RXC | \
		INT_MASK_BUSY \
	)

/* packet length register */
#define	PACKETLEN_MIN(min)		(((min) & 0xffff) << 16)
#define	PACKETLEN_MAX(max)		(((max) & 0xffff) <<  0)
#define	PACKETLEN_MIN_MAX(min, max)	(PACKETLEN_MIN(min) | \
					PACKETLEN_MAX(max))

/* transmit buffer number register */
#define	TX_BD_NUM_VAL(x)	(((x) <= 0x80) ? (x) : 0x80)

/* control module mode register */
#define	CTRLMODER_PASSALL	(1 << 0) /* pass all receive frames */
#define	CTRLMODER_RXFLOW	(1 << 1) /* receive control flow */
#define	CTRLMODER_TXFLOW	(1 << 2) /* transmit control flow */

/* MII mode register */
#define	MIIMODER_CLKDIV(x)	((x) & 0xfe) /* needs to be an even number */
#define	MIIMODER_NOPRE		(1 << 8) /* no preamble */

/* MII command register */
#define	MIICOMMAND_SCAN		(1 << 0) /* scan status */
#define	MIICOMMAND_READ		(1 << 1) /* read status */
#define	MIICOMMAND_WRITE	(1 << 2) /* write control data */

/* MII address register */
#define	MIIADDRESS_FIAD(x)		(((x) & 0x1f) << 0)
#define	MIIADDRESS_RGAD(x)		(((x) & 0x1f) << 8)
#define	MIIADDRESS_ADDR(phy, reg)	(MIIADDRESS_FIAD(phy) | \
					MIIADDRESS_RGAD(reg))

/* MII transmit data register */
#define	MIITX_DATA_VAL(x)	((x) & 0xffff)

/* MII receive data register */
#define	MIIRX_DATA_VAL(x)	((x) & 0xffff)

/* MII status register */
#define	MIISTATUS_LINKFAIL	(1 << 0)
#define	MIISTATUS_BUSY		(1 << 1)
#define	MIISTATUS_INVALID	(1 << 2)

/* TX buffer descriptor */
#define	TX_BD_CS		(1 <<  0) /* carrier sense lost */
#define	TX_BD_DF		(1 <<  1) /* defer indication */
#define	TX_BD_LC		(1 <<  2) /* late collision */
#define	TX_BD_RL		(1 <<  3) /* retransmission limit */
#define	TX_BD_RETRY_MASK	(0x00f0)
#define	TX_BD_RETRY(x)		(((x) & 0x00f0) >>  4)
#define	TX_BD_UR		(1 <<  8) /* transmitter underrun */
#define	TX_BD_CRC		(1 << 11) /* TX CRC enable */
#define	TX_BD_PAD		(1 << 12) /* pad enable for short packets */
#define	TX_BD_WRAP		(1 << 13)
#define	TX_BD_IRQ		(1 << 14) /* interrupt request enable */
#define	TX_BD_READY		(1 << 15) /* TX buffer ready */
#define	TX_BD_LEN(x)		(((x) & 0xffff) << 16)
#define	TX_BD_LEN_MASK		(0xffff << 16)

#define	TX_BD_STATS		(TX_BD_CS | TX_BD_DF | TX_BD_LC | \
				TX_BD_RL | TX_BD_RETRY_MASK | TX_BD_UR)

/* RX buffer descriptor */
#define	RX_BD_LC	(1 <<  0) /* late collision */
#define	RX_BD_CRC	(1 <<  1) /* RX CRC error */
#define	RX_BD_SF	(1 <<  2) /* short frame */
#define	RX_BD_TL	(1 <<  3) /* too long */
#define	RX_BD_DN	(1 <<  4) /* dribble nibble */
#define	RX_BD_IS	(1 <<  5) /* invalid symbol */
#define	RX_BD_OR	(1 <<  6) /* receiver overrun */
#define	RX_BD_MISS	(1 <<  7)
#define	RX_BD_CF	(1 <<  8) /* control frame */
#define	RX_BD_WRAP	(1 << 13)
#define	RX_BD_IRQ	(1 << 14) /* interrupt request enable */
#define	RX_BD_EMPTY	(1 << 15)
#define	RX_BD_LEN(x)	(((x) & 0xffff) << 16)

#define	RX_BD_STATS	(RX_BD_LC | RX_BD_CRC | RX_BD_SF | RX_BD_TL | \
			RX_BD_DN | RX_BD_IS | RX_BD_OR | RX_BD_MISS)

#define	ETHOC_BUFSIZ		1536
#define	ETHOC_ZLEN		64
#define	ETHOC_BD_BASE		0x400
#define	ETHOC_TIMEOUT		(HZ / 2)
#define	ETHOC_MII_TIMEOUT	(1 + (HZ / 5))



/*----------------------------------------
* Register header of NE2000 chip
*----------------------------------------*/
#define Base_ADDR           0xA0000200 ///CE2 space of DSK is 0xA0000000 
									 // and ethernet chip is at 0x200 by default

// actual address on DSK
#define     EN_CMD          *(unsigned char *)(Base_ADDR+0x00)  /*	The command register (for all pages) */
#define     EN_DATA		    *(unsigned short *)(Base_ADDR+0x10)	/*by ming (change to 16bit)  Remote DMA Port10~17h (for all pages)*/
#define     EN_RESET	    *(unsigned char *)(Base_ADDR+0x1F)	/*  Reset Port 1fh(for all pages)     */

/* Page 0 register offsets   */
#define     EN0_STARTPG     *(unsigned char *)(Base_ADDR+0x01)	/*  WR Starting page of ring buffer      */
#define     EN0_STOPPG  	*(unsigned char *)(Base_ADDR+0x02)	/*  WR Ending page +1 of ring buffer     */
#define     EN0_BOUNDARY	*(unsigned char *)(Base_ADDR+0x03)	/*  RD/WR Boundary page of ring buffer   */
#define     EN0_TSR			*(unsigned char *)(Base_ADDR+0x04)	/*  RD Transmit status reg               */
#define     EN0_TPSR		*(unsigned char *)(Base_ADDR+0x04)	/*  WR Transmit starting page            */
#define     EN0_NCR			*(unsigned char *)(Base_ADDR+0x05)	/*  RD Number of collision reg           */
#define     EN0_TCNTLO  	*(unsigned char *)(Base_ADDR+0x05)	/*  WR Low  byte of tx byte count        */
#define     EN0_CRP			*(unsigned char *)(Base_ADDR+0x06)	/*  Current Page Register                              */
#define     EN0_TCNTHI		*(unsigned char *)(Base_ADDR+0x06)	/*  WR High byte of tx byte count        */
#define     EN0_ISR			*(unsigned char *)(Base_ADDR+0x07)	/*  RD/WR Interrupt status reg           */
#define     EN0_CRDALO  	*(unsigned char *)(Base_ADDR+0x08)	/*  RD low byte of current remote dma add*/
#define     EN0_RSARLO		*(unsigned char *)(Base_ADDR+0x08)	/*  WR Remote start address reg 0        */
#define     EN0_CRDAHI		*(unsigned char *)(Base_ADDR+0x09)	/*  RD high byte, current remote dma add.*/
#define     EN0_RSARHI		*(unsigned char *)(Base_ADDR+0x09)	/*  WR Remote start address reg 1        */
#define     EN0_RCNTLO	    *(unsigned char *)(Base_ADDR+0x0A)	/*  WR Remote byte count reg 0           */
#define     EN0_RCNTHI		*(unsigned char *)(Base_ADDR+0x0B)	/*  WR Remote byte count reg 1           */
#define     EN0_RSR			*(unsigned char *)(Base_ADDR+0x0C)	/*  RD RX status reg                     */
#define     EN0_RXCR		*(unsigned char *)(Base_ADDR+0x0C)	/*  WR RX configuration reg              */
#define     EN0_TXCR		*(unsigned char *)(Base_ADDR+0x0D)	/*  WR TX configuration reg              */
#define     EN0_DCFG		*(unsigned char *)(Base_ADDR+0x0E)	/*  WR Data configuration reg            */
#define     EN0_IMR			*(unsigned char *)(Base_ADDR+0x0F)	/*  WR Interrupt mask reg                */

/* Page 1 register offsets    */
#define     EN1_PAR0	    *(unsigned char *)(Base_ADDR+0x01)	/* RD/WR This board's physical ethernet addr */
#define     EN1_PAR1	    *(unsigned char *)(Base_ADDR+0x02)
#define     EN1_PAR2	    *(unsigned char *)(Base_ADDR+0x03)
#define     EN1_PAR3	    *(unsigned char *)(Base_ADDR+0x04)
#define     EN1_PAR4	    *(unsigned char *)(Base_ADDR+0x05)
#define     EN1_PAR5	    *(unsigned char *)(Base_ADDR+0x06)
#define     EN1_CURR	    *(unsigned char *)(Base_ADDR+0x07)   /* RD/WR current page reg */
#define		EN1_CURPAG		EN1_CURR
#define     EN1_MAR0        *(unsigned char *)(Base_ADDR+0x08)   /* RD/WR Multicast filter mask array (8 bytes) */
#define     EN1_MAR1		*(unsigned char *)(Base_ADDR+0x09)
#define     EN1_MAR2        *(unsigned char *)(Base_ADDR+0x0A)
#define     EN1_MAR3        *(unsigned char *)(Base_ADDR+0x0B)
#define     EN1_MAR4        *(unsigned char *)(Base_ADDR+0x0C)
#define     EN1_MAR5        *(unsigned char *)(Base_ADDR+0x0D)
#define     EN1_MAR6        *(unsigned char *)(Base_ADDR+0x0E)
#define     EN1_MAR7        *(unsigned char *)(Base_ADDR+0x0F)

/* Command Values at EN_CMD */
#define     EN_STOP		    0x01	/*  Stop and reset the chip              */
#define     EN_START	    0x02	/*  Start the chip, clear reset          */
#define     EN_TRANS	    0x04	/*  Transmit a frame                     */
#define     EN_RREAD	    0x08	/*  Remote read                          */
#define     EN_RWRITE	    0x10	/*  Remote write                         */
#define     EN_NODMA	    0x20	/*  Remote DMA                           */
#define     EN_PAGE0	    0x00	/*  Select page chip registers           */
#define     EN_PAGE1	    0x40	/*  using the two high-order bits        */


//---------------------------------
// Values for Ring-Buffer setting
//---------------------------------

#define     NE_START_PG	    0x40     	/* First page of TX buffer           */
#define     NE_STOP_PG	    0x80		/* Last page + 1 of RX Ring          */ 

#define     TX_PAGES	    6       
#define	    TX_START_PG		NE_START_PG	//0x40

#define     RX_START_PG	    NE_START_PG + TX_PAGES //0x46
#define     RX_CURR_PG      RX_START_PG + 1		   //0x47
#define     RX_STOP_PG      NE_STOP_PG  //0x80




/* Bits in EN0_ISR - Interrupt status register        (RD WR)                */
#define     ENISR_RX   		0x01	/*  Receiver, no error                   */
#define     ENISR_TX	    0x02	/*  Transceiver, no error                */
#define     ENISR_RX_ERR	0x04	/*  Receiver, with error 				 */
									//�������ݰ�����������������BNRY��CURR������ 
#define     ENISR_TX_ERR	0x08	/*  Transmitter, with error              */
									//���ڳ�ͻ�������࣬���ͳ��������ط�����
#define     ENISR_OVER	    0x10	/*  Receiver overwrote the ring          */
                       				/*  Gap area of receiver ring buffer was disappeared  */ 
                       				//�����ڴ������������������������������ֲᡣ
#define     ENISR_COUNTERS	0x20	/*  Counters need emptying               */
                                    /*  MSB of network tally counter became 1 */
                                    //�����������жϣ����ε���������IMR�Ĵ�������
#define     ENISR_RDC	    0x40	/*  remote dma complete                  */
									//���ε�����ѯ�ȴ�DMA������
#define     ENISR_RESET     0x80	/*  Reset completed                      */
									//����Reset�����ε���
#define     ENISR_ALL	    0x3f	/*  3f  Interrupts we will enable        */
                                	/*  RST RDC CNT OVW TXE RXE PTX PRX		 */


/* Bits in EN0_RXCR - RX configuration reg                                   */
//#define     ENRXCR_RXCONFIG 0x04 	/* EN0_RXCR: broadcasts,no multicast,errors */
#define     ENRXCR_RXCONFIG 0x00 	/* EN0_RXCR: only unicast */
#define     ENRXCR_CRC	    0x01	/*  Save error packets(admit)            */
#define     ENRXCR_RUNT	    0x02	/*  Accept runt pckt(below 64bytes)      */
#define     ENRXCR_BCST	    0x04	/*  Accept broadcasts when 1             */
#define     ENRXCR_MULTI	0x08	/*  Multicast (if pass filter) when 0    */
#define     ENRXCR_PROMP	0x10	/*  Promiscuous physical addresses when 1*/
									/* when 0,accept assigned PAR0~5 address */
#define     ENRXCR_MON	    0x20	/*  Monitor mode (no packets rcvd)       */


/* Bits in EN0_TXCR - TX configuration reg                                   */
#define     ENTXCR_TXCONFIG 0x00    /* Normal transmit mode                  */
#define     ENTXCR_CRC	    0x01	/*  inhibit CRC,do not append crc when 1 */
#define     ENTXCR_LOOP	    0x02	/*  set internal loopback mode     ?     */
#define     ENTXCR_LB01	    0x06	/*  encoded loopback control       ?     */
#define     ENTXCR_ATD	    0x08	/*  auto tx disable                      */
/* when 1, if specified multicast packet was received, disable transmit      */ 
#define     ENTXCR_OFST	    0x10	/*  collision offset enable              */
/* selection of collision algorithm. When 0, gererally back-off algorithm select */

#endif /* _NE2K_H_ */
