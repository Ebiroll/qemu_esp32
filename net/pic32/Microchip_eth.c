/*
 * File:   Microchip_eth.h
 * Author: vbannelier
 *
 * Created on 2 avril 2015, 10:49
 */


#include <netif/Microchip_eth.h>

#include <netif/ETHPIC32ExtPhy.h>
#include <peripheral/eth.h>
#include <netif/ETHPIC32ExtPhyDP83848.h>

#include <HardwareProfile.h>
#include <ethernetif.h>

static void* _MacAllocCallback( size_t nitems, size_t size, void* param );

#define Link_status_changed 0x2000
#define Link_autonegotiation_complete 0x0400


int MACInit(volatile TxBuffers* T1, unsigned char AppMACAddr[6])
{
    /***********Eth interrupt***************/
    IEC1CLR =0x10000000;        // disable Ethernet interrupts
    ETHCON1CLR=0x00008300;      // reset: disable ON, clear TXRTS, RXEN
    while(ETHSTAT&0x80);        // wait everything down
    ETHCON1SET=0x00008000;      // turn device ON

    ETHIENCLR=0x000063ef;       // disable all events
    ETHIENSET=0x00000088;       // enable the RXDONE, TXDONE interrupt events

    IPC12CLR = 0x0000001f;      // clear the Ethernet Controller priority and sub-priority
    IPC12bits.ETHIS = 0;        // sub priority of 0
    IPC12bits.ETHIP = 3;        // priority of 3

    /************Phy interrupt**************/
    IEC0bits.INT3IE = 0;        //Disnable INT1 Interrupt Service Routine
    INTCONbits.INT3EP = 0;      //Setup INT1 to interupt on falling edge
    IPC3bits.INT3IP = 3;        //set 3 priority
    IPC3bits.INT3IS = 1;        //set 1 sub priority
    unsigned char	initFail=0;

    if (T1 == NULL)
    {
        printf("ERROR : macinit : T1 = NULL\r\n");
        initFail++;
    }
    else
    {
        if (AppMACAddr == NULL)
        {
            printf("ERROR : macinit : AppMACAddr = NULL\r\n");
            initFail++;
        }
        else
        {
            unsigned char	_RxBuffers[EMAC_RX_DESCRIPTORS][EMAC_RX_BUFF_SIZE];	// rx buffers for incoming data
            eEthLinkStat	linkStat = ETH_LINK_ST_DOWN;
            eEthOpenFlags	oFlags = 0, linkFlags = 0;
            eEthMacPauseType    pauseType = 0;
            eEthPhyCfgFlags     cfgFlags = 0;

            eEthRes	ethRes = ETH_RES_DTCT_ERR, phyInitRes = ETH_RES_DTCT_ERR;
            unsigned char	useFactMACAddr[6] = {0x00, 0x04, 0xa3, 0x00, 0x00, 0x00};		// to check if factory programmed MAC address needed
            unsigned char	unsetMACAddr[6] =   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};		// not set MAC address

            int ix = 0;

            // set the TX/RX pointers
            for(ix=0; ix<sizeof(T1->_TxDescriptors)/sizeof(*(T1->_TxDescriptors)); ix++)
            {
                T1->_TxDescriptors[ix].txBusy=0;
            }
            T1->_pTxCurrDcpt= (sEthTxDcpt*) (T1->_TxDescriptors+0);
            T1->_TxLastDcptIx=0;
            T1->_CurrWrPtr = 0;


            #ifdef PHY_RMII
                cfgFlags=ETH_PHY_CFG_RMII;
            #else
                cfgFlags=ETH_PHY_CFG_MII;
            #endif

            #ifdef PHY_CONFIG_ALTERNATE
                cfgFlags|=ETH_PHY_CFG_ALTERNATE;
            #else
                cfgFlags|=ETH_PHY_CFG_DEFAULT;
            #endif


            #if ETH_CFG_LINK
                oFlags=ETH_CFG_AUTO?ETH_OPEN_AUTO:0;
                oFlags|=ETH_CFG_10?ETH_OPEN_10:0;
                oFlags|=ETH_CFG_100?ETH_OPEN_100:0;
                oFlags|=ETH_CFG_HDUPLEX?ETH_OPEN_HDUPLEX:0;
                oFlags|=ETH_CFG_FDUPLEX?ETH_OPEN_FDUPLEX:0;
                if(ETH_CFG_AUTO_MDIX)
                {
                    oFlags|=ETH_OPEN_MDIX_AUTO;
                }
                else
                {
                    oFlags|=ETH_CFG_SWAP_MDIX?ETH_OPEN_MDIX_SWAP:ETH_OPEN_MDIX_NORM;
                }
            #else
                oFlags= ETH_OPEN_DEFAULT;
            #endif // ETH_CFG_LINK

            pauseType=(oFlags&ETH_OPEN_FDUPLEX)?ETH_MAC_PAUSE_CPBL_MASK:ETH_MAC_PAUSE_TYPE_NONE;

            EthInit();
            phyInitRes=EthPhyInit(oFlags, cfgFlags, &linkFlags);

            // let the auto-negotiation (if any) take place
            // continue the initialization
            EthRxFiltersClr(ETH_FILT_ALL_FILTERS);
            EthRxFiltersSet(ETH_FILT_CRC_ERR_REJECT|ETH_FILT_RUNT_REJECT|ETH_FILT_ME_UCAST_ACCEPT|ETH_FILT_MCAST_ACCEPT|ETH_FILT_BCAST_ACCEPT);

            // set the MAC address
            if(memcmp(AppMACAddr, useFactMACAddr, sizeof(useFactMACAddr))==0 || memcmp(AppMACAddr, unsetMACAddr, sizeof(unsetMACAddr))==0 )
            {	// use the factory programmed address existent in the MAC
                unsigned short* pS=(unsigned short*)AppMACAddr;
                *pS++=EMACxSA2;
                *pS++=EMACxSA1;
                *pS=EMACxSA0;
            }
            else
            {   // use the supplied address
                EthMACSetAddress(AppMACAddr);
            }


            if(EthDescriptorsPoolAdd(EMAC_TX_DESCRIPTORS, ETH_DCPT_TYPE_TX, _MacAllocCallback, 0)!=EMAC_TX_DESCRIPTORS)
            {
                printf("ERROR : macinit : EthDescriptorsPoolAdd TX\r\n");
                initFail++;
            }

            if(EthDescriptorsPoolAdd(EMAC_RX_DESCRIPTORS, ETH_DCPT_TYPE_RX, _MacAllocCallback, 0)!=EMAC_RX_DESCRIPTORS)
            {
                printf("ERROR : macinit : EthDescriptorsPoolAdd RX\r\n");
                initFail++;
            }

            if (EthRxSetBufferSize(EMAC_RX_BUFF_SIZE) != ETH_RES_OK)
            {
                printf("ERROR : macinit : EthRxSetBufferSize\r\n");
                initFail++;
            }

            // set the RX buffers as permanent receive buffers
            for(ix=0, ethRes=ETH_RES_OK; ix<EMAC_RX_DESCRIPTORS && ethRes==ETH_RES_OK; ix++)
            {
                void* pRxBuff=_RxBuffers[ix];
                ethRes=EthRxBuffersAppend(&pRxBuff, 1, ETH_BUFF_FLAG_RX_STICKY);
            }

            if(ethRes!=ETH_RES_OK)
            {
                printf("ERROR : macinit : ethRes\r\n");
                initFail++;
            }

            if(phyInitRes==ETH_RES_OK)
            {	// PHY was detected
                if(oFlags&ETH_OPEN_AUTO)
                {	// we'll just wait for the negotiation to be done
                    _LinkReconfigure()?ETH_LINK_ST_UP:ETH_LINK_ST_DOWN;	// if negotiation not done yet we need to try it next time
                    while ((EthPhyGetLinkStatus(0)&ETH_LINK_ST_UP) == 0)
                    {
                            //printf("MAC Unlinked\r\n");
                    }
                    _LinkReconfigure();	// if negotiation not done yet we need to try it next time
                }
                else
                {	// no need of negotiation results; just update the MAC
                    EthMACOpen(linkFlags, pauseType);
                    linkStat=EthPhyGetLinkStatus(0);
                    if (linkStat&ETH_LINK_ST_UP == 0)
                    {
                        printf("ERROR : macinit : linkStat\r\n");
                        initFail++;
                    }
                }
            }
            else
            {
                printf("ERROR : macinit : phyInitRes\r\n");
                initFail++;
            }

            /*****************************Configure interruption on Phy**********************************************/
            EthMIIMWriteStart ( PHY_REG_MII_INT_CTRL, PHY_ADDRESS, 0x0003 );
            while(EthMIIMBusy());   // wait device not busy
            EthMIIMReadStart(PHY_REG_MII_INT_CTRL, PHY_ADDRESS);
            while(EthMIIMBusy());   // wait read operation complete
            short resPhy = EthMIIMReadResult();
            if ((resPhy & 0x0007) != 0x0003)
            {
                printf("ERROR : macinit : configuration phy : MICR incorrect : 0x%x\r\n",resPhy);
                initFail++;
            }
            else
            {
                EthMIIMWriteStart ( PHY_REG_MII_INT_STAT, PHY_ADDRESS, 0x0024 );
                while(EthMIIMBusy());   // wait device not busy
                EthMIIMReadStart(PHY_REG_MII_INT_STAT, PHY_ADDRESS);
                while(EthMIIMBusy());   // wait read operation complete
                resPhy = EthMIIMReadResult();
                if ((resPhy & 0x7f) != 0x0024)
                {
                    printf("ERROR : macinit : configuration phy : MISR incorrect : 0x%x\r\n",resPhy);
                    initFail++;
                }
                else
                {

                    //Start external interruption from Phy
                    IFS0bits.INT3IF = 0;        //Reset INT3 interrupt flag
                    IEC0bits.INT3IE = 1;        //Enable INT3 Interrupt Service Routine

                    //Start ethernet interruption
                    IFS1CLR=0x10000000;         // clear the interrupt controller flag
                    ETHIRQCLR=0x000063ef;       // clear any existing interrupt event
                    IEC1SET=0x10000000;         // enable the Ethernet Controller interrupt
                }   // MISR incorrect
            }   // MICR incorrect
        }   //AppMACAddr = NULL
    }   //T1 = NULL
    return initFail;
}




