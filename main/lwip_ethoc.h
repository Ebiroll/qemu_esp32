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


//#define		DELAY							0x590b2  //0.5s test by ming
//#define		DELAY_2S						0xbf000  //2s test 
//#define     DELAY_MS						0x38F4   //20ms test 


// Open core registers
#define	OC_BASE          0x3ff69000
#define	OC_DESC_START    0x3ff69400
#define	OC_BUF_START     0x3FFF8000

// End of peripherals 0x3FF7_FFFF
// Data 0x3FF6_9000 0x3FF6_AFFF 8 KB   EMAC

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




#endif /* _NE2K_H_ */
