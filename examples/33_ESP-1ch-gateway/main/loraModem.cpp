// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017 Maarten Westenberg version for ESP8266
// Version 5.0.1
// Date: 2017-11-15
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains the LoRa modem specific code enabling to receive
// and transmit packages/messages.
// ========================================================================================
//
// STATE MACHINE
// The program uses the following state machine (in _state), all states
// are done in interrupt routine, only the follow-up of S-RXDONE is done
// in the main loop() program. This is because otherwise the interrupt processing
// would take too long to finish
//
// S-INIT=0, 	The commands in this state are executed only once
//	- Goto S_SCAN
// S-SCAN, 		CadScanner() part
//	- upon CDDONE (int0) got S_CAD
// S-CAD, 		
//	- Upon CDDECT (int1) goto S_RX, 
//	- Upon CDDONE (int0) goto S_SCAN
// S-RX, 		Received CDDECT so message detected, RX cycle started. 
//	- Upon RXDONE (int0) read ok goto S_RX or S_SCAN, 
//	- upon RXTOUT (int1) goto S_SCAN
// S-RXDONE, 	Read the buffer
//	- Wait for reading in loop()
//	- Upon message send to server goto S_SCAN
// S-TX			Transmitting a message
//	- Upon TX goto S_SCAN
//
//
// SPI AND INTERRUPTS
// The RFM96/SX1276 communicaties with the ESP8266 by means of interrupts 
// and SPI interface. The SPI interface is bidirectional and allows both
// parties to simultaneous write and read to registers.
// Major drawback is that access is not protected for interrupt and non-
// interrupt access. This means that when a program in loop() and a program
// in interrupt do access the readregister and writeRegister() function
// at teh same time that probably an error will occur.
// Therefore it is best to Either not use interrupts AT all (like LMIC)
// or only use these functions in inteerupts and to further processing
// in the main loop() program.
//
// ============================================================================
#include <Arduino.h>
#include "loraModem.h"
#include "ESP-sc-gway.h"
#include <SPI.h>


extern bool sx1272;	
void writeRegister(uint8_t addr, uint8_t value);


long txDelay= 0x00;								// delay time on top of server TMST


bool _cad= (bool) _CAD;	// Set to true for Channel Activity Detection, only when dio 1 connected
bool _hop=false;		// experimental; frequency hopping. Only use when dio2 connected
bool inHop=false;

unsigned long nowTime=0;
unsigned long hopTime=0;
unsigned long msgTime=0;


int freqs [] = { 
	868100000, 									// Channel 0, 868.1 MHz primary
	868300000, 									// Channel 1, 868.3 MHz mandatory
	868500000, 									// Channel 2, 868.5 MHz mandatory
	867100000, 									// Channel 3, 867.1 MHz
	867300000, 
	867500000, 
	867700000, 
	867900000, 
	868800000, 
	869525000									// Channel, for responses gateway (10%)
	// TTN defines an additional channel at 869.525Mhz using SF9 for class B. Not used
};


uint32_t  freq = freqs[0];
uint8_t	 ifreq = 0;								// Channel Index

volatile state_t _state;
volatile uint8_t _event=0;

uint8_t payLoad[128];			// Payload i

t_LoraBuffer LoraDown;


#if STATISTICS >= 1
// History of received uplink messages from nodes
struct stat_t statr[MAX_STAT];

// STATC contains the statistic that are gateway related and not per
// message. Example: Number of messages received on SF7 or number of (re) boots
// So where statr contains the statistics gathered per packet the statc
// contains general statics of the node
#if STATISTICS >= 2							// Only if we explicitely set it higher
//stat_c_t stat_c
struct stat_c_t statc;
#endif
#else // STATISTICS==0
struct stat_t statr[1];						// Always have at least one element to store in
#endif

uint8_t _rssi;	

LoraUp_t LoraUp;


#if _PIN_OUT==1
// Definition of the GPIO pins used by the Gateway for Hallard type boards
struct pins {
	uint8_t dio0=15;	// GPIO15 / D8. For the Hallard board shared between DIO0/DIO1/DIO2
	uint8_t dio1=15;	// GPIO15 / D8. Used for CAD, may or not be shared with DIO0
	uint8_t dio2=15;	// GPIO15 / D8. Used for frequency hopping, don't care
	uint8_t ss=16;		// GPIO16 / D0. Select pin connected to GPIO16 / D0
	uint8_t rst=0;		// GPIO0 / D3. Reset pin not used	
	// MISO 12 / D6
	// MOSI 13 / D7
	// CLK  14 / D5
} pins;
#elif _PIN_OUT==2
// For ComResult gateway PCB use the following settings
struct pins {
	uint8_t dio0=5;		// GPIO5 / D1. Dio0 used for one frequency and one SF
	uint8_t dio1=4;		// GPIO4 / D2. Used for CAD, may or not be shared with DIO0
	uint8_t dio2=0;		// GPIO0 / D3. Used for frequency hopping, don't care
	uint8_t ss=15;		// GPIO15 / D8. Select pin connected to GPIO15
	uint8_t rst=0;		// GPIO0 / D3. Reset pin not used	
} pins;
#elif _PIN_OUT==3
// For ComResult gateway PCB use the following settings
struct pins_t pins= {
  .dio0=26,   // GPIO5 / D1. Dio0 used for one frequency and one SF
  .dio1=33,   // GPIO4 / D2. Used for CAD, may or not be shared with DIO0
  .dio2=32,   // GPIO0 / D3. Used for frequency hopping, don't care
  .ss=18,     // GPIO15 / D8. Select pin connected to GPIO15
  .rst=14,    // GPIO0 / D3. Reset pin not used 
// Pin definetion of WIFI LoRa 32
// HelTec AutoMation 2017 support@heltec.cn 
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)  
};
#else
	// Use your own pin definitions, and uncomment #error line below
	// MISO 12 / D6
	// MOSI 13 / D7
	// CLK  14 / D5
	// SS   16 / D0
#error "Pin Definitions _PIN_OUT must be 1(HALLARD) or 2 (COMRESULT)"
#endif

// ----------------------------------------------------------------------------
// Mutex definitiona
//
// ----------------------------------------------------------------------------
#if MUTEX==1
	void CreateMutux(int *mutex) {
		*mutex=1;
	}

#define LIB_MUTEX 1
#if LIB_MUTEX==1
	bool GetMutex(int *mutex) {
		//noInterrupts();
		if (*mutex==1) { 
			*mutex=0; 
			//interrupts(); 
			return(true); 
		}
		//interrupts();
		return(false);
	}
#else	
	bool GetMutex(int *mutex) {

	int iOld = 1, iNew = 0;

	asm volatile (
		"rsil a15, 1\n"    // read and set interrupt level to 1
		"l32i %0, %1, 0\n" // load value of mutex
		"bne %0, %2, 1f\n" // compare with iOld, branch if not equal
		"s32i %3, %1, 0\n" // store iNew in mutex
		"1:\n"             // branch target
		"wsr.ps a15\n"     // restore program state
		"rsync\n"
		: "=&r" (iOld)
		: "r" (mutex), "r" (iOld), "r" (iNew)
		: "a15", "memory"
	);
	return (bool)iOld;
}
#endif

	void ReleaseMutex(int *mutex) {
		*mutex=1;
	}
	