int MACReady(volatile TxBuffers *T1)
{
    int returnCode = FALSE;
    if (T1 == NULL)
    {
        printf("ERROR : MACReady : T1 = NULL\r\n");
        returnCode = FALSE;
    }
    else
    {
        int	ix;
        if(T1->_pTxCurrDcpt==0)
        {
            for(ix=T1->_TxLastDcptIx+1; ix<sizeof(T1->_TxDescriptors)/sizeof(*(T1->_TxDescriptors)); ix++)
            {
                if(T1->_TxDescriptors[ix].txBusy==0)
                {	// found a non busy descriptor
                    T1->_pTxCurrDcpt= (sEthTxDcpt*) (T1->_TxDescriptors+ix);
                    T1->_TxLastDcptIx=ix;
                    break;
                }
            }
            if(T1->_pTxCurrDcpt==0)
            {
                for(ix=0; ix <= T1->_TxLastDcptIx; ix++)
                {
                    if(T1->_TxDescriptors[ix].txBusy==0)
                    {	// found a non busy descriptor
                        T1->_pTxCurrDcpt= (sEthTxDcpt*) (T1->_TxDescriptors+ix);
                        T1->_TxLastDcptIx=ix;
                        break;
                    }
                }
            }
        }
        returnCode = (T1->_pTxCurrDcpt!=0);
    }   //T1 = NULL
    return returnCode;
}

