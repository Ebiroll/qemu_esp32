/*********************************************************************
 *
 *     PHY external API implementation for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        ETHPIC32ExtPhy.c
 * Dependencies:
 * Processor:       PIC32
 *
 * Complier:        MPLAB C32
 *                  MPLAB IDE
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright ?2009 Microchip Technology Inc.  All rights reserved.
 * 
 * Microchip licenses the Software for your use with Microchip microcontrollers
 * and Microchip digital signal controllers pursuant to the terms of the
 * Non-Exclusive Software License Agreement accompanying this Software.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED �AS IS?WITHOUT WARRANTY
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
 * $Id: $
 ********************************************************************/

#include <plib.h>

#include <HardwareProfile.h>
//#include LWIP_PORT_INCLUDE_PATH(include/netif/HardwareProfile.h)

// Compile only for PIC32MX with Ethernet MAC interface (must not have external ENCX24J600, ENC28J60, or MRF24WB0M hardware defined)
#if defined(__PIC32MX__) && defined(_ETH) && !defined(ENC100_INTERFACE_MODE) && !defined(ENC_CS_TRIS) && !defined(WF_CS_TRIS)

#include <netif/ETHPIC32ExtPhy.h>

#include <netif/ETHPIC32ExtPhyRegs.h>



// local definitions 
// 
#define	PROT_802_3	0x01	// IEEE 802.3 capability
#define	MAC_COMM_CPBL_MASK	(_BMSTAT_BASE10T_HDX_MASK|_BMSTAT_BASE10T_FDX_MASK|_BMSTAT_BASE100TX_HDX_MASK|_BMSTAT_BASE100TX_FDX_MASK)
// all comm capabilities our MAC supports



// local prototypes
// 

static void		_PhyInitIo(void);
static int		_PhyDetectReset(void);



// local data
// 
unsigned int		_PhyAdd;		// address of the PHY we're talking to



// local inlined functions
// 

static __inline__ eEthLinkStat __attribute__((always_inline)) _Phy2LinkStat(__BMSTATbits_t phyStat)
{
	eEthLinkStat	linkStat;
	linkStat=(phyStat.LINK_STAT)?ETH_LINK_ST_UP:ETH_LINK_ST_DOWN;
	if(phyStat.REM_FAULT)
	{
		linkStat|=ETH_LINK_ST_REMOTE_FAULT;
	}

	return linkStat;	
}

static __inline__ unsigned short __attribute__((always_inline)) _PhyReadReg( unsigned int rIx, unsigned int phyAdd )
{
    EthMIIMReadStart(rIx, phyAdd);
    return EthMIIMReadResult();
}


/****************************************************************************
 *                 interface functions
 ****************************************************************************/

/****************************************************************************
 * Function:        EthPhyRestartNegotiation
 *
 * PreCondition:    - EthPhyInit should have been called.
 *                  - The PHY should have been initialized with proper duplex/speed mode!
 *
 * Input:           None
 *
 * Output:          ETH_RES_OK for success,
 *                  ETH_RES_NEGOTIATION_UNABLE if the auto-negotiation is not supported.
 *
 * Side Effects:    None
 *
 * Overview:        This function restarts the PHY negotiation.
 *                  After this restart the link can be reconfigured.
 *                  The EthPhyGetNegotiationResults() can be used to see the outcoming result.
 *
 * Note:            None
 *****************************************************************************/
eEthRes  __attribute__((weak)) EthPhyRestartNegotiation(void)
{
	eEthRes	res;
	__BMSTATbits_t	phyCpbl;
	
	phyCpbl.w=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);
 
	if(phyCpbl.AN_ABLE)
	{	// ok, we can perform auto negotiation
		EthMIIMWriteStart(PHY_REG_BMCON, _PhyAdd, _BMCON_AN_ENABLE_MASK|_BMCON_AN_RESTART_MASK);	// restart negotiation and we'll have to wait
		res=ETH_RES_OK;
	}
	else
	{
		res=ETH_RES_NEGOTIATION_UNABLE;		// no negotiation ability!
	}

	return res;
}