#endif //MUTEX==1

// ----------------------------------------------------------------------------
// Read one byte value, par addr is address
// Returns the value of register(addr)
// 
// The SS (Chip select) pin is used to make sure the RFM95 is selected
// We also use a volatile mutexSPI to tell use whether the interrupt is in use.
// The variable is for obvious reasons valid for read and write traffic at the
// same time. Since both read and write mean that we write to the SPI interface.
// Parameters:
//	Address: SPI address to read from. Type unint8_t
// Return:
//	Value read from address
// ----------------------------------------------------------------------------

// define the SPI settings for reading messages
//SPISettings readSettings(SPISPEED, MSBFIRST, SPI_MODE0);

uint8_t readRegister(uint8_t addr)
{
	//noInterrupts();								// XXX
#if MUTEX_SPI==1
	if(!GetMutex(&mutexSPI)) {
#if DUSB>=1
		if (debug>=0) {
			gwayConfig.reents++;
			Serial.print(F("readRegister:: read reentry"));
			printTime();
			Serial.println();
			if (debug>=2) Serial.flush();
			delayMicroseconds(50);
			initLoraModem();
		}
#endif
		return 0;
	}
#endif

	SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE0));
							
    digitalWrite(pins.ss, LOW);					// Select Receiver
	SPI.transfer(addr & 0x7F);
	uint8_t res = (uint8_t) SPI.transfer(0x00);
    digitalWrite(pins.ss, HIGH);				// Unselect Receiver
	SPI.endTransaction();

#if MUTEX_SPI==1
	ReleaseMutex(&mutexSPI);
#endif
	//interrupts();								// XXX
    return((uint8_t) res);
}


// ----------------------------------------------------------------------------
// Write value to a register with address addr. 
// Function writes one byte at a time.
// Parameters:
//	addr: SPI address to write to
//	value: The value to write to address
// Returns:
//	<void>
// ----------------------------------------------------------------------------

// define the settings for SPI writing
SPISettings writeSettings(SPISPEED, MSBFIRST, SPI_MODE0);

void writeRegister(uint8_t addr, uint8_t value)
{
	//noInterrupts();							// XXX
#if MUTEX_SPO==1
	if(!GetMutex(&mutexSPI)) {
#if DUSB>=1
		if (debug>=0) {
			gwayConfig.reents++;
			Serial.print(F("writeRegister:: write reentry"));
			printTime();
			Serial.println();
			delayMicroseconds(50);
			initLoraModem();
			if (debug>=2) Serial.flush();
		}
#endif
		return;
	}
#endif
	
	SPI.beginTransaction(writeSettings);
	digitalWrite(pins.ss, LOW);					// Select Receiver
	
	SPI.transfer((addr | 0x80) & 0xFF);
	SPI.transfer(value & 0xFF);
	//delayMicroseconds(10);
	
    digitalWrite(pins.ss, HIGH);				// Unselect Receiver
	
	SPI.endTransaction();
	
#if MUTEX_SPO==1
	ReleaseMutex(&mutexSPI);
#endif
	//interrupts();
}


// ----------------------------------------------------------------------------
// Write a buffer to a register with address addr. 
// Function writes one byte at a time.
// Parameters:
//	addr: SPI address to write to
//	value: The value to write to address
// Returns:
//	<void>
// ----------------------------------------------------------------------------

void writeBuffer(uint8_t addr, uint8_t *buf, uint8_t len)
{
	//noInterrupts();							// XXX
#if MUTEX_SPO==1
	if(!GetMutex(&mutexSPI)) {
#if DUSB>=1
		if (debug>=0) {
			gwayConfig.reents++;
			Serial.print(F("writeBuffer:: write reentry"));
			printTime();
			Serial.println();
			delayMicroseconds(50);
			initLoraModem();
			if (debug>=2) Serial.flush();
		}
#endif
		return;
	}
#endif
	
	SPI.beginTransaction(writeSettings);
	digitalWrite(pins.ss, LOW);					// Select Receiver
	
	SPI.transfer((addr | 0x80) & 0xFF);			// write buffer address
	for (uint8_t i=0; i<len; i++) {
		SPI.transfer(buf[i] & 0xFF);
	}
    digitalWrite(pins.ss, HIGH);				// Unselect Receiver
	
	SPI.endTransaction();
	
#if MUTEX_SPI==1
	ReleaseMutex(&mutexSPI);
#endif
	//interrupts();
}

// ----------------------------------------------------------------------------
//  setRate is setting rate and spreading factor and CRC etc. for transmission
//		Modem Config 1 (MC1) == 0x72 for sx1276
//		Modem Config 2 (MC2) == (CRC_ON) | (sf<<4)
//		Modem Config 3 (MC3) == 0x04 | (optional SF11/12 LOW DATA OPTIMIZE 0x08)
//		sf == SF7 default 0x07, (SF7<<4) == SX72_MC2_SF7
//		bw == 125 == 0x70
//		cr == CR4/5 == 0x02
//		CRC_ON == 0x04
// ----------------------------------------------------------------------------

void setRate(uint8_t sf, uint8_t crc) 
{
	uint8_t mc1=0, mc2=0, mc3=0;
#if DUSB>=2
	if ((sf<SF7) || (sf>SF12)) {
		if (debug>=1) {
			Serial.print(F("setRate:: SF="));
			Serial.println(sf);
		}
		return;
	}
#endif
	// Set rate based on Spreading Factor etc
    if (sx1272) {
		mc1= 0x0A;				// SX1276_MC1_BW_250 0x80 | SX1276_MC1_CR_4_5 0x02
		mc2= ((sf<<4) | crc) % 0xFF;
		// SX1276_MC1_BW_250 0x80 | SX1276_MC1_CR_4_5 0x02 | SX1276_MC1_IMPLICIT_HEADER_MODE_ON 0x01
        if (sf == SF11 || sf == SF12) { mc1= 0x0B; }			        
    } 
	else {
	    mc1= 0x72;				// SX1276_MC1_BW_125==0x70 | SX1276_MC1_CR_4_5==0x02
		mc2= ((sf<<4) | crc) & 0xFF; // crc is 0x00 or 0x04==SX1276_MC2_RX_PAYLOAD_CRCON
		mc3= 0x04;				// 0x04; SX1276_MC3_AGCAUTO
        if (sf == SF11 || sf == SF12) { mc3|= 0x08; }		// 0x08 | 0x04
    }
	
	// Implicit Header (IH), for class b beacons (&& SF6)
	//if (getIh(LMIC.rps)) {
    //   mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
    //    writeRegister(REG_PAYLOAD_LENGTH, getIh(LMIC.rps)); // required length
    //}
	
	writeRegister(REG_MODEM_CONFIG1, (uint8_t) mc1);
	writeRegister(REG_MODEM_CONFIG2, (uint8_t) mc2);
	writeRegister(REG_MODEM_CONFIG3, (uint8_t) mc3);
	
	// Symbol timeout settings
    if (sf == SF10 || sf == SF11 || sf == SF12) {
        writeRegister(REG_SYMB_TIMEOUT_LSB, (uint8_t) 0x05);
    } else {
        writeRegister(REG_SYMB_TIMEOUT_LSB, (uint8_t) 0x08);
    }
	return;
}