void MACSetCurretPtr(volatile TxBuffers *T1)
{
    if (T1 == NULL)
    {
        printf("ERROR : MACSetCurretPtr : T1 = NULL\r\n");
    }
    else
    {
        T1->_CurrWrPtr = (unsigned char*)T1->_pTxCurrDcpt->dataBuff;
    }
}

void MACAddToBuffer(volatile TxBuffers *T1,unsigned char *buff,unsigned int dataLen)
{
    if (T1 == NULL)
    {
        printf("ERROR : MACAddToBuffer : T1 = NULL\r\n");
    }
    else
    {
        memcpy(T1->_CurrWrPtr, buff, dataLen);
        T1->_CurrWrPtr +=dataLen;
    }
}


void MACSendPckt(volatile TxBuffers *T1,unsigned short dataLen)
{
    if (T1 == NULL)
    {
        printf("ERROR : MACSendPckt : T1 = NULL\r\n");
    }
    else
    {
        if(T1->_pTxCurrDcpt && dataLen)
        {	// there is a buffer to transmit
            T1->_pTxCurrDcpt->txBusy=1;
            eEthRes result = ETH_RES_DTCT_ERR;
            result = EthTxSendBuffer((void*)T1->_pTxCurrDcpt->dataBuff, dataLen);
            if (result != ETH_RES_OK)
            {
                printf("ERROR : MACSendPckt : EthTxSendBuffer Failed %d\n",result);
            }
            T1->_CurrWrPtr = 0;
            T1->_pTxCurrDcpt = 0;
        }
    }   //T1 = NULL
}

/**************************
 * local functions and helpers
 ***********************************************/

