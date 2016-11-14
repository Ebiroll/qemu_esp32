/* 
 * File:   Microchip_eth.h
 * Author: vbannelier
 *
 * Created on 2 avril 2015, 10:49
 */

#ifndef MICROCHIP_ETH_H
#define	MICROCHIP_ETH_H


#include "GenericTypeDefs.h"

// =======================================================================
//   Network Addressing Options
// =======================================================================

/* Default Network Configuration
 *   These settings are only used if data is not found in EEPROM.
 *   To clear EEPROM, hold BUTTON0, reset the board, and continue
 *   holding until the LEDs flash.  Release, and reset again.
 */

#define MY_DEFAULT_MAC_BYTE1            (0x00)	// Use the default of
#define MY_DEFAULT_MAC_BYTE2            (0x04)	// 00-04-A3-00-00-00 if using
#define MY_DEFAULT_MAC_BYTE3            (0xA3)	// an ENCX24J600 or MRF24WB0M
#define MY_DEFAULT_MAC_BYTE4            (0x00)	// and wish to use the internal
#define MY_DEFAULT_MAC_BYTE5            (0x00)	// factory programmed MAC
#define MY_DEFAULT_MAC_BYTE6            (0x00)	// address instead.

#define MY_DEFAULT_IP_ADDR_BYTE1        (192ul)
#define MY_DEFAULT_IP_ADDR_BYTE2        (168ul)
#define MY_DEFAULT_IP_ADDR_BYTE3        (10ul)
#define MY_DEFAULT_IP_ADDR_BYTE4        (134ul)

#define MY_DEFAULT_MASK_BYTE1           (255ul)
#define MY_DEFAULT_MASK_BYTE2           (255ul)
#define MY_DEFAULT_MASK_BYTE3           (255ul)
#define MY_DEFAULT_MASK_BYTE4           (0ul)

#define MY_DEFAULT_GATE_BYTE1           (192ul)//169.254.1.1
#define MY_DEFAULT_GATE_BYTE2           (168ul)
#define MY_DEFAULT_GATE_BYTE3           (10ul)
#define MY_DEFAULT_GATE_BYTE4           (254ul)

#define MY_DEFAULT_PRIMARY_DNS_BYTE1	(192ul)  //169.254.1.1
#define MY_DEFAULT_PRIMARY_DNS_BYTE2	(168ul)
#define MY_DEFAULT_PRIMARY_DNS_BYTE3	(0ul)
#define MY_DEFAULT_PRIMARY_DNS_BYTE4	(1ul)

#define MY_DEFAULT_SECONDARY_DNS_BYTE1	(0ul)
#define MY_DEFAULT_SECONDARY_DNS_BYTE2	(0ul)
#define MY_DEFAULT_SECONDARY_DNS_BYTE3	(0ul)
#define MY_DEFAULT_SECONDARY_DNS_BYTE4	(0ul)

//See <<TCPIP Stack Help>>  P172
#define	ETH_CFG_LINK			0		// set to 1 if you need to config the link to specific following parameters
										// otherwise the default connection will be attempted
										// depending on the selected PHY
#define	ETH_CFG_AUTO		1		// use auto negotiation
#define	ETH_CFG_10		1		// use/advertise 10 Mbps capability
#define	ETH_CFG_100		1		// use/advertise 100 Mbps capability
#define	ETH_CFG_HDUPLEX		1		// use/advertise half duplex capability
#define	ETH_CFG_FDUPLEX		1		// use/advertise full duplex capability
#define	ETH_CFG_AUTO_MDIX	1		// use/advertise auto MDIX capability
#define	ETH_CFG_SWAP_MDIX	1		// use swapped MDIX. else normal MDIX

#define EMAC_TX_DESCRIPTORS		3		// number of the TX descriptors to be created
#define EMAC_RX_DESCRIPTORS		2		// number of the RX descriptors and RX buffers to be created

#define	EMAC_RX_BUFF_SIZE		1520	// size of a RX buffer. should be multiple of 16
										// this is the size of all receive buffers processed by the ETHC
										// The size should be enough to accomodate any network received packet
										// If the packets are larger, they will have to take multiple RX buffers
										// The current implementation does not handle this situation right now and the packet is discarded.

#define MAC_TX_BUFFER_SIZE			(1518ul)

// Structure to contain a MAC address
typedef struct __attribute__((__packed__))
{
    unsigned char v[6];
} MAC_ADDR;

// A generic structure representing the Ethernet header starting all Ethernet
// frames
typedef struct  __attribute__((aligned(2), packed))
{
	MAC_ADDR        DestMACAddr;
	MAC_ADDR        SourceMACAddr;
	WORD_VAL        Type;
} ETHER_HEADER;

typedef struct
{
	int		txBusy;										// busy flag
	unsigned int		dataBuff[(MAC_TX_BUFFER_SIZE+sizeof(ETHER_HEADER)+sizeof(int)-1)/sizeof(int)];	// actual data buffer
}sEthTxDcpt;	// TX buffer descriptor


typedef struct
{
    // TX buffers
    sEthTxDcpt     _TxDescriptors[EMAC_TX_DESCRIPTORS];	// the statically allocated TX buffers
    sEthTxDcpt*    _pTxCurrDcpt;			// the current TX buffer
    int          _TxLastDcptIx;		// the last TX descriptor used
    unsigned char*         _CurrWrPtr;		// the current write pointer
}TxBuffers;

typedef struct
{
    // RX buffers
	unsigned char	_RxBuffers[EMAC_RX_DESCRIPTORS][EMAC_RX_BUFF_SIZE];	// rx buffers for incoming data
	unsigned char*	_pRxCurrBuff;						// the current RX buffer
	unsigned short	_RxCurrSize;						// the current RX buffer size
	unsigned char*	_CurrRdPtr;						// the current read pointer
}RxBuffers;


int     MACInit(volatile TxBuffers *, unsigned char AppMACAddr[6]);
int     MACReady(volatile TxBuffers *T1);
void    MACSetCurretPtr(volatile TxBuffers *T1);
void    MACAddToBuffer(volatile TxBuffers *T1,unsigned char *buff,unsigned int dataLen);
void    MACSendPckt(volatile TxBuffers *T1,unsigned short dataLen);


int    _LinkReconfigure(void);
void   _TxAckCallback(void* pPktBuff, int buffIx, void* fParam);       // Eth tx buffer acnowledge function

#endif	/* MICROCHIP_ETH_H */