/****************************************************************************
 * Function:        EthPhyGetHwConfigFlags
 *
 * PreCondition:    - EthPhyInit should have been called.
 *
 * Input:           None  
 *
 * Output:          a eEthPhyCfgFlags value
 *
 *
 * Side Effects:    None
 *
 * Overview:        This function returns the current PHY hardware MII/RMII and ALTERNATE/DEFAULT configuration flags.
 *
 * Note:            None
 *****************************************************************************/
eEthPhyCfgFlags __attribute__((weak)) EthPhyGetHwConfigFlags(void)
{
    eEthPhyCfgFlags hwFlags;
    // the way the hw is configured
    hwFlags=(DEVCFG3bits.FMIIEN!=0)?ETH_PHY_CFG_MII:ETH_PHY_CFG_RMII;
    hwFlags|=(DEVCFG3bits.FETHIO!=0)?ETH_PHY_CFG_DEFAULT:ETH_PHY_CFG_ALTERNATE;

    return hwFlags;
}

/****************************************************************************
 * Function:        EthPhyInit
 *
 * PreCondition:    - EthInit should have been called.
 *
 * Input:           oFlags - the requested open flags
 *                  cFlags - PHY MII/RMII configuration flags
 *                  pResFlags - address to store the initialization result  
 *
 * Output:          ETH_RES_OK for success,
 *                  an error code otherwise
 *
 *
 * Side Effects:    None
 *
 * Overview:        This function initializes the PHY communication.
 *                  It tries to detect the external PHY, to read the capabilties and find a match
 *                  with the requested features.
 *                  Then it programs the PHY accordingly.
 *
 * Note:            None
 *****************************************************************************/