/*********************************************************************
* Function:        void	_TxAckCallback(void* pPktBuff, int buffIx, void* fParam)
 *
 * PreCondition:    None
 *
 * Input:           pPktBuff - tx buffer to be acknowledged
 *                  buffIx   - buffer index, when packet spans multiple buffers
 *                  fParam   - optional parameter specified when EthTxAcknowledgeBuffer() called
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        TX acknowledge call back function.
 *                  Called by the Eth MAC when TX buffers are acknoledged (as a result of a call to EthTxAcknowledgeBuffer).
 *
 * Note:            None
 ********************************************************************/
void	_TxAckCallback(void* pPktBuff, int buffIx, void* fParam)
{
    //fParam and buffIx could be NULL
    if (pPktBuff == NULL)
    {
        printf("ERROR : _TxAckCallback : pPktBuff = NULL\r\n");
    }
    else
    {
        volatile sEthTxDcpt*	pDcpt;

        pDcpt=(sEthTxDcpt*)((char*)pPktBuff-offsetof(sEthTxDcpt, dataBuff));

        pDcpt->txBusy=0;
    }
}
/*********************************************************************
* Function:        void* _MacAllocCallback( size_t nitems, size_t size, void* param )
 *
 * PreCondition:    None
 *
 * Input:           nitems - number of items to be allocated
 *                  size   - size of each item
 *                  param  - optional parameter specified when EthDescriptorsPoolAdd() called
 *
 * Output:          pointer to the allocated memory of NULL if allocation failed
 *
 * Side Effects:    None
 *
 * Overview:        Memory allocation callback.
 *
 * Note:            None
 ********************************************************************/
static void* _MacAllocCallback( size_t nitems, size_t size, void* param )
{
    //param could be NULL
    return calloc(nitems, size);
}
/*********************************************************************
* Function:        int	_LinkReconfigure(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TRUE if negotiation succeeded and MAC was updated
 *                  FALSE otherwise
 *
 * Side Effects:    None
 *
 * Overview:        Performs re-configuration after auto-negotiation performed.
 *
 * Note:            None
 ********************************************************************/
int _LinkReconfigure(void)
{
    eEthOpenFlags       linkFlags;
    eEthLinkStat        linkStat;
    eEthMacPauseType    pauseType;
    eEthRes		phyRes;
    int	        	success=0;

    phyRes=EthPhyNegotiationComplete(0);	// see if negotiation complete
    if(phyRes==ETH_RES_OK)
    {
        linkStat=EthPhyGetNegotiationResult(&linkFlags, &pauseType);
        if(linkStat&ETH_LINK_ST_UP)
        {   // negotiation succeeded; properly update the MAC
            linkFlags|=(EthPhyGetHwConfigFlags()&ETH_PHY_CFG_RMII)?ETH_OPEN_RMII:ETH_OPEN_MII;
            EthMACOpen(linkFlags, pauseType);
            success=1;
        }
    }
    return success;
}


/************************************************************************/

void __attribute__( (interrupt(ipl3), vector( _ETH_VECTOR ), nomips16)) __EthInterrupt(void)
{
    int ethFlags = ETHIRQ;     // read the interrupt flags (requests)
    // the sooner we acknowledge, the smaller the chance to miss another event of the
    // same type because of a lengthy ISR
    ETHIRQCLR = ethFlags;     // acknowledge the interrupt flags

    CommonEthISR(ethFlags & 0x80,ethFlags & 0x8);

    IFS1CLR= 0x10000000;    // Be sure to clear the Ethernet Controller Interrupt
                            // Controller flag before exiting the service routine.
}



void __attribute__( (interrupt(ipl3), vector( _EXTERNAL_3_VECTOR ), nomips16)) _INT1Interrupt(void)
{
    unsigned short result = 0;

    //External interrupt link to the Phy interrupt
    //So we can reset the flag
    IFS0bits.INT3IF = 0;

    //And find out the flag from Phy
    while(EthMIIMBusy());   // wait device not busy
    EthMIIMReadStart(PHY_REG_MII_INT_STAT, PHY_ADDRESS);
    while(EthMIIMBusy());   // wait read operation complete
    result = EthMIIMReadResult();

    if (result & Link_autonegotiation_complete)     // Link retablished
    {
        while ( _LinkReconfigure() != ETH_LINK_ST_UP);
        SetLinkUpCallback();
    }
    else if (result & Link_status_changed)          // Link down
    {
        SetLinkDownCallback();
    }
    else
    {
        printf("Phy interrupt : unknow flag\r\n");
    }
}