// ----------------------------------------------------------------------------
// Set the frequency for our gateway
// The function has no parameter other than the freq setting used in init.
// Since we are usin a 1ch gateway this value is set fixed.
// ----------------------------------------------------------------------------

void  setFreq(uint32_t freq)
{
    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    writeRegister(REG_FRF_MSB, (uint8_t)(frf>>16) );
    writeRegister(REG_FRF_MID, (uint8_t)(frf>> 8) );
    writeRegister(REG_FRF_LSB, (uint8_t)(frf>> 0) );
	
	return;
}


// ----------------------------------------------------------------------------
//	Set Power for our gateway
// ----------------------------------------------------------------------------
void setPow(uint8_t powe)
{
	if (powe >= 16) powe = 15;
	//if (powe >= 15) powe = 14;
	else if (powe < 2) powe =2;
	
	ASSERT((powe>=2)&&(powe<=15));
	
	uint8_t pac = (0x80 | (powe & 0xF)) & 0xFF;
	writeRegister(REG_PAC, (uint8_t)pac);								// set 0x09 to pac
	
	// XXX Power settings for CFG_sx1272 are different
	
	return;
}


// ----------------------------------------------------------------------------
// Used to set the radio to LoRa mode (transmitter)
// Please note that this mode can only be set in SLEEP mode and not in Standby.
// Also there should be not need to re-init this mode is set in the setup() 
// function.
// For high freqs (>860 MHz) we need to & with 0x08 otherwise with 0x00
// ----------------------------------------------------------------------------

//void ICACHE_RAM_ATTR opmodeLora()
//{
//#ifdef CFG_sx1276_radio
//       uint8_t u = OPMODE_LORA | 0x80;   					// TBD: sx1276 high freq 
//#else // SX-1272
//	    uint8_t u = OPMODE_LORA | 0x08;
//#endif
//    writeRegister(REG_OPMODE, (uint8_t) u);
//}


// ----------------------------------------------------------------------------
// Set the opmode to a value as defined on top
// Values are 0x00 to 0x07
// The value is set for the lowest 3 bits, the other bits are as before.
// ----------------------------------------------------------------------------
void  opmode(uint8_t mode)
{
	if (mode == OPMODE_LORA) 
		writeRegister(REG_OPMODE, (uint8_t) mode);
	else
		writeRegister(REG_OPMODE, (uint8_t)((readRegister(REG_OPMODE) & ~OPMODE_MASK) | mode));
}

// ----------------------------------------------------------------------------
// Hop to next frequency as defined by NUM_HOPS
// This function should only be used for receiver operation. The current
// receiver frequency is determined by ifreq index like so: freqs[ifreq] 
// ----------------------------------------------------------------------------
void hop() {
	// If we are already in a hop function, do not proceed
	if (!inHop) {

		inHop=true;
		opmode(OPMODE_STANDBY);
		ifreq = (ifreq + 1)% NUM_HOPS ;
		setFreq(freqs[ifreq]);
		hopTime = micros();								// record HOP moment
		opmode(OPMODE_CAD);
		inHop=false;
	}
#if DUSB>=2
	else {
		if (debug >= 3) {
			Serial.println(F("Hop:: Re-entrance try"));
		}
	}
#endif
}
	

// ----------------------------------------------------------------------------
// This LoRa function reads a message from the LoRa transceiver
// on Success: returns message length read when message correctly received
// on Failure: it returns a negative value on error (CRC error for example).
// UP function
// This is the "lowlevel" receive function called by stateMachine()
// dealing with the radio specific LoRa functions
// ----------------------------------------------------------------------------
uint8_t receivePkt(uint8_t *payload)
{
    uint8_t irqflags = readRegister(REG_IRQ_FLAGS);			// 0x12; read back flags

    cp_nb_rx_rcv++;											// Receive statistics counter

    //  Check for payload IRQ_LORA_CRCERR_MASK=0x20 set
    if((irqflags & IRQ_LORA_CRCERR_MASK) == IRQ_LORA_CRCERR_MASK)
    {
#if DUSB>=2
        Serial.println(F("CRC"));
#endif
		// Reset CRC flag 0x20
        writeRegister(REG_IRQ_FLAGS, (uint8_t)(IRQ_LORA_CRCERR_MASK || IRQ_LORA_RXDONE_MASK));	// 0x12; clear CRC (== 0x20) flag
        return 0;
    }
	else {
        cp_nb_rx_ok++;										// Receive OK statistics counter

        uint8_t currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);	// 0x10
        uint8_t receivedCount = readRegister(REG_RX_NB_BYTES);	// 0x13; How many bytes were read
#if DUSB>=2
		if ((debug>=0) && (currentAddr > 64)) {
			Serial.print(F("receivePkt:: Rx addr>64"));
			Serial.println(currentAddr);
			if (debug>=2) Serial.flush();
		}
#endif
        writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) currentAddr);		// 0x0D 

		if (receivedCount > 64) {
#if DUSB>=2
			Serial.print(F("receivePkt:: receivedCount="));
			Serial.println(receivedCount);
			if (debug>=2) Serial.flush();
#endif
			receivedCount=64;
		}
#if DUSB>=2
		else if (debug>=2) {
			Serial.print(F("ReceivePkt:: addr="));
			Serial.print(currentAddr);
			Serial.print(F(", len="));
			Serial.println(receivedCount);
			if (debug>=2) Serial.flush();
		}
#endif
        for(int i = 0; i < receivedCount; i++)
        {
            payload[i] = readRegister(REG_FIFO);			// 0x00
        }

		writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);		// Reset ALL interrupts
		return(receivedCount);
    }
	
	writeRegister(REG_IRQ_FLAGS, (uint8_t) IRQ_LORA_RXDONE_MASK);	// 0x12; Clear RxDone IRQ_LORA_RXDONE_MASK
    return 0;
}
	
	
	
// ----------------------------------------------------------------------------
// This DOWN function sends a payload to the LoRa node over the air
// Radio must go back in standby mode as soon as the transmission is finished
// 
// NOTE:: writeRegister functions should not be used outside interrupts
// ----------------------------------------------------------------------------
bool sendPkt(uint8_t *payLoad, uint8_t payLength)
{
#if DUSB>=2
	if (payLength>=128) {
		if (debug>=1) {
			Serial.print("sendPkt:: len=");
			Serial.println(payLength);
		}
		return false;
	}
#endif
	writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_TX_BASE_AD));	// 0x0D, 0x0E
	
	writeRegister(REG_PAYLOAD_LENGTH, (uint8_t) payLength);				// 0x22
	//for(int i = 0; i < payLength; i++)
    //{
    //    writeRegister(REG_FIFO, (uint8_t) payLoad[i]);					// 0x00
    //}
	writeBuffer(REG_FIFO, (uint8_t *) payLoad, payLength);
	return true;
}