eEthRes __attribute__((weak)) EthPhyInit(eEthOpenFlags oFlags, eEthPhyCfgFlags cFlags, eEthOpenFlags* pResFlags)
{
	unsigned short	ctrlReg;
	eEthPhyCfgFlags	hwFlags, swFlags;
	unsigned short	phyCpbl, openReqs, matchCpbl;
	eEthRes	res;

	// the way the hw is configured
    hwFlags=EthPhyGetHwConfigFlags();

	if(cFlags&ETH_PHY_CFG_AUTO)
	{
		cFlags=hwFlags;
	}
	else
	{	// some minimal check against the way the hw is configured
		swFlags=cFlags&(ETH_PHY_CFG_RMII|ETH_PHY_CFG_ALTERNATE);	
	
		if((swFlags ^ hwFlags)!=0)
		{	// hw-sw configuration mismatch MII/RMII, ALT/DEF config
			return ETH_RES_CFG_ERR;
		}
	}

	
	_PhyAdd=EthPhyMIIMAddress();		// get the PHY address


	if(oFlags&(ETH_OPEN_PHY_LOOPBACK|ETH_OPEN_MAC_LOOPBACK))
	{
		oFlags&=~ETH_OPEN_AUTO;	// no negotiation in loopback mode!
	}
	
	if(!(oFlags&ETH_OPEN_AUTO))
	{
		oFlags&=~ETH_OPEN_MDIX_AUTO;		// Auto-MDIX has to be in auto negotiation only
	}
	
    oFlags|=(cFlags&ETH_PHY_CFG_RMII)?ETH_OPEN_RMII:ETH_OPEN_MII;
    
	_PhyInitIo();	// init IO pins

	EthMIIMConfig(GetSystemClock(), EthPhyMIIMClock());


	// try to detect the PHY and reset it
	if(!_PhyDetectReset())
	{	// failed to detect the PHY
		return ETH_RES_DTCT_ERR; 
	}

	// provide some defaults
	if(!(oFlags&(ETH_OPEN_FDUPLEX|ETH_OPEN_HDUPLEX)))
	{
		oFlags|=ETH_OPEN_HDUPLEX;
	}
	if(!(oFlags&(ETH_OPEN_100|ETH_OPEN_10)))
	{
		oFlags|=ETH_OPEN_10;
	}
	
	if(oFlags&ETH_OPEN_AUTO)
	{	// advertise auto negotiation
		openReqs=_BMSTAT_AN_ABLE_MASK;

		if(oFlags&ETH_OPEN_100)
		{
			if(oFlags&ETH_OPEN_FDUPLEX)
			{
				openReqs|=_BMSTAT_BASE100TX_FDX_MASK;
			}
			if(oFlags&ETH_OPEN_HDUPLEX)
			{
				openReqs|=_BMSTAT_BASE100TX_HDX_MASK;
			}
		}
			
		if(oFlags&ETH_OPEN_10)
		{
			if(oFlags&ETH_OPEN_FDUPLEX)
			{
				openReqs|=_BMSTAT_BASE10T_FDX_MASK;
			}
			if(oFlags&ETH_OPEN_HDUPLEX)
			{
				openReqs|=_BMSTAT_BASE10T_HDX_MASK;
			}
		}		
	}
	else
	{	// no auto negotiation
		if(oFlags&ETH_OPEN_100)
		{
			openReqs=(oFlags&ETH_OPEN_FDUPLEX)?_BMSTAT_BASE100TX_FDX_MASK:_BMSTAT_BASE100TX_HDX_MASK;
		}
		else
		{
			openReqs=(oFlags&ETH_OPEN_FDUPLEX)?_BMSTAT_BASE10T_FDX_MASK:_BMSTAT_BASE10T_HDX_MASK;
		}
	}
	
	// try to match the oFlags with the PHY capabilities
	phyCpbl=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);
	matchCpbl=(openReqs&(MAC_COMM_CPBL_MASK|_BMSTAT_AN_ABLE_MASK))&phyCpbl;	// common features
	if(!(matchCpbl&MAC_COMM_CPBL_MASK))
	{	// no match?
		return ETH_RES_CPBL_ERR;
	}
	
	// we're ok, we can configure the PHY
	res=EthPhyConfigureMII(cFlags);
	if(res!=ETH_RES_OK)
	{
		return res;
	}
	
	res=EthPhyConfigureMdix(oFlags);
	if(res!=ETH_RES_OK)
	{
		return res;
	}
		
	if(matchCpbl&_BMSTAT_AN_ABLE_MASK)
	{	// ok, we can perform auto negotiation
		unsigned short	anadReg;

		anadReg=(((matchCpbl>>_BMSTAT_NEGOTIATION_POS)<<_ANAD_NEGOTIATION_POS)&_ANAD_NEGOTIATION_MASK)|PROT_802_3;
		if(ETH_MAC_PAUSE_CPBL_MASK & ETH_MAC_PAUSE_TYPE_PAUSE)
		{
			anadReg|=_ANAD_PAUSE_MASK;
		}
		if(ETH_MAC_PAUSE_CPBL_MASK& ETH_MAC_PAUSE_TYPE_ASM_DIR)
		{
			anadReg|=_ANAD_ASM_DIR_MASK;
		}

		EthMIIMWriteStart(PHY_REG_ANAD, _PhyAdd, anadReg);		// advertise our capabilities
						
		EthPhyRestartNegotiation();	// restart negotiation and we'll have to wait
	}
	else
	{	// ok, just don't use negotiation

		ctrlReg=0;
		if(matchCpbl&(_BMSTAT_BASE100TX_HDX_MASK|_BMSTAT_BASE100TX_FDX_MASK))	// set 100Mbps request/capability
		{
			ctrlReg|=_BMCON_SPEED_MASK;
		}

		if(matchCpbl&(_BMSTAT_BASE10T_FDX_MASK|_BMSTAT_BASE100TX_FDX_MASK))
		{
			ctrlReg|=_BMCON_DUPLEX_MASK;
		}
		
		if(oFlags&ETH_OPEN_PHY_LOOPBACK)
		{
			ctrlReg|=_BMCON_LOOPBACK_MASK;
		}
		
		EthMIIMWriteStart(PHY_REG_BMCON, _PhyAdd, ctrlReg);	// update the configuration
	}


	// now update the open flags
	// the upper layer needs to know the PHY set-up to further set-up the MAC.
	
	// clear the capabilities	
	oFlags&=~(ETH_OPEN_AUTO|ETH_OPEN_FDUPLEX|ETH_OPEN_HDUPLEX|ETH_OPEN_100|ETH_OPEN_10);

	if(matchCpbl&_BMSTAT_AN_ABLE_MASK)
	{
		oFlags|=ETH_OPEN_AUTO;
	}
	if(matchCpbl&(_BMSTAT_BASE100TX_HDX_MASK|_BMSTAT_BASE100TX_FDX_MASK))	// set 100Mbps request/capability
	{
		oFlags|=ETH_OPEN_100;
	}
	if(matchCpbl&(_BMSTAT_BASE10T_HDX_MASK|_BMSTAT_BASE10T_FDX_MASK))	// set 10Mbps request/capability
	{
		oFlags|=ETH_OPEN_10;
	}
	if(matchCpbl&(_BMSTAT_BASE10T_FDX_MASK|_BMSTAT_BASE100TX_FDX_MASK))
	{
		oFlags|=ETH_OPEN_FDUPLEX;
	}
	if(matchCpbl&(_BMSTAT_BASE10T_HDX_MASK|_BMSTAT_BASE100TX_HDX_MASK))
	{
		oFlags|=ETH_OPEN_HDUPLEX;
	}

	*pResFlags=oFlags;		// upper layer needs to know the PHY set-up to further set-up the MAC.

	return ETH_RES_OK;	

}


