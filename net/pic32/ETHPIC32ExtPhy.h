/*********************************************************************
 *
 *                  External Phy API header file
 *
 *********************************************************************
 * FileName:        ETHPIC32ExtPhy.h
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


#ifndef _ETH_PHY_H_
#define _ETH_PHY_H_

#include <peripheral/eth.h>

// definitions

#ifdef _ETH		// ETHC present


#define	PHY_NEG_INIT_TMO	1		// negotiation initiation timeout, ms.

#define	PHY_NEG_DONE_TMO	2000		// negotiation complete timeout, ms.
						// based on IEEE 802.3 Clause 28 Table 28-9 autoneg_wait_timer value (max 1s)

#define	PHY_RESET_CLR_TMO	500		// reset self clear timeout, ms.
						// IEEE 802.3 Clause 22 Table 22-7 and paragraph "22.2.4.1.1 Reset" (max 0.5s)

typedef enum
{
	// PHY flags, connection flags 
	ETH_PHY_CFG_RMII        = 0x01,		// check that configuration fuses is RMII
	ETH_PHY_CFG_MII         = 0x00,		// check that configuration fuses is MII
	ETH_PHY_CFG_ALTERNATE   = 0x02,		// check that configuration fuses is ALT
	ETH_PHY_CFG_DEFAULT     = 0x00,		// check that configuration fuses is DEFAULT
	ETH_PHY_CFG_AUTO        = 0x10		// use the fuses configuration to detect if you are RMII/MII and ALT/DEFAULT configuration
							// NOTE: - this option does not check the consistency btw the software call and the way the
							//         fuses are configured. If just assumes that the fuses are properly configured.
							//       - option is valid for EthPhyInit() call only!
       	
}eEthPhyCfgFlags;		// flags for EthPhyInit() call



/*****************************************************************
 *                  PHY Interface functions
 *              They are generic and should work for any PHY
 *              They can be overridden if needed.    
 *****************************************************************/


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
eEthRes 		EthPhyInit(eEthOpenFlags oFlags, eEthPhyCfgFlags cFlags, eEthOpenFlags* pResFlags);


/****************************************************************************
 * Function:        EthPhyGetHwConfigFlags
 *
 * PreCondition:    None.
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
eEthPhyCfgFlags        EthPhyGetHwConfigFlags(void);



/****************************************************************************
 * Function:        EthPhyRestartNegotiation
 *
 * PreCondition:    - EthPhyInit should have been called.
 *                  - The PHY should have been initialized with proper duplex/speed mode!
 *
 * Input:           None
 *
 * Output:          ETH_RES_OK for success,
 *                  an error code otherwise
 *
 * Side Effects:    None
 *
 * Overview:        This function restarts the PHY negotiation.
 *                  After this restart the link can be reconfigured.
 *                  The EthPhyGetNegotiationResults() can be used to see the outcoming result.
 *
 * Note:            None
 *****************************************************************************/
eEthRes 		EthPhyRestartNegotiation(void);


/****************************************************************************
 * Function:        EthPhyNegotiationComplete
 *
 * PreCondition:    EthPhyInit (and EthPhyRestartNegotiation) should have been called.
 *
 * Input:           waitComplete - if wait for completion needed
 *
 * Output:          ETH_RES_OK if negotiation done,
 *                  an error code otherwise
 *
 * Side Effects:    None
 *
 * Overview:        This function waits for a previously initiated PHY negotiation to complete.
 *                  Subsequently, EthPhyGetNegotiationResult() can be called. 
 *
 * Note:            None
 *****************************************************************************/
eEthRes 		EthPhyNegotiationComplete(int waitComplete);


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
eEthLinkStat 	EthPhyGetNegotiationResult(eEthOpenFlags* pFlags, eEthMacPauseType* pPauseType);


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
 * Note:            This function performs a full MIIM transaction.
 *                  It should not be used when a link scan has been initiated (EthPhyScanLinkStart()).
 *****************************************************************************/
eEthLinkStat 	EthPhyGetLinkStatus(int refresh);



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
int 		EthPhyReset(int waitComplete);


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
void 		EthPhyScanLinkStart(void);


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
eEthLinkStat 		EthPhyScanLinkRead(void);


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
void 		EthPhyScanLinkStop(void);



/*****************************************************************
 *                    PHY SPecific Interface functions
 *                 Have to be provided for each specific PHY
 *****************************************************************/

/****************************************************************************
 * Function:        EthPhyMIIMAddress
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          PHY MIIM address
 *
 *
 * Side Effects:    None
 *
 * Overview:        This function returns the address the PHY uses for MIIM transactions
 *
 * Note:            None
 *****************************************************************************/
extern unsigned int		EthPhyMIIMAddress(void);


/****************************************************************************
 * Function:        EthPhyMIIMClock
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          PHY MIIM clock, Hz
 *
 *
 * Side Effects:    None
 *
 * Overview:        This function returns the maximum clock frequency that the PHY can use for the MIIM transactions
 *
 * Note:            None
 *****************************************************************************/
extern unsigned int		EthPhyMIIMClock(void);


/****************************************************************************
 * Function:        EthPhyConfigureMII
 *
 * PreCondition:    - Communication to the PHY should have been established.
 *
 * Input:           cFlags - the requested configuration flags: ETH_PHY_CFG_RMII/ETH_PHY_CFG_MII
 *
 * Output:          ETH_RES_OK - success,
 *                  an error code otherwise
 *
 *
 * Side Effects:    None
 *
 * Overview:        This function configures the PHY in one of MII/RMII operation modes.
 *
 * Note:            None
 *****************************************************************************/
extern eEthRes		EthPhyConfigureMII(eEthPhyCfgFlags cFlags);



/****************************************************************************
 * Function:        EthPhyConfigureMdix
 *
 * PreCondition:    - Communication to the PHY should have been established.
 *
 * Input:           oFlags - the requested open flags: ETH_OPEN_MDIX_AUTO, ETH_OPEN_MDIX_NORM/ETH_OPEN_MDIX_SWAP
 *
 * Output:          ETH_RES_OK - success,
 *                  an error code otherwise
 *
 *
 * Side Effects:    None
 *
 * Overview:        This function configures the MDIX mode for the PHY.
 *
 * Note:            None
 *****************************************************************************/
extern eEthRes		EthPhyConfigureMdix(eEthOpenFlags oFlags);



#endif	// _ETH

#endif	// _ETH_PHY_H_