// ----------------------------------------------------------------------------
// loraWait()
// This function implements the wait protocol needed for downstream transmissions.
// Note: Timing of downstream and JoinAccept messages is VERY critical.
//
// As the ESP8266 watchdog will not like us to wait more than a few hundred
// milliseconds (or it will kick in) we have to implement a simple way to wait
// time in case we have to wait seconds before sending messages (e.g. for OTAA 5 or 6 seconds)
// Without it, the system is known to crash in half of the cases it has to wait for 
// JOIN-ACCEPT messages to send.
//
// This function uses a combination of delay() statements and delayMicroseconds().
// As we use delay() only when there is still enough time to wait and we use micros()
// to make sure that delay() did not take too much time this works.
// 
// Parameter: uint32-t tmst gives the micros() value when transmission should start.
// ----------------------------------------------------------------------------

void loraWait(uint32_t tmst)
{
#if DUSB>=2
	uint32_t startTime = micros();						// Start of the loraWait function
#endif
	tmst += txDelay;
	uint32_t waitTime = tmst - micros();
		
	while (waitTime > 16000) {
		delay(15);										// ms delay including yield, slightly shorter
		waitTime= tmst - micros();
	}
	if (waitTime>0) delayMicroseconds(waitTime);
#if DUSB>=2
	else if ((waitTime+20) < 0) {
		Serial.println(F("loraWait TOO LATE"));
	}
	
	if (debug >=1) { 
		Serial.print(F("start: ")); 
		Serial.print(startTime);
		Serial.print(F(", end: "));
		Serial.print(tmst);
		Serial.print(F(", waited: "));
		Serial.print(tmst - startTime);
		Serial.print(F(", delay="));
		Serial.print(txDelay);
		Serial.println();
		if (debug>=2) Serial.flush();
	}
#endif
}


// ----------------------------------------------------------------------------
// txLoraModem
// Init the transmitter and transmit the buffer
// After successful transmission (dio0==1) TxDone re-init the receiver
//
//	crc is set to 0x00 for TX
//	iiq is set to 0x27 (or 0x40 based on ipol value in txpkt)
//
//	1. opmode Lora (only in Sleep mode)
//	2. opmode StandBY
//	3. Configure Modem
//	4. Configure Channel
//	5. write PA Ramp
//	6. config Power
//	7. RegLoRaSyncWord LORA_MAC_PREAMBLE
//	8. write REG dio mapping (dio0)
//	9. write REG IRQ flags
// 10. write REG IRQ mask
// 11. write REG LoRa Fifo Base Address
// 12. write REG LoRa Fifo Addr Ptr
// 13. write REG LoRa Payload Length
// 14. Write buffer (byte by byte)
// 15. Wait until the right time to transmit has arrived
// 16. opmode TX
// ----------------------------------------------------------------------------

void txLoraModem(uint8_t *payLoad, uint8_t payLength, uint32_t tmst, uint8_t sfTx,
						uint8_t powe, uint32_t freq, uint8_t crc, uint8_t iiq)
{
#if DUSB>=2
	if (debug>=1) {
		// Make sure that all serial stuff is done before continuing
		Serial.print(F("txLoraModem::"));
		Serial.print(F("  powe: ")); Serial.print(powe);
		Serial.print(F(", freq: ")); Serial.print(freq);
		Serial.print(F(", crc: ")); Serial.print(crc);
		Serial.print(F(", iiq: 0X")); Serial.print(iiq,HEX);
		Serial.println();
		if (debug>=2) Serial.flush();
	}
#endif
	_state = S_TX;
		
	// 1. Select LoRa modem from sleep mode
	//opmode(OPMODE_LORA);									// set register 0x01 to 0x80
	
	// Assert the value of the current mode
	ASSERT((readRegister(REG_OPMODE) & OPMODE_LORA) != 0);
	
	// 2. enter standby mode (required for FIFO loading))
	opmode(OPMODE_STANDBY);									// set 0x01 to 0x01
	
	// 3. Init spreading factor and other Modem setting
	setRate(sfTx, crc);
	
	// Frquency hopping
	//writeRegister(REG_HOP_PERIOD, (uint8_t) 0x00);		// set 0x24 to 0x00 only for receivers
	
	// 4. Init Frequency, config channel
	setFreq(freq);

	// 6. Set power level, REG_PAC
	setPow(powe);
	
	// 7. prevent node to node communication
	writeRegister(REG_INVERTIQ, (uint8_t) iiq);						// 0x33, (0x27 or 0x40)
	
	// 8. set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP (or lesss for 1ch gateway)
    writeRegister(REG_DIO_MAPPING_1, (uint8_t)(MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP));
	
	// 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);
	
	// 10. mask all IRQs but TxDone
    writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~IRQ_LORA_TXDONE_MASK);
	
	// txLora
	opmode(OPMODE_FSTX);									// set 0x01 to 0x02 (actual value becomes 0x82)
	
	// 11, 12, 13, 14. write the buffer to the FiFo
	sendPkt(payLoad, payLength);

	// 15. wait extra delay out. The delayMicroseconds timer is accurate until 16383 uSec.
	loraWait(tmst);
	
	//Set the base addres of the transmit buffer in FIFO
    writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_TX_BASE_AD));	// set 0x0D to 0x0F (contains 0x80);	
	
	//For TX we have to set the PAYLOAD_LENGTH
    writeRegister(REG_PAYLOAD_LENGTH, (uint8_t) payLength);		// set 0x22, max 0x40==64Byte long
	
	//For TX we have to set the MAX_PAYLOAD_LENGTH
    writeRegister(REG_MAX_PAYLOAD_LENGTH, (uint8_t) MAX_PAYLOAD_LENGTH);	// set 0x22, max 0x40==64Byte long
	
	// Reset the IRQ register
	//writeRegister(REG_IRQ_FLAGS, 0xFF);						// set 0x12 to 0xFF
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);			// Clear the mask
	writeRegister(REG_IRQ_FLAGS, (uint8_t) IRQ_LORA_TXDONE_MASK);// set 0x12 to 0x08
	
	// 16. Initiate actual transmission of FiFo
	opmode(OPMODE_TX);											// set 0x01 to 0x03 (actual value becomes 0x83)
	
}// txLoraModem