/****************************************************************************
 * Function:        EthPhyNegotiationComplete
 *
 * PreCondition:    EthPhyInit (and EthPhyRestartNegotiation) should have been called.
 *
 * Input:           waitComplete - if wait for completion needed
 *
 * Output:          ETH_RES_OK if negotiation done,
 *                  ETH_RES_NEGOTIATION_INACTIVE if no negotiation in progress
 *                  ETH_RES_NEGOTIATION_NOT_STARTED if negotiation not started yet (means tmo if waitComplete was requested)
 *                  ETH_RES_NEGOTIATION_ACTIVE if negotiation ongoing (means tmo if waitComplete was requested)
 *
 * Side Effects:    None
 *
 * Overview:        This function waits for a previously initiated PHY negotiation to complete.
 *                  Subsequently, EthPhyGetNegotiationResult() can be called. 
 *
 * Note:            None
 *****************************************************************************/
eEthRes  __attribute__((weak)) EthPhyNegotiationComplete(int waitComplete)
{
	__BMCONbits_t	phyBMCon;
	__BMSTATbits_t	phyStat;
	eEthRes	res;
	
	phyBMCon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);
	if(phyBMCon.AN_ENABLE)
	{	// just protect from an accidental call
		phyBMCon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);
		phyStat.w=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);

		if(waitComplete)
		{
			unsigned int	tStart, tWait;
			
			if(phyBMCon.AN_RESTART)
			{	// not started yet
				tWait=(GetSystemClock()/2000)*PHY_NEG_INIT_TMO;
				tStart=ReadCoreTimer();
				do
				{
					phyBMCon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);
				}while(phyBMCon.AN_RESTART && (ReadCoreTimer()-tStart)<tWait);		// wait auto negotiation start
			}

			if(!phyBMCon.AN_RESTART)
			{	// ok, started
				tWait=(GetSystemClock()/2000)*PHY_NEG_DONE_TMO;
				tStart=ReadCoreTimer();
				do
				{
					phyStat.w=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);
				}while(phyStat.AN_COMPLETE==0 && (ReadCoreTimer()-tStart)<tWait);	// wait auto negotiation done

				phyStat.w=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);
			}
		}
	}

	if(!phyBMCon.AN_ENABLE)
	{	
		res=ETH_RES_NEGOTIATION_INACTIVE;		// no negotiation is taking place!
	}
	else if(phyBMCon.AN_RESTART)
	{
		res=ETH_RES_NEGOTIATION_NOT_STARTED;		// not started yet/tmo
	}
	else
	{
		res= (phyStat.AN_COMPLETE==0)?ETH_RES_NEGOTIATION_ACTIVE:ETH_RES_OK;	// active/tmo/ok
	}

	return res;
}



/****************************************************************************
 * Function:        EthPhyGetNegotiationResult
 *
 * PreCondition:    EthPhyInit, EthPhyRestartNegotiation and EthPhyNegotiationComplete should have been called.
 *
 * Input:           pFlags     - address to store the negotiation result
 *                  pPauseType - address to store the pause type supported by the LP
 *
 * Output:          the link status after the (completed) negotiation
 *
 * Side Effects:    None
 *
 * Overview:        This function returns the result of a previously initiated negotiation.
 *                  The result is based on the PHY status!.
 *
 * Note:            If no negotiation possible/active/failed, most likely the flags are invalid!
 *****************************************************************************/
