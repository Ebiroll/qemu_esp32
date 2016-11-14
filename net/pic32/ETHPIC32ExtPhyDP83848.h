/*********************************************************************
 *
 *                   National DP83848 PHY definitions
 *
 *********************************************************************
 * FileName:        ETHPIC32ExtPhyDP83848.h
 * Dependencies:
 * Processor:       PIC32
 *
 * Complier:        MPLAB C32
 *                  MPLAB IDE
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 * Microchip Audio Library – PIC32 Software.
 * Copyright © 2008 Microchip Technology Inc.  All rights reserved.
 * 
 * Microchip licenses the Software for your use with Microchip microcontrollers
 * and Microchip digital signal controllers pursuant to the terms of the
 * Non-Exclusive Software License Agreement accompanying this Software.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION,
 * ANY WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS
 * FOR A PARTICULAR PURPOSE.
 * MICROCHIP AND ITS LICENSORS ASSUME NO RESPONSIBILITY FOR THE ACCURACY,
 * RELIABILITY OR APPLICATION OF THE SOFTWARE AND DOCUMENTATION.
 * IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED
 * UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH
 * OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL,
 * SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS
 * OR LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY,
 * SERVICES, OR ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED
 * TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *
 *$Id: $
 ********************************************************************/
#ifndef _NAT_DP83848C_H_

#define _NAT_DP83848C_H_

typedef enum
{
	/*
	// basic registers, accross all registers: 0-1
	PHY_REG_BMCON		= 0,
	PHY_REG_BMSTAT		= 1,
	// extended registers: 2-15
	PHY_REG_PHYID1		= 2,
	PHY_REG_PHYID2		= 3,
	PHY_REG_ANAD		= 4,
	PHY_REG_ANLPAD		= 5,
	PHY_REG_ANLPADNP	= 5,
	PHY_REG_ANEXP		= 6,
	PHY_REG_ANNPTR		= 7,
	PHY_REG_ANLPRNP		= 8,
	*/
	// specific vendor registers: 16-31
	PHY_REG_STAT		= 0x10,
	PHY_REG_MII_INT_CTRL	= 0x11,
	PHY_REG_MII_INT_STAT	= 0x12,
	PHY_REG_FALSE_CS_COUNT	= 0x14,
	PHY_REG_RXERR_COUNT	= 0x15,
	PHY_REG_PCS_CONFIG	= 0x16,
	PHY_REG_RMII_BYPASS	= 0x17,
	PHY_REG_LED_CTRL	= 0x18,
	PHY_REG_PHY_CTRL	= 0x19,
	PHY_REG_10BT_CTRL	= 0x1a,
	PHY_REG_TEST_CTRL	= 0x1b,
	PHY_REG_ENERGY_CTRL	= 0x1d,	
	
	//
	//PHY_REGISTERS		= 32	// total number of registers
}ePHY_VENDOR_REG;
// updated version of ePHY_REG


// vendor registers
//
typedef union {
  struct {    
    unsigned ELAST_BUF:2;
    unsigned RX_UNF_STS:1;
    unsigned RX_OVF_STS:1;
    unsigned RMII_REV1_0:1;
    unsigned RMII_MODE:1;
    unsigned :10;
  };
  struct {
    unsigned short w:16;
  };
} __RMIIBYPASSbits_t;	// reg 0x17: PHY_REG_RMII_BYPASS 
#define	_RMIIBYPASS_RMII_MODE_MASK		0x0020
#define	_RMIIBYPASS_RMII_REV1_0_MASK		0x0010
#define	_RMIIBYPASS_RX_OVF_STS_MASK		0x0008
#define	_RMIIBYPASS_RX_UNF_STS_MASK		0x0004
#define	_RMIIBYPASS_ELAST_BUF_MASK		0x0003




typedef union {
  struct {    
    unsigned PHYADDR:5;
    unsigned LED_CFG:2;
    unsigned BP_STRETCH:1;
    unsigned BIST_START:1;
    unsigned BIST_STATUS:1;
    unsigned PSR_15:1;
    unsigned BIST_FE:1;
    unsigned PAUSE_TX:1;
    unsigned PAUSE_RX:1;
    unsigned FORCE_MDIX:1;
    unsigned MDIX_EN:1;
  };
  struct {
    unsigned short w:16;
  };
} __PHYCTRLbits_t;	// reg 0x19: PHY_REG_PHY_CTRL
#define	_PHYCTRL_PHYADDR_MASK		0x001f
#define	_PHYCTRL_LED_CFG_MASK		0x0060
#define	_PHYCTRL_BP_STRETCH_MASK	0x0080
#define	_PHYCTRL_BIST_START_MASK	0x0100
#define	_PHYCTRL_BIST_STATUS_MASK	0x0200
#define	_PHYCTRL_PSR_15_MASK		0x0400
#define	_PHYCTRL_BIST_FE_MASK		0x0800
#define	_PHYCTRL_PAUSE_TX_MASK		0x1000
#define	_PHYCTRL_PAUSE_RX_MASK		0x2000
#define	_PHYCTRL_FORCE_MDIX_MASK	0x4000
#define	_PHYCTRL_MDIX_EN_MASK		0x8000






#endif	// _NAT_DP83848C_H_