// ----------------------------------------------------------------------------
// Setup the LoRa receiver on the connected transceiver.
// - Determine the correct transceiver type (sx1272/RFM92 or sx1276/RFM95)
// - Set the frequency to listen to (1-channel remember)
// - Set Spreading Factor (standard SF7)
// The reset RST pin might not be necessary for at least the RGM95 transceiver
//
// 1. Put the radio in LoRa mode
// 2. Put modem in sleep or in standby
// 3. Set Frequency
// 4. Spreading Factor
// 5. Set interrupt mask
// 6. Clear all interrupt flags
// 7. Set opmode to OPMODE_RX
// 8. Set _state to S_RX
// 9. Reset all interrupts
// ----------------------------------------------------------------------------

void rxLoraModem()
{
	// 1. Put system in LoRa mode
	//opmode(OPMODE_LORA);
	
	// 2. Put the radio in sleep mode
	opmode(OPMODE_STANDBY);
    //opmode(OPMODE_SLEEP);										// set 0x01 to 0x00
	
	// 3. Set frequency based on value in freq
	setFreq(freqs[ifreq]);										// set to 868.1MHz

	// 4. Set spreading Factor and CRC
    setRate(sf, 0x04);
	
	// prevent node to node communication
	writeRegister(REG_INVERTIQ, (uint8_t) 0x27);				// 0x33, 0x27; to reset from TX
	
	// Max Payload length is dependent on 256 byte buffer. 
	// At startup TX starts at 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
	//For TX we have to set the PAYLOAD_LENGTH
    //writeRegister(REG_PAYLOAD_LENGTH, (uint8_t) PAYLOAD_LENGTH);	// set 0x22, 0x40==64Byte long

	// Set CRC Protection or MAX payload protection
    //writeRegister(REG_MAX_PAYLOAD_LENGTH, (uint8_t) MAX_PAYLOAD_LENGTH);	// set 0x23 to 0x80==128
	
	//Set the start address for the FiFO (Which should be 0)
    writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_RX_BASE_AD));	// set 0x0D to 0x0F (contains 0x00);
	
    // Low Noise Amplifier used in receiver
    writeRegister(REG_LNA, (uint8_t) LNA_MAX_GAIN);  						// 0x0C, 0x23
	
	// Accept no interrupts except RXDONE, RXTOUT en RXCRC
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK | IRQ_LORA_CRCERR_MASK));

	// set frequency hopping
	if (_hop) {
#if DUSB>=1
		if (debug >= 1) { 
			Serial.print(F("rxLoraModem:: Hop, channel=")); 
			Serial.println(ifreq); 
		}
#endif
		writeRegister(REG_HOP_PERIOD, 0x01);					// 0x24, 0x01 was 0xFF
		// Set RXDONE interrupt to dio0
		writeRegister(REG_DIO_MAPPING_1, (uint8_t)(MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT | MAP_DIO1_LORA_FCC));
	}
	else {
		writeRegister(REG_HOP_PERIOD,0x00);						// 0x24, 0x00 was 0xFF
		// Set RXDONE interrupt to dio0
		writeRegister(REG_DIO_MAPPING_1, (uint8_t)(MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT));
	}
	

	// Set the opmode to either single or continuous receive. The first is used when
	// every message can come on a different SF, the second when we have fixed SF
	if (_cad) {
		// cad Scanner setup, set _state to S_RX
		// Set Single Receive Mode, goes in STANDBY mode after receipt
		_state = S_RX;
		opmode(OPMODE_RX_SINGLE);								// 0x80 | 0x06 (listen one message)
	}
	else {
		// Set Continous Receive Mode, usefull if we stay on one SF
		_state= S_RX;
		opmode(OPMODE_RX);										// 0x80 | 0x05 (listen)
	}
	
	// 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);
	
	return;
}// rxLoraModem


// ----------------------------------------------------------------------------
// function cadScanner()
//
// CAD Scanner will scan on the given channel for a valid Symbol/Preamble signal.
// So instead of receiving continuous on a given channel/sf combination
// we will wait on the given channel and scan for a preamble. Once received
// we will set the radio to the SF with best rssi (indicating reception on that sf).
// The function sets the _state to S_SCAN
// NOTE: DO not set the frequency here but use the frequency hopper
// ----------------------------------------------------------------------------
void cadScanner()
{
	// 1. Put system in LoRa mode (which destroys all other nodes(
	//opmode(OPMODE_LORA);
	
	// 2. Put the radio in sleep mode
	opmode(OPMODE_STANDBY);										// Was old value
	//opmode(OPMODE_SLEEP);										// set 0x01 to 0x00
	
	// As we can come back from S_TX with other frequencies and SF
	// reset both to good values for cadScanner
	
	// 3. Set frequency based on value in freq					// XXX New, might be needed when receiving down
#if DUSB>=2
	if ((debug>=1) && (ifreq>9)) {
		Serial.print(F("cadScanner:: E Freq="));
		Serial.println(ifreq);
		if (debug>=2) Serial.flush();
		ifreq=0;
	}
#endif
	setFreq(freqs[ifreq]);

	// For every time we stat the scanner, we set the SF to the begin value
	sf = SF7;													// we make SF the lower, this is faster!
	
	// 4. Set spreading Factor and CRC
	setRate(sf, 0x04);
	
	// listen to LORA_MAC_PREAMBLE
	writeRegister(REG_SYNC_WORD, (uint8_t) 0x34);				// set reg 0x39 to 0x34
	
	// Set the interrupts we want top listen to
	writeRegister(REG_DIO_MAPPING_1,
		(uint8_t)(MAP_DIO0_LORA_CADDONE | MAP_DIO1_LORA_CADDETECT | MAP_DIO2_LORA_NOP | MAP_DIO3_LORA_NOP));
	
	// Set the mask for interrupts (we do not want to listen to) except for
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~(IRQ_LORA_CDDONE_MASK | IRQ_LORA_CDDETD_MASK | IRQ_LORA_CRCERR_MASK));
	
	// Set the opMode to CAD
	opmode(OPMODE_CAD);

	// Clear all relevant interrupts
	//writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF );						// May work better, clear ALL interrupts
	
	// If we are here. we either might have set the SF or we have a timeout in which
	// case the receive is started just as normal.
	return;
}// cadScanner