eEthLinkStat  __attribute__((weak)) EthPhyGetNegotiationResult(eEthOpenFlags* pFlags, eEthMacPauseType* pPauseType)
{
	eEthLinkStat	linkStat;
	eEthOpenFlags	oFlags;
	__BMSTATbits_t	phyStat;
	__ANEXPbits_t	phyExp;
	__ANLPADbits_t	lpAD;
	__ANADbits_t	anadReg;
	eEthMacPauseType pauseType;

	
	//	should have BMCON.AN_ENABLE==1
	//	wait for it to finish! 


	oFlags=0;	// don't know the result yet
	pauseType=ETH_MAC_PAUSE_TYPE_NONE;

	phyStat.w=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);
	if(phyStat.AN_COMPLETE==0)
	{
		linkStat=(ETH_LINK_ST_DOWN|ETH_LINK_ST_NEG_TMO);
	}
	else if(!phyStat.LINK_STAT)
	{
		linkStat=ETH_LINK_ST_DOWN;
	}
	else
	{	// we're up and running
		int	lcl_Pause, lcl_AsmDir, lp_Pause, lp_AsmDir;		// pause capabilities, local and LP
	
		linkStat=ETH_LINK_ST_UP;

		lcl_Pause=(ETH_MAC_PAUSE_CPBL_MASK & ETH_MAC_PAUSE_TYPE_PAUSE)?1:0;
		lcl_AsmDir=(ETH_MAC_PAUSE_CPBL_MASK & ETH_MAC_PAUSE_TYPE_ASM_DIR)?1:0;
		lp_Pause=lp_AsmDir=0;			// in case negotiation fails
		lpAD.w=_ANAD_BASE10T_MASK;		// lowest priority resolution
				
		phyExp.w=_PhyReadReg(PHY_REG_ANEXP, _PhyAdd);
		if(phyExp.LP_AN_ABLE)
		{	// ok,valid auto negotiation info

			lpAD.w=_PhyReadReg(PHY_REG_ANLPAD, _PhyAdd);
			if(lpAD.REM_FAULT)
			{
				linkStat|=ETH_LINK_ST_REMOTE_FAULT;
			}
			
			if(lpAD.PAUSE)
			{
				linkStat|=ETH_LINK_ST_LP_PAUSE;
				lp_Pause=1;
			}
			if(lpAD.ASM_DIR)
			{
				linkStat|=ETH_LINK_ST_LP_ASM_DIR;
				lp_AsmDir=1;
			}
			
		}
		else
		{
			linkStat|=ETH_LINK_ST_LP_NEG_UNABLE;
			if(phyExp.PDF)
			{
				linkStat|=ETH_LINK_ST_PDF;
			}
		}
	
		// set the PHY connection params
		
		anadReg.w=_PhyReadReg(PHY_REG_ANAD, _PhyAdd);		// get our advertised capabilities
		anadReg.w&=lpAD.w;				// get the matching ones
		// get the settings, according to IEEE 802.3 Annex 28B.3 Priority Resolution
		// Note: we don't support 100BaseT4 !
		
		if(anadReg.w&_ANAD_BASE100TX_FDX_MASK)
		{
			oFlags=(ETH_OPEN_100|ETH_OPEN_FDUPLEX);
		}
		else if(anadReg.w&_ANAD_BASE100TX_MASK)
		{
			oFlags=(ETH_OPEN_100|ETH_OPEN_HDUPLEX);
		}
		else if(anadReg.w&_ANAD_BASE10T_FDX_MASK)
		{
			oFlags=(ETH_OPEN_10|ETH_OPEN_FDUPLEX);
		}
		else if(anadReg.w&_ANAD_BASE10T_MASK)
		{
			oFlags=(ETH_OPEN_10|ETH_OPEN_HDUPLEX);
		}
		else
		{	// this should NOT happen!
			linkStat|=ETH_LINK_ST_NEG_FATAL_ERR;
			linkStat&=~ETH_LINK_ST_UP;		// make sure we stop...!
		}


		// set the pause type for the MAC
		// according to IEEE Std 802.3-2002 Tables 28B-2, 28B-3 
		if(oFlags&ETH_OPEN_FDUPLEX)
		{	// pause type relevant for full duplex only
			if(lp_Pause & (lcl_Pause|(lcl_AsmDir&lp_AsmDir)))
			{
				pauseType=ETH_MAC_PAUSE_TYPE_EN_TX;
			}
			if(lcl_Pause & (lp_Pause | (lcl_AsmDir&lp_AsmDir)))
			{
				pauseType|=ETH_MAC_PAUSE_TYPE_EN_RX;
			}
		}
	}
	
	if(pFlags)
	{
		*pFlags=oFlags;
	}

	if(pPauseType)
	{
		*pPauseType=pauseType;
	}
	return linkStat;
}