// ----------------------------------------------------------------------------
// First time initialisation of the LoRa modem
// Subsequent changes to the modem state etc. done by txLoraModem or rxLoraModem
// After initialisation the modem is put in rx mode (listen)
// ----------------------------------------------------------------------------
void initLoraModem()
{
	_state = S_INIT;
	// Reset the transceiver chip with a pulse of 10 mSec
#ifdef ESP32BUILD
	digitalWrite(pins.rst, LOW);
	delayMicroseconds(10000);
  digitalWrite(pins.rst, HIGH);
	delayMicroseconds(10000);
#else
  digitalWrite(pins.rst, HIGH);
  delayMicroseconds(10000);
  digitalWrite(pins.rst, LOW);
  delayMicroseconds(10000);
#endif
  digitalWrite(pins.ss, HIGH);

  // Check chip version first
    uint8_t version = readRegister(REG_VERSION);        // Read the LoRa chip version id
    if (version == 0x22) {
        // sx1272
#if DUSB>=2
        Serial.println(F("WARNING:: SX1272 detected"));
#endif
        sx1272 = true;
    } 
  else if (version == 0x12) {
        // sx1276?
#if DUSB>=2
            if (debug >=1) 
        Serial.println(F("SX1276 starting"));
#endif
            sx1272 = false;
  } 
  else {
#if DUSB>=2
    Serial.print(F("Unknown transceiver="));
    Serial.println(version,HEX);
#endif
    //die("");
    }

	// 2. Set radio to sleep
	opmode(OPMODE_SLEEP);										// set register 0x01 to 0x00

	// 1 Set LoRa Mode
	opmode(OPMODE_LORA);										// set register 0x01 to 0x80
	
	// 3. Set frequency based on value in freq
	ifreq = 0; 
	freq=freqs[0];
	setFreq(freq);												// set to 868.1MHz
	
	// 4. Set spreading Factor
    setRate(sf, 0x04);
	
	// Low Noise Amplifier used in receiver
    writeRegister(REG_LNA, (uint8_t) LNA_MAX_GAIN);  						// 0x0C, 0x23
	
	// 7. set sync word
    writeRegister(REG_SYNC_WORD, (uint8_t) 0x34);				// set 0x39 to 0x34 LORA_MAC_PREAMBLE
	
	// prevent node to node communication
	writeRegister(REG_INVERTIQ,0x27);							// 0x33, 0x27; to reset from TX
	
	// Max Payload length is dependent on 256 byte buffer. At startup TX starts at
	// 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
    writeRegister(REG_MAX_PAYLOAD_LENGTH,MAX_PAYLOAD_LENGTH);	// set 0x23 to 0x80==128 bytes
    writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);			// 0x22, 0x40==64Byte long
	
    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD));	// set reg 0x0D to 0x0F
	writeRegister(REG_HOP_PERIOD,0x00);							// reg 0x24, set to 0x00 was 0xFF
	if (_hop) {
		writeRegister(REG_HOP_CHANNEL,0x00);					// reg 0x1C, set to 0x00
	}

	// 5. Config PA Ramp up time								// set reg 0x0A  
	writeRegister(REG_PARAMP, (readRegister(REG_PARAMP) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
	
	// Set 0x4D PADAC for SX1276 ; XXX register is 0x5a for sx1272
	writeRegister(REG_PADAC_SX1276,  0x84); 					// set 0x4D (PADAC) to 0x84
	//writeRegister(REG_PADAC, readRegister(REG_PADAC)|0x4);
	
	// Reset interrupt Mask, enable all interrupts
	writeRegister(REG_IRQ_FLAGS_MASK, 0x00);
	
	// 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);
}// initLoraModem



// ----------------------------------------------------------------------------
// stateMachine handler of the state machine.
// We use ONE state machine for all kind of interrupts. This assures that we take
// the correct action upon receiving an interrupt.
//
// STATE MACHINE
// The program uses the following state machine (in _state), all states
// are done in interrupt routine, only the follow-up of S-RXDONE is done
// in the main loop() program. This is because otherwise the interrupt processing
// would take too long to finish
//
// S-INIT=0, 	The commands in this state are executed only once
//	- Goto S_SCAN
//
// S-SCAN, 		CadScanner() part
//	- Upon CDDECT (int1) goto S_RX, 
//	- upon CDDONE (int0) got S_CAD, walk through all SF until CDDETD
//	- Else stay in SCAN state
//
// S-CAD, 		
//	- Upon CDDECT (int1) goto S_RX, 
//	- Upon CDDONE (int0) goto S_SCAN, start with SF7 recognition again
//
// S-RX, 		Received CDDECT so message detected, RX cycle started. 
//	- Upon RXDONE (int0) package read. If read ok continue, 
//	- upon RXTOUT (int1) goto S_SCAN
//
// S-RXDONE Buffer reading is complete
//	- Wait for reading in loop()
//	- Upon message send to server goto S_SCAN
//
// S-TX			Transmitting a message
//	- Upon TX goto S_SCAN
//
// S-TXDONE		Transmission complete by loop() now again in interrupt
//	- Set the Mask
//	- reset the Flags
//	- Goto either SCAN or RX
//
// This interrupt routine has been kept as simple and short as possible.
// If we receive an interrupt that does not below to a _state then print error.
// 
// NOTE: We may clear the interrupt but leave the flag for the moment. 
//	The eventHandler should take care of repairing flags between interrupts.
// ----------------------------------------------------------------------------

void stateMachine()
{
	// Make a sort of mutex by using a volatile variable
#if MUTEX_INT==1	
	if(!GetMutex(&inIntr)) {
#if DUSB>=1
		if (debug>=0) {
			Serial.println(F("eInt:: Mutex"));
			if (debug>=2) Serial.flush();
		}
#endif
		return;
	}
#endif
	// Determine what interrupt flags are set
	uint8_t flags = readRegister(REG_IRQ_FLAGS);
	uint8_t mask  = readRegister(REG_IRQ_FLAGS_MASK);
	uint8_t intr  = flags & ( ~ mask );				// Only react on non masked interrupts
	uint8_t rssi;
	
	if (intr == 0x00) {
#if DUSB>=1
		// Something strange has happened: There has been an event
		// and we do not have a value for interrupt.
		if (debug>=1) Serial.println(F("stateMachine:: NO intr"));
#endif

		// Mayby wait a little before resetting all/
#if MUTEX_INT==1
		ReleaseMutex(&inIntr);
#endif		
		//_state = S_SCAN;
		writeRegister(REG_IRQ_FLAGS, 0xFF );		// Clear ALL interrupts
		_event = 0;
		return;
	}
	
	// Small state machine inside the interrupt handler
	// as next actions are depending on the state we are in.
	switch (_state) 
	{
	
	  // --------------------------------------------------------------
	  // If the state is init, we are starting up.
	  // The initLoraModem() function is already called ini setup();
	  case S_INIT:
#if DUSB>=2
		if (debug >= 1) { 
			Serial.println(F("S_INIT")); 
		}
#endif
		// new state, needed to startup the radio (to S_SCAN)
		writeRegister(REG_IRQ_FLAGS, 0xFF );		// Clear ALL interrupts
	  break;

	  
	  // --------------------------------------------------------------
	  // In S_SCAN we measure a high RSSI this means that there (probably) is a message
	  // coming in at that freq. But not necessarily on the current SF.
	  // If so find the right SF with CDDETD. 
	  case S_SCAN:
	    //
		// Intr=IRQ_LORA_CDDETD_MASK
		// We detected a message on this frequency and SF when scanning
		// We clear both CDDETD and CDDONE and swich to reading state
		//
		if (intr & IRQ_LORA_CDDETD_MASK) {
#if DUSB>=2
			if (debug >=3) {
				Serial.println(F("SCAN:: CADDETD, "));
			}
#endif

			_state = S_RX;								// Set state to receiving
			opmode(OPMODE_RX_SINGLE);					// set reg 0x01 to 0x06

			// Set RXDONE interrupt to dio0, RXTOUT to dio1
			writeRegister(REG_DIO_MAPPING_1, (MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT));
			
			// Since new state is S_RX, accept no interrupts except RXDONE or RXTOUT		
			writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK));
			
			delayMicroseconds( RSSI_WAIT_DOWN );		// Wait some microseconds less
			// Starting with version 5.0.1 the waittime is dependent on the SF
			// So for SF12 we wait longer (2^7 == 128 uSec) and for SF7 4 uSec.
			//delayMicroseconds( (0x01 << ((uint8_t)sf - 5 )) );
			rssi = readRegister(REG_RSSI);				// Read the RSSI
			_rssi = rssi;								// Read the RSSI in the state variable

			writeRegister(REG_IRQ_FLAGS, 0xFF );		// reset all interrupt flags
		}//if

		// CDDONE
		// We received a CDDONE int telling us that we received a message on this
		// frequency and possibly on one of its SF.
		// If so, we switch to CAD state where we will only listen to CDDETD event.
		//
		else if (intr & IRQ_LORA_CDDONE_MASK) {

			opmode(OPMODE_CAD);
		
			rssi = readRegister(REG_RSSI);				// Read the RSSI

			// We choose the generic RSSI as a sorting mechanism for packages/messages
			// The pRSSI (package RSSI) is caluclated upon successful reception of message
			// So we expect tht this value makes little sense for the moment with CDDONE.
			// Set the rssi as low as the noise floor. Lower values are not recognized then.
			//
			if ( rssi > RSSI_LIMIT ) {					// Is set to 37 (27/08/2017)
				_state = S_CAD;							// XXX invoke the interrupt handler again?
			}

			// Clear the CADDONE flag
			//writeRegister(REG_IRQ_FLAGS, IRQ_LORA_CDDONE_MASK);
			writeRegister(REG_IRQ_FLAGS, 0xFF);
		}
		
		// If not CDDETC and not CDDONE and sf==12 we have to hop
		else if ((_hop) && (sf==12)) {
				hop();
				sf=(sf_t)7;
		}
		
		// If we do not switch to S_CAD, we have to hop
		// Instead of waiting for an interrupt we do this on timer basis (more regular).
		else {
			_state=S_SCAN;
			cadScanner();
			writeRegister(REG_IRQ_FLAGS, 0xFF);
		}
		// else keep on scanning
	  break; // S_SCAN

	  
	  // --------------------------------------------------------------
	  // S_CAD: In CAD mode we scan every SF for high RSSI until we have a DETECT.
	  // Reason is the we received a CADDONE interrupt so we know a message is received
	  // on the frequency but may be on another SF.
	  //
	  // If message is of the right frequency and SF, IRQ_LORA_CDDETD_MSAK interrupt
	  // is raised, indicating that we can start beging reading the message from SPI.
	  //
	  // DIO0 interrupt IRQ_LORA_CDDONE_MASK in state S_CAD==2 means that we might have
	  // a lock on the Freq but not the right SF. So we increase the SF
	  //
	  case S_CAD:

		// Intr=IRQ_LORA_CDDETD_MASK
		// We have to set the sf based on a strong RSSI for this channel
		//
		if (intr & IRQ_LORA_CDDETD_MASK) {

			_state = S_RX;								// Set state to start receiving
			opmode(OPMODE_RX_SINGLE);					// set reg 0x01 to 0x06, initiate READ
			
			// Set RXDONE interrupt to dio0, RXTOUT to dio1
			writeRegister(REG_DIO_MAPPING_1, (MAP_DIO0_LORA_RXDONE | MAP_DIO1_LORA_RXTOUT));
			
			// Accept no interrupts except RXDONE or RXTOUT
			writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~(IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK));

			delayMicroseconds( RSSI_WAIT_DOWN );		// Wait some microseconds less
			//delayMicroseconds( (0x01 << ((uint8_t)sf - 5 )) );
			rssi = readRegister(REG_RSSI);				// Read the RSSI
			_rssi = rssi;								// Read the RSSI in the state variable

			//writeRegister(REG_IRQ_FLAGS, IRQ_LORA_CDDETD_MASK | IRQ_LORA_RXDONE_MASK);
			writeRegister(REG_IRQ_FLAGS, 0xFF );		// reset all CAD Detect interrupt flags
		}// CDDETD
		
		// Intr == CADDONE
		// So we scan this SF and if not high enough ... next
		//
		else if (intr & IRQ_LORA_CDDONE_MASK) {
			// If this is not SF12, increment the SF and try again
			// We expect on other SF get CDDETD
			//
			if (((uint8_t)sf) < SF12) {
				sf = (sf_t)((uint8_t)sf+1);				// XXX This would mean SF7 never used
				setRate(sf, 0x04);						// Set SF with CRC==on

				opmode(OPMODE_CAD);						// Scanning mode
				
				delayMicroseconds(RSSI_WAIT );
				rssi = readRegister(REG_RSSI);			// Read the RSSI
				
				// reset interrupt flags for CAD Done
				writeRegister(REG_IRQ_FLAGS, IRQ_LORA_CDDONE_MASK | IRQ_LORA_CDDETD_MASK);
				//writeRegister(REG_IRQ_FLAGS, 0xFF );	// XXX this will prevent the CDDETD from being read
				
			}
			// If we reach SF12, we should go back to SCAN state
			else {
				if (_hop) { hop(); }					// if HOP we start at the next frequency
				_state = S_SCAN;
				cadScanner();							// Which will reset SF to SF7
				// Reset the interrupts
				writeRegister(REG_IRQ_FLAGS, IRQ_LORA_CDDONE_MASK);
			}
		} //CADDONE

		//
		// if this interrupt is not CDECT or CDDONE then the interrupt is
		// is unknown in this state. So we clear interrupt and give a warning.
		else {
#if DUSB>=2
			if (debug>=1) { 
				Serial.println(F("CAD:: Unknown interrupt")); 
			}
#endif
			_state = S_SCAN;
			cadScanner();
			writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);			// Reset all interrupts
		}
	  break; //S_CAD

	  
	  // --------------------------------------------------------------
	  // If we receive an interrupt on dio0 state==S_RX
	  // it should be a RxDone interrupt
	  // So we should handle the received message
	  case S_RX:
	
		if (intr & IRQ_LORA_RXDONE_MASK) {

			// We have to check for CRC error which will be visible AFTER RXDONE is set.
			// CRC errors might indicate tha the reception is not OK.
			// Could be CRC error or message too large.
			// CRC error checking requires DIO3
			if (intr & IRQ_LORA_CRCERR_MASK) {
#if DUSB>=2
				Serial.println(F("CRC err"));
				if (debug>=2) Serial.flush();
#endif
				if (_cad) {
					_state = S_SCAN;
					cadScanner();
				}
				else {
					_state = S_RX;
					rxLoraModem();
				}
				
				writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);	// Reset the interrupt mask
				// Reset interrupts
				writeRegister(REG_IRQ_FLAGS, (uint8_t)(IRQ_LORA_CRCERR_MASK | IRQ_LORA_RXDONE_MASK | IRQ_LORA_RXTOUT_MASK));
				break;
			}

			if((LoraUp.payLength = receivePkt(LoraUp.payLoad)) <= 0) {
#if DUSB>=1
				if (debug>=0) {
					Serial.println(F("sMachine:: Error S-RX"));
				}
#endif
			}
				
			// Do all register processing in this section (interrupt)
			uint8_t value = readRegister(REG_PKT_SNR_VALUE);	// 0x19; 
			if( value & 0x80 ) {								// The SNR sign bit is 1
				
				value = ( ( ~value + 1 ) & 0xFF ) >> 2;			// Invert and divide by 4
				LoraUp.snr = -value;
			}
			else {
				// Divide by 4
				LoraUp.snr = ( value & 0xFF ) >> 2;
			}
	
			LoraUp.prssi = readRegister(REG_PKT_RSSI);			// read register 0x1A, packet rssi
    
			// Correction of RSSI value based on chip used.	
			if (sx1272) {										// Is it a sx1272 radio?
				LoraUp.rssicorr = 139;
			} else {											// Probably SX1276 or RFM95
				LoraUp.rssicorr = 157;
			}
				
			LoraUp.sf = readRegister(REG_MODEM_CONFIG2) >> 4;

			if (receivePacket() <= 0) {							// read is not successful
#if DUSB>=1
				Serial.println(F("sMach:: Error receivePacket"));
#endif
			}