/****************************************************************************
 * Function:        EthPhyGetLinkStatus
 *
 * PreCondition:    EthPhyInit should have been called.
 *
 * Input:           refresh - boolean to specify if double read is needed.
 *
 * Output:          the current link status
 *
 * Side Effects:    None
 *
 * Overview:        This function reads the PHY to get current link status
 *                  If refresh is specified then, if the link is down a second read
 *                  will be performed to return the current link status.
 *
 * Note:            None
 *****************************************************************************/
eEthLinkStat  __attribute__((weak)) EthPhyGetLinkStatus(int refresh)
{
	__BMSTATbits_t	phyStat;
	
	// read the link status
	phyStat.w=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);
	if(phyStat.LINK_STAT==0 && refresh)
	{	// link down could be an old condition. re-read
		phyStat.w=_PhyReadReg(PHY_REG_BMSTAT, _PhyAdd);
	}

	return _Phy2LinkStat(phyStat);
}


/****************************************************************************
 * Function:        EthPhyReset
 *
 * PreCondition:    EthPhyInit() should have been called
 *                  Communication with the PHY already established
 *
 * Input:           waitComplete  - if TRUE the procedure will wait for reset to complete
 *
 * Output:          TRUE if the reset procedure completed (or completion not required)
 *                  FALSE otherwise
 *
 * Side Effects:    None
 *
 * Overview:        This function immediately resets the PHY.
 *                  It does not wait for the reset procedure to complete
 *
 * Note:            None
 *****************************************************************************/
int  __attribute__((weak)) EthPhyReset(int waitComplete)
{

	EthMIIMWriteStart(PHY_REG_BMCON, _PhyAdd, _BMCON_RESET_MASK);		// Soft Reset the PHY
	
	if(waitComplete)
	{	// wait reset self clear
		__BMCONbits_t bmcon;
		unsigned int	tStart, tWaitReset;

		tWaitReset=(GetSystemClock()/2000)*PHY_RESET_CLR_TMO;
		tStart=ReadCoreTimer();
		do
		{
			bmcon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);
		}while(bmcon.RESET && (ReadCoreTimer()-tStart)<tWaitReset);

		bmcon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);
		if(bmcon.RESET)
		{	// tmo clearing the reset
			return 0;
		}
	}

	return 1;

}



/****************************************************************************
 * Function:        EthPhyScanLinkStart
 *
 * PreCondition:    EthPhyInit() should have been called
 *                  Communication with the PHY already established
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function starts a scan of the PHY link status register.
 *                  It is meant as a more efficient way of having access to the current link status
 *                  since the normal MIIM frame read operation is pretty lengthy.
 *
 * Note:            Any PHY register can be subject of a scan.
 *                  The application should use the MIIM access functions of the Ethernet plib abd the specific PHY knowledge. 
 *****************************************************************************/
void __attribute__((weak)) EthPhyScanLinkStart(void)
{
	EthMIIMScanStart(PHY_REG_BMSTAT, _PhyAdd);
}

/****************************************************************************
 * Function:        EthPhyScanLinkRead
 *
 * PreCondition:    EthPhyInit() should have been called
 *                  Communication with the PHY already established
 *                  A PHY scan operation should have been started.
 *
 * Input:           None
 *
 * Output:          the current link status as being updated by the current scan in progress
 *
 * Side Effects:    None
 *
 * Overview:        This function returns the current result of a scan operation.
 *                  The last updated value is returned.
 *                  There's no way of knowing when effectively this last update occurred.
 *
 * Note:            None
 *****************************************************************************/