#if DUSB>=2
			else if (debug>=2) {
				Serial.println(F("sMach:: receivePacket OK"));
			}
#endif
			
			// Set the modem to receiving BEFORE going back to
			// user space.
			if (_cad) {
				_state = S_SCAN;
				cadScanner();
			}
			else {
				_state = S_RX;
				rxLoraModem();
			}
			
			writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
			writeRegister(REG_IRQ_FLAGS, 0xFF);			// Reset the interrupt mask
		}
		
		// Receive message timeout
		else if (intr & IRQ_LORA_RXTOUT_MASK) {
			
			// Set the modem for next receive action. This must be done before
			// scanning takes place as we cannot do it once RXDETTD is set.
			
			if (_cad) {
				// Set the state to CAD scanning
				_state = S_SCAN;
				cadScanner();							// Start the scanner after RXTOUT
			}
			else {
				_state = S_RX;							// XXX 170828, why?
				rxLoraModem();			
			}
			writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00 );
			writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);			// reset all interrupts
		}

		// The interrupt received is not RXDONE nor RXTOUT
		// therefore we restart the scanning sequence (catch all)
		else {
#if DUSB>=2
			if (debug >=3) {
				Serial.println(F("S_RX:: No RXDONE/RXTOUT, "));
			}
#endif
			initLoraModem();							// Reset all communication,3
			if (_cad) {
				_state = S_SCAN;
				cadScanner();
			}
			else {
				_state = S_RX;
				rxLoraModem();			
			}
			writeRegister(REG_IRQ_FLAGS_MASK, 0x00);	// Reset all masks
			writeRegister(REG_IRQ_FLAGS, 0xFF);			// Reset all interrupts
		}
	  break; // S_RX

	  
	  // --------------------------------------------------------------  

	  //
	  case S_TX:
	  
	  	// Initiate the transmission of the buffer (in Interrupt space)
		// We react on ALL interrupts if we are in TX state.
		
		txLoraModem(
			LoraDown.payLoad,
			LoraDown.payLength,
			LoraDown.tmst,
			LoraDown.sfTx,
			LoraDown.powe,
			LoraDown.fff,
			LoraDown.crc,
			LoraDown.iiq
		);
		