eEthLinkStat __attribute__((weak)) EthPhyScanLinkRead(void)
{
	__BMSTATbits_t	phyStat;
	
	phyStat.w=EthMIIMScanResult();

	return _Phy2LinkStat(phyStat);
}


/****************************************************************************
 * Function:        EthPhyScanLinkStop
 *
 * PreCondition:    EthPhyInit() should have been called
 *                  Communication with the PHY already established
 *                  A PHY scan operation should have been started.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function stops a previously started PHY scan.
 *
 * Note:            The scan operation shouldn't interfere with normal read operations.
 *                  Therefore the scan operation should be stopped before initiating another
 *                  normal MIIM transaction
 *****************************************************************************/
void __attribute__((weak)) EthPhyScanLinkStop(void)
{
	EthMIIMScanStop();
}




/****************************************************************************
 *                 local functions
 ****************************************************************************/


/****************************************************************************
 * Function:        _PhyInitIo
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Helper to properly set the Eth i/o pins to digital pins.
 *
 * Note:            Even when the Eth device is turned on the analog shared pins have to be configured.
 *****************************************************************************/
static void _PhyInitIo(void)
{
	__DEVCFG3bits_t bcfg3;

	bcfg3=DEVCFG3bits;
	if(bcfg3.FETHIO)
	{	// default setting, both RMII and MII 
		PORTSetPinsDigitalOut(_ETH_MDC_PORT, _ETH_MDC_BIT);	
		PORTSetPinsDigitalIn(_ETH_MDIO_PORT, _ETH_MDIO_BIT);

		PORTSetPinsDigitalOut(_ETH_TXEN_PORT, _ETH_TXEN_BIT);	
		PORTSetPinsDigitalOut(_ETH_TXD0_PORT, _ETH_TXD0_BIT);	
		PORTSetPinsDigitalOut(_ETH_TXD1_PORT, _ETH_TXD1_BIT);	

		
		PORTSetPinsDigitalIn(_ETH_RXCLK_PORT, _ETH_RXCLK_BIT);
		PORTSetPinsDigitalIn(_ETH_RXDV_PORT, _ETH_RXDV_BIT);
		PORTSetPinsDigitalIn(_ETH_RXD0_PORT, _ETH_RXD0_BIT);
		PORTSetPinsDigitalIn(_ETH_RXD1_PORT, _ETH_RXD1_BIT);
		PORTSetPinsDigitalIn(_ETH_RXERR_PORT, _ETH_RXERR_BIT);


		if(bcfg3.FMIIEN)
		{	// just MII
			PORTSetPinsDigitalIn(_ETH_TXCLK_PORT, _ETH_TXCLK_BIT);
			PORTSetPinsDigitalOut(_ETH_TXD2_PORT, _ETH_TXD2_BIT);	
			PORTSetPinsDigitalOut(_ETH_TXD3_PORT, _ETH_TXD3_BIT);	
			PORTSetPinsDigitalOut(_ETH_TXERR_PORT, _ETH_TXERR_BIT);	

			PORTSetPinsDigitalIn(_ETH_RXD2_PORT, _ETH_RXD2_BIT);
			PORTSetPinsDigitalIn(_ETH_RXD3_PORT, _ETH_RXD3_BIT);
			PORTSetPinsDigitalIn(_ETH_CRS_PORT, _ETH_CRS_BIT);
			PORTSetPinsDigitalIn(_ETH_COL_PORT, _ETH_COL_BIT);
		}
	}
	else
	{	// alternate setting, both RMII and MII
		PORTSetPinsDigitalOut(_ETH_ALT_MDC_PORT, _ETH_ALT_MDC_BIT);	
		PORTSetPinsDigitalIn(_ETH_ALT_MDIO_PORT, _ETH_ALT_MDIO_BIT);

		PORTSetPinsDigitalOut(_ETH_ALT_TXEN_PORT, _ETH_ALT_TXEN_BIT);	
		PORTSetPinsDigitalOut(_ETH_ALT_TXD0_PORT, _ETH_ALT_TXD0_BIT);	
		PORTSetPinsDigitalOut(_ETH_ALT_TXD1_PORT, _ETH_ALT_TXD1_BIT);	

		
		PORTSetPinsDigitalIn(_ETH_ALT_RXCLK_PORT, _ETH_ALT_RXCLK_BIT);
		PORTSetPinsDigitalIn(_ETH_ALT_RXDV_PORT, _ETH_ALT_RXDV_BIT);
		PORTSetPinsDigitalIn(_ETH_ALT_RXD0_PORT, _ETH_ALT_RXD0_BIT);
		PORTSetPinsDigitalIn(_ETH_ALT_RXD1_PORT, _ETH_ALT_RXD1_BIT);
		PORTSetPinsDigitalIn(_ETH_ALT_RXERR_PORT, _ETH_ALT_RXERR_BIT);


		if(bcfg3.FMIIEN)
		{	// just MII
			PORTSetPinsDigitalIn(_ETH_ALT_TXCLK_PORT, _ETH_ALT_TXCLK_BIT);
			PORTSetPinsDigitalOut(_ETH_ALT_TXD2_PORT, _ETH_ALT_TXD2_BIT);	
			PORTSetPinsDigitalOut(_ETH_ALT_TXD3_PORT, _ETH_ALT_TXD3_BIT);	
			PORTSetPinsDigitalOut(_ETH_ALT_TXERR_PORT, _ETH_ALT_TXERR_BIT);	

			PORTSetPinsDigitalIn(_ETH_ALT_RXD2_PORT, _ETH_ALT_RXD2_BIT);
			PORTSetPinsDigitalIn(_ETH_ALT_RXD3_PORT, _ETH_ALT_RXD3_BIT);
			PORTSetPinsDigitalIn(_ETH_ALT_CRS_PORT, _ETH_ALT_CRS_BIT);
			PORTSetPinsDigitalIn(_ETH_ALT_COL_PORT, _ETH_ALT_COL_BIT);
		}
		
	}
}