#if DUSB>=2
		if (debug>=0) { 
			Serial.println(F("S_TX, ")); 
		}
#endif
		_state = S_TXDONE;
		writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
		writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);				// reset interrupt flags
		
	  break; // S_TX
	  
	  // ---------------------------------------------------
	  // AFter the transmission is completed by the hardware, 
	  // the interrupt TXDONE is raised telling us that the tranmission
	  // was successful.
	  // If we receive an interrupt on dio0 _state==S_TX it is a TxDone interrupt
	  // Do nothing with the interrupt, it is just an indication.
	  // sendPacket switch back to scanner mode after transmission finished OK
	  case S_TXDONE:
		if (intr & IRQ_LORA_TXDONE_MASK) {
#if DUSB>=1
			Serial.println(F("TXDONE interrupt"));
#endif
			// After transmission reset to receiver
			if (_cad) {
				// Set the state to CAD scanning
				_state = S_SCAN;
				cadScanner();								// Start the scanner after TX cycle
			}
			else {
				_state = S_RX;
				rxLoraModem();		
			}
		
			writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
			writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);				// reset interrupt flags
#if DUSB>=1
			if (debug>=1) {
				Serial.println(F("TXDONE handled"));
				if (debug>=2) Serial.flush();
			}
#endif
		}
		else {
#if DUSB>=1
			if (debug>=0) {
				Serial.println(F("TXDONE unknown interrupt"));
				if (debug>=2) Serial.flush();
			}
#endif
		}
	  break; // S_TXDONE	  

	  
	  // --------------------------------------------------------------
	  // If _STATE is in undefined state
	  // If such a thing happens, we should re-init the interface and 
	  // make sure that we pick up next interrupt
	  default:
#if DUSB>=2
		if (debug >= 2) { 
			Serial.print("E state="); 
			Serial.println(_state);	
		}
#endif
		if (_cad) {
			_state = S_SCAN;
			cadScanner();
		}
		else
		{
			_state = S_RX;
			rxLoraModem();
		}
		writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
		writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);				// Reset all interrupts
	  break;
	}
	
#if MUTEX_INT==1
	ReleaseMutex(&inIntr);
#endif
	_event = 0;
	return;
}


// ----------------------------------------------------------------------------
// Interrupt_0 Handler.
// Both interrupts DIO0 and DIO1 are mapped on GPIO15. Se we have to look at 
// the interrupt flags to see which interrupt(s) are called.
//
// NOTE:: This method may work not as good as just using more GPIO pins on 
//  the ESP8266 mcu. But in practice it works good enough
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR Interrupt_0()
{
	_event = 1;
}


// ----------------------------------------------------------------------------
// Interrupt handler for DIO1 having High Value
// As DIO0 and DIO1 may be multiplexed on one GPIO interrupt handler
// (as we do) we have to be careful only to call the right Interrupt_x
// handler and clear the corresponding interrupts for that dio.
// NOTE: Make sure all Serial communication is only for debug level 3 and up.
// Handler for:
//		- CDDETD
//		- RXTIMEOUT
//		- (RXDONE error only)
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR Interrupt_1()
{
	_event = 1;
}

// ----------------------------------------------------------------------------
// Frequency Hopping Channel (FHSS) dio2
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR Interrupt_2() 
{
	_event = 1;
}