/****************************************************************************
 * Function:        _PhyDetectReset
 *
 * PreCondition:    EthMIIMConfig() should have been called
 *
 * Input:           None
 *
 * Output:          TRUE if the detection and the reset of the PHY succeeded,
 *                  FALSE if no PHY detected
 *
 * Side Effects:    None
 *
 * Overview:        This function detects and resets the PHY.
 *
 * Note:            Needs the system running frequency to for the PHY detection
 *****************************************************************************/
static int _PhyDetectReset(void)
{
	__BMCONbits_t bmcon;
	unsigned int	tStart, tWaitReset;

	bmcon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);		// read the BMCON register

	if(bmcon.RESET)
	{	// that is already suspicios...but give it a chance to clear itself
		tWaitReset=(GetSystemClock()/2000)*PHY_RESET_CLR_TMO;
		tStart=ReadCoreTimer();
		do
		{
			bmcon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);
		}while(bmcon.RESET && (ReadCoreTimer()-tStart)<tWaitReset);	// wait reset self clear

		bmcon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);
		if(bmcon.RESET)
		{	// tmo clearing the reset
			return 0;
		}
	}

	// ok, reset bit is low
	// try to see if we can write smth to the PHY
	// we use Loopback and Isolate bits
	EthMIIMWriteStart(PHY_REG_BMCON, _PhyAdd, _BMCON_LOOPBACK_MASK|_BMCON_ISOLATE_MASK);	// write control bits
	bmcon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);		// read back
	if(bmcon.LOOPBACK==0 || bmcon.ISOLATE==0)
	{	// failed to set
		return 0;
	}
	bmcon.w^=_BMCON_LOOPBACK_MASK|_BMCON_ISOLATE_MASK;
	EthMIIMWriteStart(PHY_REG_BMCON, _PhyAdd, bmcon.w);		// clear bits and write
	bmcon.w=_PhyReadReg(PHY_REG_BMCON, _PhyAdd);		// read back
	if(bmcon.LOOPBACK || bmcon.ISOLATE)
	{	// failed to clear
		return 0;
	}
	
	// everything seems to be fine
	//
	return EthPhyReset(1);
}


#endif	// defined(__PIC32MX__) && defined(_ETH)	// ETHC present
