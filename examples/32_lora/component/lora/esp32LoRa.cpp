/*
 *  Library for LoRa 868 / 915MHz SX1272 LoRa module
 *  
 *  Copyright (C) Libelium Comunicaciones Distribuidas S.L. 
 *  http://www.libelium.com 
 *  
 *  This program is free software: you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation, either version 3 of the License, or 
 *  (at your option) any later version. 
 *  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License 
 *  along with this program.  If not, see http://www.gnu.org/licenses/. 
 *  
 *  Version:           1.4
 *  Design:            David Gascón 
 *  Implementation:    Covadonga Albiñana, Victor Boria, Ruben Martin
 */

//**********************************************************************
// Includes
//**********************************************************************
#include "arduPiLoRa.h"

#include "arduPiClasses.h"

//#include "bcm2835.h"

//**********************************************************************
// Public functions.
//**********************************************************************

/*
 Function: Sets the module ON.
 Returns: uint8_t setLORA state
*/
uint8_t SX1272::ON()
{

  uint8_t state = 2;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'ON'\n");
  #endif

  // Inital Reset Sequence

  // 1.- power ON embebed socket
  Utils.socketON();

  // 2.- reset pulse for LoRa module initialization
  pinMode(LORA_RESET_PIN, OUTPUT);
  digitalWrite(LORA_RESET_PIN, HIGH);
  delay(100);

  digitalWrite(LORA_RESET_PIN, LOW);
  delay(100);

  // 3.- SPI chip select
  pinMode(SX1272_SS,OUTPUT);
  digitalWrite(SX1272_SS,HIGH);
  delayMicroseconds(100);
 
  //Configure the MISO, MOSI, CS, SPCR.
  SPI.begin();
  //Set Most significant bit first
  SPI.setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  //Divide the clock frequency
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  //Set data mode
  SPI.setDataMode(SPI_MODE0);
  delayMicroseconds(100);
  setMaxCurrent(0x1B);
  #if (SX1272_debug_mode > 1)
	  printf("## Setting ON with maximum current supply ##\n");
	  printf("\n");
  #endif

  // set LoRa mode
  state = setLORA();

	//Set initialization values
	writeRegister(0x0,0x0);
	writeRegister(0x1,0x81);
	writeRegister(0x2,0x1A);
	writeRegister(0x3,0xB);
	writeRegister(0x4,0x0);
	writeRegister(0x5,0x52);
	writeRegister(0x6,0xD8);
	writeRegister(0x7,0x99);
	writeRegister(0x8,0x99);
	writeRegister(0x9,0x0);
	writeRegister(0xA,0x9);
	writeRegister(0xB,0x3B);
	writeRegister(0xC,0x23);
	writeRegister(0xD,0x1);
	writeRegister(0xE,0x80);
	writeRegister(0xF,0x0);
	writeRegister(0x10,0x0);
	writeRegister(0x11,0x0);
	writeRegister(0x12,0x0);
	writeRegister(0x13,0x0);
	writeRegister(0x14,0x0);
	writeRegister(0x15,0x0);
	writeRegister(0x16,0x0);
	writeRegister(0x17,0x0);
	writeRegister(0x18,0x10);
	writeRegister(0x19,0x0);
	writeRegister(0x1A,0x0);
	writeRegister(0x1B,0x0);
	writeRegister(0x1C,0x0);
	writeRegister(0x1D,0x4A);
	writeRegister(0x1E,0x97);
	writeRegister(0x1F,0xFF);
	writeRegister(0x20,0x0);
	writeRegister(0x21,0x8);
	writeRegister(0x22,0xFF);
	writeRegister(0x23,0xFF);
	writeRegister(0x24,0x0);
	writeRegister(0x25,0x0);
	writeRegister(0x26,0x0);
	writeRegister(0x27,0x0);
	writeRegister(0x28,0x0);
	writeRegister(0x29,0x0);
	writeRegister(0x2A,0x0);
	writeRegister(0x2B,0x0);
	writeRegister(0x2C,0x0);
	writeRegister(0x2D,0x50);
	writeRegister(0x2E,0x14);
	writeRegister(0x2F,0x40);
	writeRegister(0x30,0x0);
	writeRegister(0x31,0x3);
	writeRegister(0x32,0x5);
	writeRegister(0x33,0x27);
	writeRegister(0x34,0x1C);
	writeRegister(0x35,0xA);
	writeRegister(0x36,0x0);
	writeRegister(0x37,0xA);
	writeRegister(0x38,0x42);
	writeRegister(0x39,0x12);
	writeRegister(0x3A,0x65);
	writeRegister(0x3B,0x1D);
	writeRegister(0x3C,0x1);
	writeRegister(0x3D,0xA1);
	writeRegister(0x3E,0x0);
	writeRegister(0x3F,0x0);
	writeRegister(0x40,0x0);
	writeRegister(0x41,0x0);
	writeRegister(0x42,0x22);
	
  return state;
}

/*
 Function: Sets the module OFF.
 Returns: Nothing
*/
void SX1272::OFF()
{
  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'OFF'\n");
  #endif

  // Powerdown sequence:

  // 1.- Close SPI
  SPI.end();

  // 2.- Chip Select OFF
  pinMode(SX1272_SS,OUTPUT);
  digitalWrite(SX1272_SS,LOW);

  // 3.- power OFF embebed socket
  Utils.socketOFF();

  #if (SX1272_debug_mode > 1)
	  printf("## Setting OFF ##\n");
	  printf("\n");
  #endif
}

/*
 Function: Reads the indicated register.
 Returns: The content of the register
 Parameters:
   address: address register to read from
*/
byte SX1272::readRegister(byte address)
{
	digitalWrite(SX1272_SS,LOW);
    bitClear(address, 7);		// Bit 7 cleared to write in registers
    //SPI.transfer(address);
    //value = SPI.transfer(0x00);
    txbuf[0] = address;
	txbuf[1] = 0x00;
	maxWrite16();
	digitalWrite(SX1272_SS,HIGH);

    #if (SX1272_debug_mode > 1)
        printf("## Reading:  ##\tRegister ");
		printf("%X", address);
		printf(":  ");
		printf("%X", rxbuf[1]);
		printf("\n");
	#endif

    return rxbuf[1];
}

/*
 Function: Writes on the indicated register.
 Returns: Nothing
 Parameters:
   address: address register to write in
   data : value to write in the register
*/
void SX1272::writeRegister(byte address, byte data)
{
	digitalWrite(SX1272_SS,LOW);
	delayMicroseconds(1);
    bitSet(address, 7);			// Bit 7 set to read from registers
    //SPI.transfer(address);
    //SPI.transfer(data);
    txbuf[0] = address;
	txbuf[1] = data;
	maxWrite16();
	//digitalWrite(SX1272_SS,HIGH);

    #if (SX1272_debug_mode > 1)
        printf("## Writing:  ##\tRegister ");
		bitClear(address, 7);
		printf("%X", address);
		printf(":  ");
		printf("%X", data);
		printf("\n");
	#endif

}

/*
 Function: It gets the temperature from the measurement block module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
void SX1272::maxWrite16()
{
	digitalWrite(SX1272_SS,LOW);
	SPI.transfernb(txbuf, rxbuf, 2);
	digitalWrite(SX1272_SS,HIGH);
}

/*
 Function: Clears the interruption flags
 Returns: Nothing
*/
void SX1272::clearFlags()
{
    byte st0;

	st0 = readRegister(REG_OP_MODE);		// Save the previous status

	if( _modem == LORA )
	{ // LoRa mode
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby mode to write in registers
		writeRegister(REG_IRQ_FLAGS, 0xFF);	// LoRa mode flags register
		writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
		#if (SX1272_debug_mode > 1)
			printf("## LoRa flags cleared ##\n");
		#endif
	}
	else
	{ // FSK mode
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Stdby mode to write in registers
		writeRegister(REG_IRQ_FLAGS1, 0xFF); // FSK mode flags1 register
		writeRegister(REG_IRQ_FLAGS2, 0xFF); // FSK mode flags2 register
		writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
		#if (SX1272_debug_mode > 1)
			printf("## FSK flags cleared ##\n");
		#endif
	}
}

/*
 Function: Sets the module in LoRa mode.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setLORA()
{
    uint8_t state = 2;
    byte st0;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setLORA'\n");
	#endif

	writeRegister(REG_OP_MODE, FSK_SLEEP_MODE);    // Sleep mode (mandatory to set LoRa mode)
	writeRegister(REG_OP_MODE, LORA_SLEEP_MODE);    // LoRa sleep mode
	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode

	writeRegister(REG_MAX_PAYLOAD_LENGTH,MAX_LENGTH);

	delayMicroseconds(100);

	st0 = readRegister(REG_OP_MODE);	// Reading config mode
	if( st0 == LORA_STANDBY_MODE )
	{ // LoRa mode
		_modem = LORA;
		state = 0;
		#if (SX1272_debug_mode > 1)
			printf("## LoRa set with success ##\n");
			printf("\n");
		#endif
	}
	else
	{ // FSK mode
		_modem = FSK;
		state = 1;
//		#if (SX1272_debug_mode > 1)
			printf("** There has been an error while setting LoRa **\n");
			printf("\n");
//		#endif
	}
	return state;
}

/*
 Function: Sets the module in FSK mode.
 Returns:   Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setFSK()
{
	uint8_t state = 2;
    byte st0;
    byte config1;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setFSK'\n");
	#endif

	if(	_modem = LORA )
	{
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
		writeRegister(REG_OP_MODE, LORA_SLEEP_MODE);
	}
	writeRegister(REG_OP_MODE, FSK_SLEEP_MODE);	// Sleep mode (mandatory to change mode)
	writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// FSK standby mode
	config1 = readRegister(REG_PACKET_CONFIG1);
	config1 = config1 & 0B01111101;		// clears bits 8 and 1 from REG_PACKET_CONFIG1
	config1 = config1 | 0B00000100;		// sets bit 2 from REG_PACKET_CONFIG1
	writeRegister(REG_PACKET_CONFIG1,config1);	// AddressFiltering = NodeAddress + BroadcastAddress
	writeRegister(REG_FIFO_THRESH, 0x80);	// condition to start packet tx
	config1 = readRegister(REG_SYNC_CONFIG);
	config1 = config1 & 0B00111111;
	writeRegister(REG_SYNC_CONFIG,config1);

	delayMicroseconds(100);

	st0 = readRegister(REG_OP_MODE);	// Reading config mode
	if( st0 == FSK_STANDBY_MODE )
	{ // FSK mode
		_modem = FSK;
		state = 0;
//		#if (SX1272_debug_mode > 1)
			printf("## FSK set with success ##\n");
			printf("\n");
//		#endif
	}
	else
	{ // LoRa mode
		_modem = LORA;
		state = 1;
		#if (SX1272_debug_mode > 1)
			printf("** There has been an error while setting FSK **\n");
			printf("\n");
		#endif
	}
	return state;
}

/*
 Function: Gets the bandwidth, coding rate and spreading factor of the LoRa modulation.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getMode()
{
  byte st0;
  int8_t state = 2;
  byte value = 0x00;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getMode'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);		// Save the previous status
  if( _modem == FSK )
  {
	  setLORA();					// Setting LoRa mode
  }
  value = readRegister(REG_MODEM_CONFIG1);
  _bandwidth = (value >> 6);   			// Storing 2 MSB from REG_MODEM_CONFIG1 (=_bandwidth)
  _codingRate = (value >> 3) & 0x07;  		// Storing third, forth and fifth bits from
  value = readRegister(REG_MODEM_CONFIG2);			// REG_MODEM_CONFIG1 (=_codingRate)
  _spreadingFactor = (value >> 4) & 0x0F; 	// Storing 4 MSB from REG_MODEM_CONFIG2 (=_spreadingFactor)
  state = 1;

  if( isBW(_bandwidth) )		// Checking available values for:
  {								//		_bandwidth
	if( isCR(_codingRate) )		//		_codingRate
	{							//		_spreadingFactor
	   if( isSF(_spreadingFactor) )
	   {
		   state = 0;
	   }
	}
  }

  #if (SX1272_debug_mode > 1)
	  printf("## Parameters from configuration mode are:\n");
	  printf("\t Bandwidth: ");
	  printf("%X", _bandwidth);
	  printf("\n");
	  printf("\t Coding Rate: ");
	  printf("%X", _codingRate);
	  printf("\n");
	  printf("\t Spreading Factor: ");
	  printf("%X", _spreadingFactor);
	  printf(" ##\n");
	  printf("\n");
  #endif

  writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  delayMicroseconds(100);
  return state;
}

/*
 Function: Sets the bandwidth, coding rate and spreading factor of the LoRa modulation.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
 Parameters:
   mode: mode number to set the required BW, SF and CR of LoRa modem.
*/
int8_t SX1272::setMode(uint8_t mode)
{
  int8_t state = 2;
  byte st0;
  byte config1 = 0x00;
  byte config2 = 0x00;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setMode'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);		// Save the previous status

  if( _modem == FSK )
  {
	  setLORA();
  }
	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode

    switch (mode)
    {
        // mode 1 (better reach, medium time on air)
        case 1:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_12);       // SF = 12
                    setBW(BW_125);      // BW = 125 KHz
                    break;
 
        // mode 2 (medium reach, less time on air)
        case 2:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_12);       // SF = 12
                    setBW(BW_250);      // BW = 250 KHz
                    break;
 
        // mode 3 (worst reach, less time on air)
        case 3:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_10);       // SF = 10
                    setBW(BW_125);      // BW = 125 KHz
                    break;
 
        // mode 4 (better reach, low time on air)
        case 4:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_12);       // SF = 12
                    setBW(BW_500);      // BW = 500 KHz
                    break;
 
        // mode 5 (better reach, medium time on air)
        case 5:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_10);       // SF = 10
                    setBW(BW_250);      // BW = 250 KHz
                    break;
 
        // mode 6 (better reach, worst time-on-air)
        case 6:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_11);       // SF = 11
                    setBW(BW_500);      // BW = 500 KHz
                    break;
 
        // mode 7 (medium-high reach, medium-low time-on-air)
        case 7:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_9);        // SF = 9
                    setBW(BW_250);      // BW = 250 KHz
                    break;
 
        // mode 8 (medium reach, medium time-on-air)
        case 8:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_9);        // SF = 9
                    setBW(BW_500);      // BW = 500 KHz
                    break;
 
        // mode 9 (medium-low reach, medium-high time-on-air)
        case 9:     setCR(CR_5);        // CR = 4/5
                    setSF(SF_8);        // SF = 8
                    setBW(BW_500);      // BW = 500 KHz
                    break;
 
        // mode 10 (worst reach, less time_on_air)
        case 10:    setCR(CR_5);        // CR = 4/5
                    setSF(SF_7);        // SF = 7
                    setBW(BW_500);      // BW = 500 KHz
                    break;
 
        default:    state = -1; // The indicated mode doesn't exist
 
    };

	if( state == -1 )	// if state = -1, don't change its value
	{
		#if (SX1272_debug_mode > 1)
			printf("** The indicated mode doesn't exist, ");
			printf("please select from 1 to 10 **\n");
		#endif
	}
	else
	{
		state = 1;
		config1 = readRegister(REG_MODEM_CONFIG1);
        switch (mode)
        {   //      Different way to check for each mode:
            // (config1 >> 3) ---> take out bits 7-3 from REG_MODEM_CONFIG1 (=_bandwidth & _codingRate together)
            // (config2 >> 4) ---> take out bits 7-4 from REG_MODEM_CONFIG2 (=_spreadingFactor)
 
            // mode 1: BW = 125 KHz, CR = 4/5, SF = 12.
            case 1:  if( (config1 >> 3) == 0x01 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_12 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
 
            // mode 2: BW = 250 KHz, CR = 4/5, SF = 12.
            case 2:  if( (config1 >> 3) == 0x09 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_12 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 3: BW = 125 KHz, CR = 4/5, SF = 10.
            case 3:  if( (config1 >> 3) == 0x01 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_10 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 4: BW = 500 KHz, CR = 4/5, SF = 12.
            case 4:  if( (config1 >> 3) == 0x11 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_12 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 5: BW = 250 KHz, CR = 4/5, SF = 10.
            case 5:  if( (config1 >> 3) == 0x09 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_10 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 6: BW = 500 KHz, CR = 4/5, SF = 11.
            case 6:  if( (config1 >> 3) == 0x11 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_11 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 7: BW = 250 KHz, CR = 4/5, SF = 9.
            case 7:  if( (config1 >> 3) == 0x09 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_9 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 8: BW = 500 KHz, CR = 4/5, SF = 9.
            case 8:  if ((config1 >> 3) == 0x11)
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_9 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 9: BW = 500 KHz, CR = 4/5, SF = 8.
            case 9:  if( (config1 >> 3) == 0x11 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_8 )
                            {
                            state = 0;
                            }
                        }
                     break;
 
            // mode 10: BW = 500 KHz, CR = 4/5, SF = 7.
            case 10: if( (config1 >> 3) == 0x11 )
                        {  config2 = readRegister(REG_MODEM_CONFIG2);
                        if( (config2 >> 4) == SF_7 )
                            {
                            state = 0;
                            }
                        }
        }// end switch

  }
  #if (SX1272_debug_mode > 1)
  if( state == 0 )
  {
		  printf("## Mode ");
		  printf("%d", mode);
		  printf(" configured with success ##\n");
		  printf("\n");
  }
  else
  {
		  printf("** There has been an error while configuring mode ");
		  printf("%d", mode);
		  printf(". **\n");
		  printf("\n");
	 
  }
  #endif

  writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  delayMicroseconds(100);
  return state;
}

/*
 Function: Indicates if module is configured in implicit or explicit header mode.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t	SX1272::getHeader()
{
	int8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getHeader'\n");
	#endif

	// take out bit 2 from REG_MODEM_CONFIG1 indicates ImplicitHeaderModeOn
	if( bitRead(REG_MODEM_CONFIG1, 2) == 0 )
	{ // explicit header mode (ON)
		_header = HEADER_ON;
		state = 1;
	}
	else
	{ // implicit header mode (OFF)
		_header = HEADER_OFF;
		state = 1;
	}

	state = 0;

	if( _modem == FSK )
	{ // header is not available in FSK mode
		#if (SX1272_debug_mode > 1)
			printf("## Notice that FSK mode packets hasn't header ##\n");
			printf("\n");
		#endif
	}
	else
	{ // header in LoRa mode
		#if (SX1272_debug_mode > 1)
			printf("## Header is ");
			if( _header == HEADER_ON )
			{
				printf("in explicit header mode ##\n");
			}
			else
			{
				printf("in implicit header mode ##\n");
			}
			printf("\n");
		#endif
	}
	return state;
}

/*
 Function: Sets the module in explicit header mode (header is sent).
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int8_t	SX1272::setHeaderON()
{
  int8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setHeaderON'\n");
  #endif

  if( _modem == FSK )
  {
	  state = -1;		// header is not available in FSK mode
	  #if (SX1272_debug_mode > 1)
		  printf("## FSK mode packets hasn't header ##\n");
		  printf("\n");
	  #endif
  }
  else
  {
	config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the header bit
	if( _spreadingFactor == 6 )
	{
		state = -1;		// Mandatory headerOFF with SF = 6
		#if (SX1272_debug_mode > 1)
			printf("## Mandatory implicit header mode with spreading factor = 6 ##\n");
		#endif
	}
	else
	{
		config1 = config1 & 0B11111011;			// clears bit 2 from config1 = headerON
		writeRegister(REG_MODEM_CONFIG1,config1);	// Update config1
	}
	if( _spreadingFactor != 6 )
	{ // checking headerON taking out bit 2 from REG_MODEM_CONFIG1
		config1 = readRegister(REG_MODEM_CONFIG1);
		if( bitRead(config1, 2) == HEADER_ON )
		{
			state = 0;
			_header = HEADER_ON;
			#if (SX1272_debug_mode > 1)
				printf("## Header has been activated ##\n");
				printf("\n");
			#endif
		}
		else
		{
			state = 1;
		}
	}
  }
  return state;
}

/*
 Function: Sets the module in implicit header mode (header is not sent).
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int8_t	SX1272::setHeaderOFF()
{
  uint8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setHeaderOFF'\n");
  #endif

  if( _modem == FSK )
  { // header is not available in FSK mode
	  state = -1;
	  #if (SX1272_debug_mode > 1)
		  printf("## Notice that FSK mode packets hasn't header ##\n");
		  printf("\n");
	  #endif
  }
  else
  {
	  config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the header bit
	  config1 = config1 | 0B00000100;				// sets bit 2 from REG_MODEM_CONFIG1 = headerOFF
	  writeRegister(REG_MODEM_CONFIG1,config1);		// Update config1

	  config1 = readRegister(REG_MODEM_CONFIG1);
	  if( bitRead(config1, 2) == HEADER_OFF )
	  { // checking headerOFF taking out bit 2 from REG_MODEM_CONFIG1
			state = 0;
			_header = HEADER_OFF;

			#if (SX1272_debug_mode > 1)
			    printf("## Header has been desactivated ##\n");
			    printf("\n");
			#endif
	  }
	  else
	  {
		  state = 1;
		  #if (SX1272_debug_mode > 1)
			  printf("** Header hasn't been desactivated ##\n");
			  printf("\n");
		  #endif
	  }
  }
  return state;
}

/*
 Function: Indicates if module is configured with or without checking CRC.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t	SX1272::getCRC()
{
	int8_t state = 2;
	byte value;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getCRC'\n");
	#endif

	if( _modem == LORA )
	{ // LoRa mode

		// take out bit 1 from REG_MODEM_CONFIG1 indicates RxPayloadCrcOn
		value = readRegister(REG_MODEM_CONFIG1);
		if( bitRead(value, 1) == CRC_OFF )
		{ // CRCoff
			_CRC = CRC_OFF;
			#if (SX1272_debug_mode > 1)
				printf("## CRC is desactivated ##\n");
				printf("\n");
			#endif
			state = 0;
		}
		else
		{ // CRCon
			_CRC = CRC_ON;
			#if (SX1272_debug_mode > 1)
				printf("## CRC is activated ##\n");
				printf("\n");
			#endif
			state = 0;
		}
	}
	else
	{ // FSK mode

		// take out bit 2 from REG_PACKET_CONFIG1 indicates CrcOn
		value = readRegister(REG_PACKET_CONFIG1);
		if( bitRead(value, 4) == CRC_OFF )
		{ // CRCoff
			_CRC = CRC_OFF;
			#if (SX1272_debug_mode > 1)
				printf("## CRC is desactivated ##\n");
				printf("\n");
			#endif
			state = 0;
		}
		else
		{ // CRCon
			_CRC = CRC_ON;
			#if (SX1272_debug_mode > 1)
				printf("## CRC is activated ##\n");
				printf("\n");
			#endif
			state = 0;
		}
	}
	if( state != 0 )
	{
		state = 1;
		#if (SX1272_debug_mode > 1)
			printf("** There has been an error while getting configured CRC **\n");
			printf("\n");
		#endif
	}
	return state;
}

/*
 Function: Sets the module with CRC on.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t	SX1272::setCRC_ON()
{
  uint8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setCRC_ON'\n");
  #endif

  if( _modem == LORA )
  { // LORA mode
	config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the CRC bit
	config1 = config1 | 0B00000010;				// sets bit 1 from REG_MODEM_CONFIG1 = CRC_ON
	writeRegister(REG_MODEM_CONFIG1,config1);

	state = 1;

	config1 = readRegister(REG_MODEM_CONFIG1);
	if( bitRead(config1, 1) == CRC_ON )
	{ // take out bit 1 from REG_MODEM_CONFIG1 indicates RxPayloadCrcOn
		state = 0;
		_CRC = CRC_ON;
		#if (SX1272_debug_mode > 1)
			printf("## CRC has been activated ##\n");
			printf("\n");
		#endif
	}
  }
  else
  { // FSK mode
	config1 = readRegister(REG_PACKET_CONFIG1);	// Save config1 to modify only the CRC bit
	config1 = config1 | 0B00010000;				// set bit 4 and 3 from REG_MODEM_CONFIG1 = CRC_ON
	writeRegister(REG_PACKET_CONFIG1,config1);

	state = 1;

	config1 = readRegister(REG_PACKET_CONFIG1);
	if( bitRead(config1, 4) == CRC_ON )
	{ // take out bit 4 from REG_PACKET_CONFIG1 indicates CrcOn
		state = 0;
		_CRC = CRC_ON;
		#if (SX1272_debug_mode > 1)
			printf("## CRC has been activated ##\n");
			printf("\n");
		#endif
	}
  }
  if( state != 0 )
  {
	  state = 1;
	  #if (SX1272_debug_mode > 1)
		  printf("** There has been an error while setting CRC ON **\n");
		  printf("\n");
	  #endif
  }
  return state;
}

/*
 Function: Sets the module with CRC off.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t	SX1272::setCRC_OFF()
{
  int8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setCRC_OFF'\n");
  #endif

  if( _modem == LORA )
  { // LORA mode
  	config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the CRC bit
	config1 = config1 & 0B11111101;				// clears bit 1 from config1 = CRC_OFF
	writeRegister(REG_MODEM_CONFIG1,config1);

	config1 = readRegister(REG_MODEM_CONFIG1);
	if( (bitRead(config1, 1)) == CRC_OFF )
	{ // take out bit 1 from REG_MODEM_CONFIG1 indicates RxPayloadCrcOn
	  state = 0;
	  _CRC = CRC_OFF;
	  #if (SX1272_debug_mode > 1)
		  printf("## CRC has been desactivated ##\n");
		  printf("\n");
	  #endif
	}
  }
  else
  { // FSK mode
	config1 = readRegister(REG_PACKET_CONFIG1);	// Save config1 to modify only the CRC bit
	config1 = config1 & 0B11101111;				// clears bit 4 from config1 = CRC_OFF
	writeRegister(REG_PACKET_CONFIG1,config1);

	config1 = readRegister(REG_PACKET_CONFIG1);
	if( bitRead(config1, 4) == CRC_OFF )
	{ // take out bit 4 from REG_PACKET_CONFIG1 indicates RxPayloadCrcOn
		state = 0;
		_CRC = CRC_OFF;
		#if (SX1272_debug_mode > 1)
		    printf("## CRC has been desactivated ##\n");
		    printf("\n");
	    #endif
	}
  }
  if( state != 0 )
  {
	  state = 1;
	  #if (SX1272_debug_mode > 1)
		  printf("** There has been an error while setting CRC OFF **\n");
		  printf("\n");
	  #endif
  }
  return state;
}

/*
 Function: Checks if SF is a valid value.
 Returns: Boolean that's 'true' if the SF value exists and
		  it's 'false' if the SF value does not exist.
 Parameters:
   spr: spreading factor value to check.
*/
boolean	SX1272::isSF(uint8_t spr)
{
  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'isSF'\n");
  #endif

  // Checking available values for _spreadingFactor
  switch(spr)
  {
	  case SF_6:
	  case SF_7:
	  case SF_8:
	  case SF_9:
	  case SF_10:
	  case SF_11:
	  case SF_12:	return true;
					break;

	  default:		return false;
  }
  #if (SX1272_debug_mode > 1)
	  printf("## Finished 'isSF' ##\n");
	  printf("\n");
  #endif
}

/*
 Function: Gets the SF within the module is configured.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int8_t	SX1272::getSF()
{
  int8_t state = 2;
  byte config2;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getSF'\n");
  #endif

  if( _modem == FSK )
  {
	  state = -1;		// SF is not available in FSK mode
	  #if (SX1272_debug_mode > 1)
		  printf("** FSK mode hasn't spreading factor **\n");
		  printf("\n");
	  #endif
  }
  else
  {
	// take out bits 7-4 from REG_MODEM_CONFIG2 indicates _spreadingFactor
	config2 = (readRegister(REG_MODEM_CONFIG2)) >> 4;
	_spreadingFactor = config2;
	state = 1;

	if( (config2 == _spreadingFactor) && isSF(_spreadingFactor) )
	{
		state = 0;
		#if (SX1272_debug_mode > 1)
			printf("## Spreading factor is ");
			printf("%X", _spreadingFactor);
			printf(" ##\n");
			printf("\n");
		#endif
	}
  }
  return state;
}

/*
 Function: Sets the indicated SF in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
 Parameters:
   spr: spreading factor value to set in LoRa modem configuration.
*/
uint8_t	SX1272::setSF(uint8_t spr)
{
  byte st0;
  int8_t state = 2;
  byte config1;
  byte config2;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setSF'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);	// Save the previous status

  if( _modem == FSK )
  {
	  #if (SX1272_debug_mode > 1)
		  printf("## Notice that FSK hasn't Spreading Factor parameter, ");
		  printf("so you are configuring it in LoRa mode ##\n");
	  #endif
	  state = setLORA();				// Setting LoRa mode
  }
  else
  { // LoRa mode
	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode
	config1 = (readRegister(REG_MODEM_CONFIG1));	// Save config1 to modify only the LowDataRateOptimize
	config2 = (readRegister(REG_MODEM_CONFIG2));	// Save config2 to modify SF value (bits 7-4)
	switch(spr)
	{
		case SF_6: 	config2 = config2 & 0B01101111;	// clears bits 7 & 4 from REG_MODEM_CONFIG2
					config2 = config2 | 0B01100000;	// sets bits 6 & 5 from REG_MODEM_CONFIG2
					setHeaderOFF();		// Mandatory headerOFF with SF = 6
					break;
		case SF_7: 	config2 = config2 & 0B01111111;	// clears bits 7 from REG_MODEM_CONFIG2
					config2 = config2 | 0B01110000;	// sets bits 6, 5 & 4
					break;
		case SF_8: 	config2 = config2 & 0B10001111;	// clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
					config2 = config2 | 0B10000000;	// sets bit 7 from REG_MODEM_CONFIG2
					break;
		case SF_9: 	config2 = config2 & 0B10011111;	// clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
					config2 = config2 | 0B10010000;	// sets bits 7 & 4 from REG_MODEM_CONFIG2
					break;
		case SF_10:	config2 = config2 & 0B10101111;	// clears bits 6 & 4 from REG_MODEM_CONFIG2
					config2 = config2 | 0B10100000;	// sets bits 7 & 5 from REG_MODEM_CONFIG2
					break;
		case SF_11:	config2 = config2 & 0B10111111;	// clears bit 6 from REG_MODEM_CONFIG2
					config2 = config2 | 0B10110000;	// sets bits 7, 5 & 4 from REG_MODEM_CONFIG2
					getBW();
					if( _bandwidth == BW_125 )
					{ // LowDataRateOptimize (Mandatory with SF_11 if BW_125)
						config1 = config1 | 0B00000001;
					}
					break;
		case SF_12: config2 = config2 & 0B11001111;	// clears bits 5 & 4 from REG_MODEM_CONFIG2
					config2 = config2 | 0B11000000;	// sets bits 7 & 6 from REG_MODEM_CONFIG2
					if( _bandwidth == BW_125 )
					{ // LowDataRateOptimize (Mandatory with SF_12 if BW_125)
						config1 = config1 | 0B00000001;
					}
					break;
	}

	// Check if it is neccesary to set special settings for SF=6
    if( spr == SF_6 )
    {       
        // Mandatory headerOFF with SF = 6 (Implicit mode)
        setHeaderOFF(); 
        
        // Set the bit field DetectionOptimize of 
        // register RegLoRaDetectOptimize to value "0b101".
        writeRegister(REG_DETECT_OPTIMIZE, 0x05);
        
        // Write 0x0C in the register RegDetectionThreshold.                        
        writeRegister(REG_DETECTION_THRESHOLD, 0x0C);
    }
    else
    {
        // LoRa detection Optimize: 0x03 --> SF7 to SF12
        writeRegister(REG_DETECT_OPTIMIZE, 0x03);
        
        // LoRa detection threshold: 0x0A --> SF7 to SF12                    
        writeRegister(REG_DETECTION_THRESHOLD, 0x0A);       
    }
 
    // sets bit 2-0 (AgcAutoOn and SymbTimout) for any SF value
    config2 = config2 | 0B00000111; 



	writeRegister(REG_MODEM_CONFIG1, config1);		// Update config1
	writeRegister(REG_MODEM_CONFIG2, config2);		// Update config2

	delayMicroseconds(100);

	config1 = (readRegister(REG_MODEM_CONFIG1));	// Save config1 to check update
	config2 = (readRegister(REG_MODEM_CONFIG2));	// Save config2 to check update
	// (config2 >> 4) ---> take out bits 7-4 from REG_MODEM_CONFIG2 (=_spreadingFactor)
	// bitRead(config1, 0) ---> take out bits 1 from config1 (=LowDataRateOptimize)
	switch(spr)
	{
		case SF_6:	if(		((config2 >> 4) == spr)
						&& 	(bitRead(config2, 2) == 1)
						&& 	(_header == HEADER_OFF))
					{
						state = 0;
					}
					break;
		case SF_7:	if(		((config2 >> 4) == 0x07)
						 && (bitRead(config2, 2) == 1))
					{
						state = 0;
					}
					break;
		case SF_8:	if(		((config2 >> 4) == 0x08)
						 && (bitRead(config2, 2) == 1))
					{
						state = 0;
					}
					break;
		case SF_9:	if(		((config2 >> 4) == 0x09)
						 && (bitRead(config2, 2) == 1))
					{
						state = 0;
					}
					break;
		case SF_10:	if(		((config2 >> 4) == 0x0A)
						 && (bitRead(config2, 2) == 1))
					{
						state = 0;
					}
					break;
		case SF_11:	if(		((config2 >> 4) == 0x0B)
						 && (bitRead(config2, 2) == 1)
						 && (bitRead(config1, 0) == 1))
					{
						state = 0;
					}
					break;
		case SF_12:	if(		((config2 >> 4) == 0x0C)
						 && (bitRead(config2, 2) == 1)
						 && (bitRead(config1, 0) == 1))
					{
						state = 0;
					}
					break;
		default:	state = 1;
	}
  }

  writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  delayMicroseconds(100);

  if( isSF(spr) )
  { // Checking available value for _spreadingFactor
		state = 0;
		_spreadingFactor = spr;
		#if (SX1272_debug_mode > 1)
		    printf("## Spreading factor ");
		    printf("%d", _spreadingFactor);
		    printf(" has been successfully set ##\n");
		    printf("\n");
		#endif
  }
  else
  {
	  if( state != 0 )
	  {
		  #if (SX1272_debug_mode > 1)
		      printf("** There has been an error while setting the spreading factor **");
		      printf("\n");
		  #endif
	  }
  }
  return state;
}

/*
 Function: Checks if BW is a valid value.
 Returns: Boolean that's 'true' if the BW value exists and
		  it's 'false' if the BW value does not exist.
 Parameters:
   band: bandwidth value to check.
*/
boolean	SX1272::isBW(uint16_t band)
{
  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'isBW'\n");
  #endif

  // Checking available values for _bandwidth
  switch(band)
  {
	  case BW_125:
	  case BW_250:
	  case BW_500:	return true;
					break;

	  default:		return false;
  }
  #if (SX1272_debug_mode > 1)
	  printf("## Finished 'isBW' ##\n");
	  printf("\n");
  #endif
}

/*
 Function: Gets the BW within the module is configured.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int8_t	SX1272::getBW()
{
  uint8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getBW'\n");
  #endif

  if( _modem == FSK )
  {
	  state = -1;		// BW is not available in FSK mode
	  #if (SX1272_debug_mode > 1)
		  printf("** FSK mode hasn't bandwidth **\n");
		  printf("\n");
	  #endif
  }
  else
  {
	  // take out bits 7-6 from REG_MODEM_CONFIG1 indicates _bandwidth
	  config1 = (readRegister(REG_MODEM_CONFIG1)) >> 6;
	  _bandwidth = config1;

	  if( (config1 == _bandwidth) && isBW(_bandwidth) )
	  {
		  state = 0;
		  #if (SX1272_debug_mode > 1)
			  printf("## Bandwidth is ");
			  printf("%X", _bandwidth);
			  printf(" ##\n");
			  printf("\n");
		  #endif
	  }
	  else
	  {
		  state = 1;
		  #if (SX1272_debug_mode > 1)
			  printf("** There has been an error while getting bandwidth **");
			  printf("\n");
		  #endif
	  }
  }
  return state;
}

/*
 Function: Sets the indicated BW in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
 Parameters:
   band: bandwith value to set in LoRa modem configuration.
*/
int8_t	SX1272::setBW(uint16_t band)
{
  byte st0;
  int8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setBW'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);	// Save the previous status

  if( _modem == FSK )
  {
	  #if (SX1272_debug_mode > 1)
		  printf("## Notice that FSK hasn't Bandwidth parameter, ");
		  printf("so you are configuring it in LoRa mode ##\n");
	  #endif
	  state = setLORA();
  }
  writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode
  config1 = (readRegister(REG_MODEM_CONFIG1));	// Save config1 to modify only the BW
  switch(band)
  {
	  case BW_125:  config1 = config1 & 0B00111111;	// clears bits 7 & 6 from REG_MODEM_CONFIG1
					getSF();
					if( _spreadingFactor == 11 )
					{ // LowDataRateOptimize (Mandatory with BW_125 if SF_11)
						config1 = config1 | 0B00000001;
					}
					if( _spreadingFactor == 12 )
					{ // LowDataRateOptimize (Mandatory with BW_125 if SF_12)
						config1 = config1 | 0B00000001;
					}
					break;
	  case BW_250:  config1 = config1 & 0B01111111;	// clears bit 7 from REG_MODEM_CONFIG1
					config1 = config1 | 0B01000000;	// sets bit 6 from REG_MODEM_CONFIG1
					break;
	  case BW_500:  config1 = config1 & 0B10111111;	//clears bit 6 from REG_MODEM_CONFIG1
					config1 = config1 | 0B10000000;	//sets bit 7 from REG_MODEM_CONFIG1
					break;
  }
  writeRegister(REG_MODEM_CONFIG1,config1);		// Update config1

  delayMicroseconds(100);

  config1 = (readRegister(REG_MODEM_CONFIG1));
  // (config1 >> 6) ---> take out bits 7-6 from REG_MODEM_CONFIG1 (=_bandwidth)
  switch(band)
  {
	   case BW_125: if( (config1 >> 6) == BW_125 )
					{
						state = 0;
						if( _spreadingFactor == 11 )
						{
							if( bitRead(config1, 0) == 1 )
							{ // LowDataRateOptimize
								state = 0;
							}
							else
							{
								state = 1;
							}
						}
						if( _spreadingFactor == 12 )
						{
							if( bitRead(config1, 0) == 1 )
							{ // LowDataRateOptimize
								state = 0;
							}
							else
							{
								state = 1;
							}
						}
					}
					break;
	   case BW_250: if( (config1 >> 6) == BW_250 )
					{
						state = 0;
					}
					break;
	   case BW_500: if( (config1 >> 6) == BW_500 )
					{
						state = 0;
					}
					break;
  }

  if( not isBW(band) )
  {
	  state = 1;
	  #if (SX1272_debug_mode > 1)
		  printf("** Bandwidth ");
		  printf("%X", band);
		  printf(" is not a correct value **\n");
		  printf("\n");
	  #endif
  }
  else
  {
	  _bandwidth = band;
	  #if (SX1272_debug_mode > 1)
		  printf("## Bandwidth ");
		  printf("%X", band);
		  printf(" has been successfully set ##\n");
		  printf("\n");
	  #endif
  }
  writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  delayMicroseconds(100);
  return state;
}

/*
 Function: Checks if CR is a valid value.
 Returns: Boolean that's 'true' if the CR value exists and
		  it's 'false' if the CR value does not exist.
 Parameters:
   cod: coding rate value to check.
*/
boolean	SX1272::isCR(uint8_t cod)
{
  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'isCR'\n");
  #endif

  // Checking available values for _codingRate
  switch(cod)
  {
	  case CR_5:
	  case CR_6:
	  case CR_7:
	  case CR_8:	return true;
					break;

	  default:		return false;
  }
  #if (SX1272_debug_mode > 1)
	  printf("## Finished 'isCR' ##\n");
	  printf("\n");
  #endif
}

/*
 Function: Indicates the CR within the module is configured.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int8_t	SX1272::getCR()
{
  int8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getCR'\n");
  #endif

  if( _modem == FSK )
  {
	  state = -1;		// CR is not available in FSK mode
	  #if (SX1272_debug_mode > 1)
		  printf("** FSK mode hasn't coding rate **\n");
		  printf("\n");
	  #endif
  }
  else
  {
	// take out bits 7-3 from REG_MODEM_CONFIG1 indicates _bandwidth & _codingRate
	config1 = (readRegister(REG_MODEM_CONFIG1)) >> 3;
	config1 = config1 & 0B00000111;	// clears bits 7-5 ---> clears _bandwidth
	_codingRate = config1;
	state = 1;

	if( (config1 == _codingRate) && isCR(_codingRate) )
	{
		state = 0;
		#if (SX1272_debug_mode > 1)
			printf("## Coding rate is ");
			printf("%X", _codingRate);
			printf(" ##\n");
			printf("\n");
		#endif
	}
  }
  return state;
}

/*
 Function: Sets the indicated CR in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
 Parameters:
   cod: coding rate value to set in LoRa modem configuration.
*/
int8_t	SX1272::setCR(uint8_t cod)
{
  byte st0;
  int8_t state = 2;
  byte config1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setCR'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);		// Save the previous status

  if( _modem == FSK )
  {
	  #if (SX1272_debug_mode > 1)
		  printf("## Notice that FSK hasn't Coding Rate parameter, ");
		  printf("so you are configuring it in LoRa mode ##\n");
	  #endif
	  state = setLORA();
  }
  writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);		// Set Standby mode to write in registers

  config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the CR
  switch(cod)
  {
	 case CR_5: config1 = config1 & 0B11001111;	// clears bits 5 & 4 from REG_MODEM_CONFIG1
				config1 = config1 | 0B00001000;	// sets bit 3 from REG_MODEM_CONFIG1
				break;
	 case CR_6: config1 = config1 & 0B11010111;	// clears bits 5 & 3 from REG_MODEM_CONFIG1
				config1 = config1 | 0B00010000;	// sets bit 4 from REG_MODEM_CONFIG1
				break;
	 case CR_7: config1 = config1 & 0B11011111;	// clears bit 5 from REG_MODEM_CONFIG1
				config1 = config1 | 0B00011000;	// sets bits 4 & 3 from REG_MODEM_CONFIG1
			break;
	 case CR_8: config1 = config1 & 0B11100111;	// clears bits 4 & 3 from REG_MODEM_CONFIG1
				config1 = config1 | 0B00100000;	// sets bit 5 from REG_MODEM_CONFIG1
				break;
  }
  writeRegister(REG_MODEM_CONFIG1, config1);		// Update config1

  delayMicroseconds(100);

  config1 = readRegister(REG_MODEM_CONFIG1);
  // ((config1 >> 3) & B0000111) ---> take out bits 5-3 from REG_MODEM_CONFIG1 (=_codingRate)
  switch(cod)
  {
	 case CR_5: if( ((config1 >> 3) & 0B0000111) == 0x01 )
				{
					state = 0;
				}
			break;
	 case CR_6: if( ((config1 >> 3) & 0B0000111) == 0x02 )
				{
					state = 0;
				}
				break;
	 case CR_7: if( ((config1 >> 3) & 0B0000111) == 0x03 )
				{
					state = 0;
				}
				break;
	 case CR_8: if( ((config1 >> 3) & 0B0000111) == 0x04 )
				{
					state = 0;
				}
				break;
  }

  if( isCR(cod) )
  {
	  _codingRate = cod;
	  #if (SX1272_debug_mode > 1)
		  printf("## Coding Rate ");
		  printf("%X", cod);
		  printf(" has been successfully set ##\n");
		  printf("\n");
	  #endif
  }
  else
  {
	  state = 1;
	  #if (SX1272_debug_mode > 1)
		  printf("** There has been an error while configuring Coding Rate parameter **\n");
		  printf("\n");
	  #endif
  }
  writeRegister(REG_OP_MODE,st0);	// Getting back to previous status
  delayMicroseconds(100);
  return state;
}

/*
 Function: Checks if channel is a valid value.
 Returns: Boolean that's 'true' if the CR value exists and
		  it's 'false' if the CR value does not exist.
 Parameters:
   ch: frequency channel value to check.
*/
boolean	SX1272::isChannel(uint32_t ch)
{
  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'isChannel'\n");
  #endif

  // Checking available values for _channel
  switch(ch)
  {
	  case CH_10_868:
	  case CH_11_868:
	  case CH_12_868:
	  case CH_13_868:
	  case CH_14_868:
	  case CH_15_868:
	  case CH_16_868:
	  case CH_17_868:
	  case CH_00_900:
	  case CH_01_900:
	  case CH_02_900:
	  case CH_03_900:
	  case CH_04_900:
	  case CH_05_900:
	  case CH_06_900:
	  case CH_07_900:
	  case CH_08_900:
	  case CH_09_900:
	  case CH_10_900:
	  case CH_11_900:	return true;
						break;

	  default:			return false;
  }
  #if (SX1272_debug_mode > 1)
	  printf("## Finished 'isChannel' ##\n");
	  printf("\n");
  #endif
}

/*
 Function: Indicates the frequency channel within the module is configured.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getChannel()
{
  uint8_t state = 2;
  uint32_t ch;
  uint8_t freq3;
  uint8_t freq2;
  uint8_t freq1;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getChannel'\n");
  #endif

  freq3 = readRegister(REG_FRF_MSB);	// frequency channel MSB
  freq2 = readRegister(REG_FRF_MID);	// frequency channel MID
  freq1 = readRegister(REG_FRF_LSB);	// frequency channel LSB
  ch = ((uint32_t)freq3 << 16) + ((uint32_t)freq2 << 8) + (uint32_t)freq1;
  _channel = ch;						// frequency channel

  if( (_channel == ch) && isChannel(_channel) )
  {
	  state = 0;
	  #if (SX1272_debug_mode > 1)
		  printf("## Frequency channel is ");
		  printf("%X", _channel);
		  printf(" ##\n");
		  printf("\n");
	  #endif
  }
  else
  {
	  state = 1;
  }
  return state;
}

/*
 Function: Sets the indicated channel in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
 Parameters:
   ch: frequency channel value to set in configuration.
*/
int8_t SX1272::setChannel(uint32_t ch)
{
  byte st0;
  int8_t state = 2;
  unsigned int freq3;
  unsigned int freq2;
  uint8_t freq1;
  uint32_t freq;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setChannel'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);	// Save the previous status
  if( _modem == LORA )
  {
	  // LoRa Stdby mode in order to write in registers
	  writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
  }
  else
  {
	  // FSK Stdby mode in order to write in registers
	  writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);
  }

  freq3 = ((ch >> 16) & 0x0FF);		// frequency channel MSB
  freq2 = ((ch >> 8) & 0x0FF);		// frequency channel MIB
  freq1 = (ch & 0xFF);				// frequency channel LSB

  writeRegister(REG_FRF_MSB, freq3);
  writeRegister(REG_FRF_MID, freq2);
  writeRegister(REG_FRF_LSB, freq1);

  delayMicroseconds(100);

  // storing MSB in freq channel value
  freq3 = (readRegister(REG_FRF_MSB));
  freq = (freq3 << 8) & 0xFFFFFF;

  // storing MID in freq channel value
  freq2 = (readRegister(REG_FRF_MID));
  freq = (freq << 8) + ((freq2 << 8) & 0xFFFFFF);

  // storing LSB in freq channel value
  freq = freq + ((readRegister(REG_FRF_LSB)) & 0xFFFFFF);

  if( freq == ch )
  {
    state = 0;
    _channel = ch;
    #if (SX1272_debug_mode > 1)
		printf("## Frequency channel ");
		printf("%X", ch);
		printf(" has been successfully set ##\n");
		printf("\n");
	#endif
  }
  else
  {
    state = 1;
  }

  if( not isChannel(ch) )
  {
	 state = -1;
	 #if (SX1272_debug_mode > 1)
		 printf("** Frequency channel ");
		 printf("%X", ch);
		 printf("is not a correct value **\n");
		 printf("\n");
	 #endif
  }

  writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  delayMicroseconds(100);
  return state;
}

/*
 Function: Gets the signal power within the module is configured.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getPower()
{
  uint8_t state = 2;
  byte value = 0x00;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getPower'\n");
  #endif

  value = readRegister(REG_PA_CONFIG);
  state = 1;

  _power = value;
  if( (value > -1) & (value < 16) )
  {
	    state = 0;
		#if (SX1272_debug_mode > 1)
			printf("## Output power is ");
			printf("%X", _power);
			printf(" ##\n");
			printf("\n");
		#endif
	}

  return state;
}

/*
 Function: Sets the signal power indicated in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
 Parameters:
   p: power option to set in configuration.
*/
int8_t SX1272::setPower(char p)
{
  byte st0;
  int8_t state = 2;
  byte value = 0x00;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setPower'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);	  // Save the previous status
  if( _modem == LORA )
  { // LoRa Stdby mode to write in registers
	  writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
  }
  else
  { // FSK Stdby mode to write in registers
	  writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);
  }

  switch (p)
  {
    // L = low
    // H = high
    // M = max

    case 'M':  _power = 0x0F;
               break;

    case 'L':  _power = 0x00;
               break;

    case 'H':  _power = 0x07;
               break;

    default:   state = -1;
               break;
  }

  writeRegister(REG_PA_CONFIG, _power);	// Setting output power value
  value = readRegister(REG_PA_CONFIG);

  if( value == _power )
  {
	  state = 0;
	  #if (SX1272_debug_mode > 1)
		  printf("## Output power has been successfully set ##\n");
		  printf("\n");
	  #endif
  }
  else
  {
	  state = 1;
  }

  writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  delayMicroseconds(100);
  return state;
}

/*
 Function: Sets the signal power indicated in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
 Parameters:
   p: power option to set in configuration.
*/
int8_t SX1272::setPowerNum(uint8_t pow)
{
  byte st0;
  int8_t state = 2;
  byte value = 0x00;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'setPower'\n");
  #endif

  st0 = readRegister(REG_OP_MODE);	  // Save the previous status
  if( _modem == LORA )
  { // LoRa Stdby mode to write in registers
	  writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
  }
  else
  { // FSK Stdby mode to write in registers
	  writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);
  }

  if ( (pow >= 0) & (pow < 15) )
  {
	  _power = pow;
  }
  else
  {
	  state = -1;
	  #if (SX1272_debug_mode > 1)
		  printf("## Power value is not valid ##\n");
		  printf("\n");
	  #endif
  }

  writeRegister(REG_PA_CONFIG, _power);	// Setting output power value
  value = readRegister(REG_PA_CONFIG);

  if( value == _power )
  {
	  state = 0;
	  #if (SX1272_debug_mode > 1)
		  printf("## Output power has been successfully set ##\n");
		  printf("\n");
	  #endif
  }
  else
  {
	  state = 1;
  }

  writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  delayMicroseconds(100);
  return state;
}


/*
 Function: Gets the preamble length from the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getPreambleLength()
{
	int8_t state = 2;
	uint8_t p_length;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getPreambleLength'\n");
	#endif

	state = 1;
	if( _modem == LORA )
  	{ // LORA mode
  		p_length = readRegister(REG_PREAMBLE_MSB_LORA);
  		// Saving MSB preamble length in LoRa mode
		_preamblelength = (p_length << 8) & 0xFFFF;
		p_length = readRegister(REG_PREAMBLE_LSB_LORA);
  		// Saving LSB preamble length in LoRa mode
		_preamblelength = _preamblelength + (p_length & 0xFFFF);
		#if (SX1272_debug_mode > 1)
			printf("## Preamble length configured is ");
			printf("%X", _preamblelength);
			printf(" ##");
			printf("\n");
		#endif
	}
	else
	{ // FSK mode
		p_length = readRegister(REG_PREAMBLE_MSB_FSK);
		// Saving MSB preamble length in FSK mode
		_preamblelength = (p_length << 8) & 0xFFFF;
		p_length = readRegister(REG_PREAMBLE_LSB_FSK);
		// Saving LSB preamble length in FSK mode
		_preamblelength = _preamblelength + (p_length & 0xFFFF);
		#if (SX1272_debug_mode > 1)
			printf("## Preamble length configured is ");
			printf("%X", _preamblelength);
			printf(" ##");
			printf("\n");
		#endif
	}
	state = 0;
	return state;
}

/*
 Function: Sets the preamble length in the module
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
 Parameters:
   l: length value to set as preamble length.
*/
uint8_t SX1272::setPreambleLength(uint16_t l)
{
	byte st0;
	uint8_t p_length;
	int8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setPreambleLength'\n");
	#endif

	st0 = readRegister(REG_OP_MODE);	// Save the previous status
	state = 1;
	if( _modem == LORA )
  	{ // LoRa mode
  		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // Set Standby mode to write in registers
  		p_length = ((l >> 8) & 0x0FF);
  		// Storing MSB preamble length in LoRa mode
		writeRegister(REG_PREAMBLE_MSB_LORA, p_length);
		p_length = (l & 0x0FF);
		// Storing LSB preamble length in LoRa mode
		writeRegister(REG_PREAMBLE_LSB_LORA, p_length);
	}
	else
	{ // FSK mode
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);    // Set Standby mode to write in registers
		p_length = ((l >> 8) & 0x0FF);
  		// Storing MSB preamble length in FSK mode
		writeRegister(REG_PREAMBLE_MSB_FSK, p_length);
		p_length = (l & 0x0FF);
  		// Storing LSB preamble length in FSK mode
		writeRegister(REG_PREAMBLE_LSB_FSK, p_length);
	}

	state = 0;
	#if (SX1272_debug_mode > 1)
		printf("## Preamble length ");
		printf("%X", l);
		printf(" has been successfully set ##\n");
		printf("\n");
	#endif

	writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  	delayMicroseconds(100);
	return state;
}

/*
 Function: Gets the payload length from the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getPayloadLength()
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getPayloadLength'\n");
	#endif

	if( _modem == LORA )
  	{ // LORA mode
  		// Saving payload length in LoRa mode
		_payloadlength = readRegister(REG_PAYLOAD_LENGTH_LORA);
		state = 1;
	}
	else
	{ // FSK mode
  		// Saving payload length in FSK mode
		_payloadlength = readRegister(REG_PAYLOAD_LENGTH_FSK);
		state = 1;
	}

	#if (SX1272_debug_mode > 1)
		printf("## Payload length configured is ");
		printf("%X", _payloadlength);
		printf(" ##\n");
		printf("\n");
	#endif

	state = 0;
	return state;
}

/*
 Function: Sets the packet length in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int8_t SX1272::setPacketLength()
{
	uint16_t length;

	length = _payloadlength + OFFSET_PAYLOADLENGTH;
	return setPacketLength(length);
}

/*
 Function: Sets the packet length in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
 Parameters:
   l: length value to set as payload length.
*/
int8_t SX1272::setPacketLength(uint8_t l)
{
	byte st0;
	byte value = 0x00;
	int8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setPacketLength'\n");
	#endif

	st0 = readRegister(REG_OP_MODE);	// Save the previous status
	packet_sent.length = l;

	if( _modem == LORA )
  	{ // LORA mode
  		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // Set LoRa Standby mode to write in registers
		writeRegister(REG_PAYLOAD_LENGTH_LORA, packet_sent.length);
		// Storing payload length in LoRa mode
		value = readRegister(REG_PAYLOAD_LENGTH_LORA);
	}
	else
	{ // FSK mode
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);    //  Set FSK Standby mode to write in registers
		writeRegister(REG_PAYLOAD_LENGTH_FSK, packet_sent.length);
		// Storing payload length in FSK mode
		value = readRegister(REG_PAYLOAD_LENGTH_FSK);
	}

	if( packet_sent.length == value )
	{
		state = 0;
		#if (SX1272_debug_mode > 1)
			printf("## Packet length ");
			printf("%d", packet_sent.length);
			printf(" has been successfully set ##\n");
			printf("\n");
		#endif
	}
	else
	{
		state = 1;
	}

	writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
  	delayMicroseconds(250);
	return state;
}

/*
 Function: Gets the node address in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getNodeAddress()
{
	byte st0 = 0;
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getNodeAddress'\n");
	#endif

	if( _modem == LORA )
	{ // LoRa mode
		st0 = readRegister(REG_OP_MODE);	// Save the previous status
		// Allowing access to FSK registers while in LoRa standby mode
		writeRegister(REG_OP_MODE, LORA_STANDBY_FSK_REGS_MODE);
	}

	// Saving node address
	_nodeAddress = readRegister(REG_NODE_ADRS);
	state = 1;

	if( _modem == LORA )
	{
		writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
	}

	state = 0;
	#if (SX1272_debug_mode > 1)
		printf("## Node address configured is ");
		printf("%d", _nodeAddress);
		printf(" ##\n");
		printf("\n");
	#endif
	return state;
}

/*
 Function: Sets the node address in the module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
 Parameters:
   addr: address value to set as node address.
*/
int8_t SX1272::setNodeAddress(uint8_t addr)
{
	byte st0;
	byte value;
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setNodeAddress'\n");
	#endif

	if( addr > 255 )
	{
		state = -1;
		#if (SX1272_debug_mode > 1)
			printf("** Node address must be less than 255 **\n");
			printf("\n");
		#endif
	}
	else
	{
		// Saving node address
		_nodeAddress = addr;
		st0 = readRegister(REG_OP_MODE);	  // Save the previous status

		if( _modem == LORA )
		{ // Allowing access to FSK registers while in LoRa standby mode
			writeRegister(REG_OP_MODE, LORA_STANDBY_FSK_REGS_MODE);
		}
		else
		{ //Set FSK Standby mode to write in registers
			writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);
		}

		// Storing node and broadcast address
		writeRegister(REG_NODE_ADRS, addr);
		writeRegister(REG_BROADCAST_ADRS, BROADCAST_0);

		value = readRegister(REG_NODE_ADRS);
		writeRegister(REG_OP_MODE, st0);		// Getting back to previous status

		if( value == _nodeAddress )
		{
			state = 0;
			#if (SX1272_debug_mode > 1)
				printf("## Node address ");
				printf("%d", addr);
				printf(" has been successfully set ##\n");
				printf("\n");
			#endif
		}
		else
		{
			state = 1;
			#if (SX1272_debug_mode > 1)
				printf("** There has been an error while setting address ##\n");
				printf("\n");
			#endif
		}
	}
	return state;
}

/*
 Function: Gets the SNR value in LoRa mode.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int8_t SX1272::getSNR()
{	// getSNR exists only in LoRa mode
  int8_t state = 2;
  byte value;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getSNR'\n");
  #endif

  if( _modem == LORA )
  { // LoRa mode
	  state = 1;
	  value = readRegister(REG_PKT_SNR_VALUE);
	  if( value & 0x80 ) // The SNR sign bit is 1
	  {
		  // Invert and divide by 4
		  value = ( ( ~value + 1 ) & 0xFF ) >> 2;
          _SNR = -value;
      }
      else
      {
		  // Divide by 4
		  _SNR = ( value & 0xFF ) >> 2;
	  }
	  state = 0;
	  #if (SX1272_debug_mode > 0)
		  printf("## SNR value is ");
		  printf("%d", _SNR);
		  printf(" ##\n");
		  printf("\n");
	  #endif
  }
  else
  { // forbidden command if FSK mode
	state = -1;
	#if (SX1272_debug_mode > 0)
		printf("** SNR does not exist in FSK mode **\n");
		printf("\n");
	#endif
  }
  return state;
}

/*
 Function: Gets the current value of RSSI.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getRSSI()
{
  uint8_t state = 2;
  int rssi_mean = 0;
  int total = 5;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getRSSI'\n");
	#endif

  if( _modem == LORA )
    { 
        /// LoRa mode
        // get mean value of RSSI
        for(int i = 0; i < total; i++)
        {
            _RSSI = -OFFSET_RSSI + readRegister(REG_RSSI_VALUE_LORA);
            rssi_mean += _RSSI;         
        }
 
        rssi_mean = rssi_mean / total;  
        _RSSI = rssi_mean;
        
        state = 0;
	  #if (SX1272_debug_mode > 0)
		  printf("## RSSI value is ");
		  printf("%d", _RSSI);
		  printf(" ##\n");
		  printf("\n");
	  #endif
	  
  }
  else
  { 
  		/// FSK mode
        // get mean value of RSSI
        for(int i = 0; i < total; i++)
        {
            _RSSI = (readRegister(REG_RSSI_VALUE_FSK) >> 1);
            rssi_mean += _RSSI;
        }
        rssi_mean = rssi_mean / total;  
        _RSSI = rssi_mean;
        
        state = 0;

	  #if (SX1272_debug_mode > 0)
		  printf("## RSSI value is ");
		  printf("%d", _RSSI);
		  printf(" ##\n");
		  printf("\n");
	  #endif 
  }
  return state;
}

/*
 Function: Gets the RSSI of the last packet received in LoRa mode.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/
int16_t SX1272::getRSSIpacket()
{	// RSSIpacket only exists in LoRa
  int8_t state = 2;

  #if (SX1272_debug_mode > 1)
	  printf("\n");
	  printf("Starting 'getRSSIpacket'\n");
  #endif

  state = 1;
  if( _modem == LORA )
  { // LoRa mode
	  state = getSNR();
	  if( state == 0 )
	  {
		  if( _SNR < 0 )
		  {
			  _RSSIpacket = -NOISE_ABSOLUTE_ZERO + 10.0 * SignalBwLog[_bandwidth] + NOISE_FIGURE + ( double )_SNR;
			  state = 0;
		  }
		  else
		  {
			  _RSSIpacket = readRegister(REG_PKT_RSSI_VALUE);
			  _RSSIpacket = -OFFSET_RSSI + ( double )_RSSIpacket;
			  state = 0;
		  }
	  #if (SX1272_debug_mode > 0)
		  printf("## RSSI packet value is ");
		  printf("%d", _RSSIpacket);
  		  printf(" ##\n");
		  printf("\n");
	  #endif
	  }
  }
  else
  { // RSSI packet doesn't exist in FSK mode
	state = -1;
	#if (SX1272_debug_mode > 0)
		printf("** RSSI packet does not exist in FSK mode **\n");
		printf("\n");
	#endif
  }
  return state;
}

/*
 Function: It sets the maximum number of retries.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 -->
*/
uint8_t SX1272::setRetries(uint8_t ret)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setRetries'\n");
	#endif

	state = 1;
	if( ret > MAX_RETRIES )
	{
		state = -1;
		#if (SX1272_debug_mode > 1)
			printf("** Retries value can't be greater than ");
			printf("%d", MAX_RETRIES);
			printf(" **\n");
			printf("\n");
		#endif
	}
	else
	{
		_maxRetries = ret;
		state = 0;
		#if (SX1272_debug_mode > 1)
			printf("## Maximum retries value = ");
			printf("%d", _maxRetries);
			printf(" ##\n");
			printf("\n");
		#endif
	}
	return state;
}

/*
 Function: Gets the current supply limit of the power amplifier, protecting battery chemistries.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
 Parameters:
   rate: value to compute the maximum current supply. Maximum current is 45+5*'rate' [mA]
*/
uint8_t SX1272::getMaxCurrent()
{
	int8_t state = 2;
	byte value;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getMaxCurrent'\n");
	#endif

	state = 1;
	_maxCurrent = readRegister(REG_OCP);

	// extract only the OcpTrim value from the OCP register
    _maxCurrent &= 0B00011111;

	if( _maxCurrent <= 15 )
	{
		value = (45 + (5 * _maxCurrent));
	}
	else if( _maxCurrent <= 27 )
	{
		value = (-30 + (10 * _maxCurrent));
	}
	else
    {
        value = 240;        
    }

	_maxCurrent = value;
	#if (SX1272_debug_mode > 1)
		printf("## Maximum current supply configured is ");
		printf("%d", value);
		printf(" mA ##\n");
		printf("\n");
	#endif
	state = 0;
	return state;
}

/*
 Function: Limits the current supply of the power amplifier, protecting battery chemistries.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden parameter value for this function
 Parameters:
   rate: value to compute the maximum current supply. Maximum current is 45+5*'rate' [mA]
*/
int8_t SX1272::setMaxCurrent(uint8_t rate)
{
	int8_t state = 2;
	byte st0;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setMaxCurrent'\n");
	#endif

	// Maximum rate value = 0x1B, because maximum current supply = 240 mA
	if (rate > 0x1B)
	{
		state = -1;
		#if (SX1272_debug_mode > 1)
			printf("** Maximum current supply is 240 mA, ");
			printf("so maximum parameter value must be 27 (DEC) or 0x1B (HEX) **\n");
			printf("\n");
		#endif
	}
	else
	{
		// Enable Over Current Protection
        rate |= 0B00100000;

		state = 1;
		st0 = readRegister(REG_OP_MODE);	// Save the previous status
		if( _modem == LORA )
		{ // LoRa mode
			writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Set LoRa Standby mode to write in registers
		}
		else
		{ // FSK mode
			writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Set FSK Standby mode to write in registers
		}
		writeRegister(REG_OCP, rate);		// Modifying maximum current supply
		writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
		state = 0;
	}
	return state;
}

/*
 Function: Gets the content of different registers.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getRegs()
{
	int8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getRegs'\n");
	#endif

	state_f = 1;
	state = getMode();			// Stores the BW, CR and SF.
	if( state == 0 )
	{
		state = getPower();		// Stores the power.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting mode **\n");
		#endif
	}
 	if( state == 0 )
	{
		state = getChannel();	// Stores the channel.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting power **\n");
		#endif
	}
	if( state == 0 )
	{
		state = getCRC();		// Stores the CRC configuration.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting channel **\n");
		#endif
	}
	if( state == 0 )
	{
		state = getHeader();	// Stores the header configuration.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting CRC **\n");
		#endif
	}
	if( state == 0 )
	{
		state = getPreambleLength();	// Stores the preamble length.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting header **\n");
		#endif
	}
	if( state == 0 )
	{
		state = getPayloadLength();		// Stores the payload length.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting preamble length **\n");
		#endif
	}
	if( state == 0 )
	{
		state = getNodeAddress();		// Stores the node address.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting payload length **\n");
		#endif
	}
	if( state == 0 )
	{
		state = getMaxCurrent();		// Stores the maximum current supply.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting node address **\n");
		#endif
	}
	if( state == 0 )
	{
		state_f = getTemp();		// Stores the module temperature.
	}
	else
	{
		state_f = 1;
		#if (SX1272_debug_mode > 1)
			printf("** Error getting maximum current supply **\n");
		#endif
	}
	if( state_f != 0 )
	{
		#if (SX1272_debug_mode > 1)
			printf("** Error getting temperature **\n");
			printf("\n");
		#endif
	}
	return state_f;
}

/*
 Function: It truncs the payload length if it is greater than 0xFF.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::truncPayload(uint16_t length16)
{
	uint8_t state = 2;

	state = 1;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'truncPayload'\n");
	#endif

	if( length16 > MAX_PAYLOAD )
	{
		_payloadlength = MAX_PAYLOAD;
	}
	else
	{
		_payloadlength = (length16 & 0xFF);
	}
	state = 0;

	return state;
}

/*
 Function: It sets an ACK in FIFO in order to send it.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setACK()
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setACK'\n");
	#endif

	// delayMicroseconds(1000);

	clearFlags();	// Initializing flags

	if( _modem == LORA )
	{ // LoRa mode
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby LoRa mode to write in FIFO
	}
	else
	{ // FSK mode
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Stdby FSK mode to write in FIFO
	}

	// Setting ACK length in order to send it
	state = setPacketLength(ACK_LENGTH);
	if( state == 0 )
	{
		// Setting ACK
		ACK.dst = packet_received.src; // ACK destination is packet source
		ACK.src = packet_received.dst; // ACK source is packet destination
		ACK.packnum = packet_received.packnum; // packet number that has been correctly received
		ACK.length = 0;		  // length = 0 to show that's an ACK
		ACK.data[0] = _reception;	// CRC of the received packet

		// Setting address pointer in FIFO data buffer
		writeRegister(REG_FIFO_ADDR_PTR, 0x80);

		state = 1;

		// Writing ACK to send in FIFO
		writeRegister(REG_FIFO, ACK.dst); 		// Writing the destination in FIFO
		writeRegister(REG_FIFO, ACK.src);		// Writing the source in FIFO
		writeRegister(REG_FIFO, ACK.packnum);	// Writing the packet number in FIFO
		writeRegister(REG_FIFO, ACK.length); 	// Writing the packet length in FIFO
		writeRegister(REG_FIFO, ACK.data[0]);	// Writing the ACK in FIFO

		#if (SX1272_debug_mode > 0)
			printf("## ACK set and written in FIFO ##\n");
			// Print the complete ACK if debug_mode
			printf("## ACK to send:\n");
			printf("Destination: ");
			printf("%d\n", ACK.dst);			 	// Printing destination
			printf("Source: ");
			printf("%d\n", ACK.src);			 	// Printing source
			printf("ACK number: ");
			printf("%d\n", ACK.packnum);			// Printing ACK number
			printf("ACK length: ");
			printf("%d\n", ACK.length);				// Printing ACK length
			printf("ACK payload: ");
			printf("%d\n", ACK.data[0]);			// Printing ACK payload
			printf(" ##\n");
			printf("\n");
		#endif

		state = 0;
		_reception = CORRECT_PACKET;		// Updating value to next packet

		delayMicroseconds(500);
	}
	return state;
}

/*
 Function: Configures the module to receive information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receive()
{
	  uint8_t state = 2;

	  #if (SX1272_debug_mode > 1)
		  printf("\n");
		  printf("Starting 'receive'\n");
	  #endif

	  // Initializing packet_received struct
	  memset( &packet_received, 0x00, sizeof(packet_received) );

		// Setting Testmode
   		 writeRegister(0x31,0x43);
    	// Set LowPnTxPllOff 
    	writeRegister(REG_PA_RAMP, 0x09);

	  writeRegister(REG_LNA, 0x23);			// Important in reception
	  writeRegister(REG_FIFO_ADDR_PTR, 0x00);  // Setting address pointer in FIFO data buffer
  	  // change RegSymbTimeoutLsb 
      writeRegister(REG_SYMB_TIMEOUT_LSB, 0xFF);
  	  writeRegister(REG_FIFO_RX_BYTE_ADDR, 0x00); // Setting current value of reception buffer pointer
	 // clearFlags();						// Initializing flags
	  state = 1;
    
    //printf("_modem: %d\n", _modem);
	  if( _modem == LORA )
	  { // LoRa mode
	  	  state = setPacketLength(MAX_LENGTH);	// With MAX_LENGTH gets all packets with length < MAX_LENGTH
		  writeRegister(REG_OP_MODE, LORA_RX_MODE);  	  // LORA mode - Rx
		  #if (SX1272_debug_mode > 1)
		  	  printf("## Receiving LoRa mode activated with success ##\n");
		  	  printf("\n");
		  #endif
	  }
	  else
	  { // FSK mode
		  state = setPacketLength();
		  writeRegister(REG_OP_MODE, FSK_RX_MODE);  // FSK mode - Rx
		  #if (SX1272_debug_mode > 1)
		  	  printf("## Receiving FSK mode activated with success ##\n");
		  	  printf("\n");
		  #endif
	  }
	  return state;
}

/*
 Function: Configures the module to receive information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receivePacketMAXTimeout()
{
	return receivePacketTimeout(MAX_TIMEOUT);
}

/*
 Function: Configures the module to receive information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receivePacketTimeout()
{
	setTimeout();
	return receivePacketTimeout(_sendTime);
}

/*
 Function: Configures the module to receive information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receivePacketTimeout(uint16_t wait)
{
	uint8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'receivePacketTimeout'\n");
	#endif

	state = receive();
	if( state == 0 )
	{
		if( availableData(wait) )
		{
			// If packet received, getPacket
			state_f = getPacket();
		}
		else
		{
			state_f = 1;
		}
	}
	else
	{
		state_f = state;
	}
	return state_f;
}

/*
 Function: Configures the module to receive information and send an ACK.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receivePacketMAXTimeoutACK()
{
	return receivePacketTimeoutACK(MAX_TIMEOUT);
}

/*
 Function: Configures the module to receive information and send an ACK.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receivePacketTimeoutACK()
{
	setTimeout();
	return receivePacketTimeoutACK(_sendTime);
}

/*
 Function: Configures the module to receive information and send an ACK.
 Returns: Integer that determines if there has been any error
   state = 4  --> The command has been executed but the packet received is incorrect
   state = 3  --> The command has been executed but there is no packet received
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receivePacketTimeoutACK(uint16_t wait)
{
	uint8_t state = 2;
	uint8_t state_f = 2;


	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'receivePacketTimeoutACK'\n");
	#endif

	state = receive();
	if( state == 0 )
	{
		if( availableData(wait) )
		{
			state = getPacket();
		}
		else
		{
			state = 1;
			state_f = 3;  // There is no packet received
		}
	}
	else
	{
		state = 1;
		state_f = 1; // There has been an error with the 'receive' function
	}
	if( (state == 0) || (state == 3) )
	{
		if( _reception == INCORRECT_PACKET )
		{
			state_f = 4;  // The packet has been incorrectly received
		}
		else
		{
			state_f = 1;  // The packet has been correctly received
		}
		state = setACK();
		if( state == 0 )
		{
			state = sendWithTimeout();
			if( state == 0 )
			{
			state_f = 0;
			#if (SX1272_debug_mode > 1)
				printf("This last packet was an ACK, so ...\n");
				printf("ACK successfully sent\n");
				printf("\n");
			#endif
			}
			else
			{
				state_f = 1; // There has been an error with the 'sendWithTimeout' function
			}
		}
		else
		{
			state_f = 1; // There has been an error with the 'setACK' function
		}
	}
	else
	{
		state_f = 1;
	}
	return state_f;
}

/*
 Function: Configures the module to receive all the information on air, before MAX_TIMEOUT expires.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t	SX1272::receiveAll()
{
	return receiveAll(MAX_TIMEOUT);
}

/*
 Function: Configures the module to receive all the information on air.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::receiveAll(uint16_t wait)
{
	  uint8_t state = 2;
	  byte config1;

	  #if (SX1272_debug_mode > 1)
		  printf("\n");
		  printf("Starting 'receiveAll'\n");
	  #endif

	  if( _modem == FSK )
	  { // FSK mode
		 writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);		// Setting standby FSK mode
		 config1 = readRegister(REG_PACKET_CONFIG1);
		 config1 = config1 & 0B11111001;			// clears bits 2-1 from REG_PACKET_CONFIG1
		 writeRegister(REG_PACKET_CONFIG1, config1);		// AddressFiltering = None
	  }
	  #if (SX1272_debug_mode > 1)
		  printf("## Address filtering desactivated ##\n");
		  printf("\n");
	  #endif
	  state = receive();	// Setting Rx mode
	  if( state == 0 )
	  {
		  state = getPacket(wait);	// Getting all packets received in wait
	  }
	  return state;
}

/*
 Function: If a packet is received, checks its destination.
 Returns: Boolean that's 'true' if the packet is for the module and
		  it's 'false' if the packet is not for the module.
*/
boolean	SX1272::availableData()
{
	return availableData(MAX_TIMEOUT);
}

/*
 Function: If a packet is received, checks its destination.
 Returns: Boolean that's 'true' if the packet is for the module and
		  it's 'false' if the packet is not for the module.
 Parameters:
   wait: time to wait while there is no a valid header received.
*/
boolean	SX1272::availableData(uint16_t wait)
{
	byte value;
	byte header = 0;
	boolean forme = false;
	boolean	_hreceived = false;
	unsigned long previous;


	#if (SX1272_debug_mode > 0)
		printf("\n");
		printf("Starting 'availableData'\n");
	#endif

	previous = millis();
	if( _modem == LORA )
	{ // LoRa mode
		value = readRegister(REG_IRQ_FLAGS);
		// Wait to ValidHeader interrupt
		while( (bitRead(value, 4) == 0) && (millis() - previous < (unsigned long)wait) )
		{
			value = readRegister(REG_IRQ_FLAGS);
			// Condition to avoid an overflow (DO NOT REMOVE)
			if( millis() < previous )
			{
				previous = millis();
			}
		} // end while (millis)
		if( bitRead(value, 4) == 1 )
		{ // header received
			#if (SX1272_debug_mode > 0)
				printf("## Valid Header received in LoRa mode ##\n");
			#endif
			_hreceived = true;
			while( (header == 0) && (millis() - previous < (unsigned long)wait) )
			{ // Waiting to read first payload bytes from packet
				header = readRegister(REG_FIFO_RX_BYTE_ADDR);
                 delay(1000);
				// Condition to avoid an overflow (DO NOT REMOVE)
				if( millis() < previous )
				{
					previous = millis();
				}
			}
			if( header != 0 )
			{ // Reading first byte of the received packet
				_destination = readRegister(REG_FIFO);
			}
		}
		else
		{
			forme = false;
			_hreceived = false;
			#if (SX1272_debug_mode > 0)
				printf("** The timeout has expired **\n");
				printf("\n");
			#endif
		}
	}
	else
	{ // FSK mode
		value = readRegister(REG_IRQ_FLAGS2);
		// Wait to Payload Ready interrupt
		while( (bitRead(value, 2) == 0) && (millis() - previous < wait) )
		{
			value = readRegister(REG_IRQ_FLAGS2);
			// Condition to avoid an overflow (DO NOT REMOVE)
			if( millis() < previous )
			{
				previous = millis();
			}
		}// end while (millis)
		if( bitRead(value, 2) == 1 )	// something received
		{
			_hreceived = true;
			#if (SX1272_debug_mode > 0)
				printf("## Valid Preamble detected in FSK mode ##\n");
			#endif
			// Reading first byte of the received packet
			_destination = readRegister(REG_FIFO);
		}
		else
		{
			forme = false;
			_hreceived = false;
			#if (SX1272_debug_mode > 0)
				printf("** The timeout has expired **\n");
				printf("\n");
			#endif
		}
	}
// We use _hreceived because we need to ensure that _destination value is correctly
// updated and is not the _destination value from the previously packet
	if( _hreceived )
	{ // Checking destination
		#if (SX1272_debug_mode > 0)
			printf("## Checking destination ##\n");
		#endif
		if( (_destination == _nodeAddress) || (_destination == BROADCAST_0) )
		{ // LoRa or FSK mode
			forme = true;
			#if (SX1272_debug_mode > 0)
				printf("## Packet received is for me ##\n");
			#endif
		}
		else
		{
			forme = false;
			#if (SX1272_debug_mode > 0)
				printf("## Packet received is not for me ##\n");
				printf("\n");
			#endif
			if( _modem == LORA )	// STANDBY PARA MINIMIZAR EL CONSUMO
			{ // LoRa mode
				writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Setting standby LoRa mode
			}
			else
			{ //  FSK mode
				writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Setting standby FSK mode
			}
		}
	}
//----else
//	{
//	}
	return forme;
}

/*
 Function: It gets and stores a packet if it is received before MAX_TIMEOUT expires.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getPacketMAXTimeout()
{
	return getPacket(MAX_TIMEOUT);
}

/*
 Function: It gets and stores a packet if it is received.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
int8_t SX1272::getPacket()
{
	return getPacket(MAX_TIMEOUT);
}

/*
 Function: It gets and stores a packet if it is received before ending 'wait' time.
 Returns:  Integer that determines if there has been any error
   state = 3  --> The command has been executed but packet has been incorrectly received
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden parameter value for this function
 Parameters:
   wait: time to wait while there is no a valid header received.
*/
int8_t SX1272::getPacket(uint16_t wait)
{
	uint8_t state = 2;
	byte value = 0x00;
	unsigned long previous;
	boolean p_received = false;

	#if (SX1272_debug_mode > 0)
		printf("\n");
		printf("Starting 'getPacket'\n");
	#endif

	previous = millis();
	if( _modem == LORA )
	{ // LoRa mode
		value = readRegister(REG_IRQ_FLAGS);
		// Wait until the packet is received (RxDone flag) or the timeout expires
		while( (bitRead(value, 6) == 0) && (millis() - previous < (unsigned long)wait) )
		{
			value = readRegister(REG_IRQ_FLAGS);
			// Condition to avoid an overflow (DO NOT REMOVE)
			if( millis() < previous )
			{
				previous = millis();
			}
		} // end while (millis)

		if( (bitRead(value, 6) == 1) && (bitRead(value, 5) == 0) )
		{ // packet received & CRC correct
			p_received = true;	// packet correctly received
			_reception = CORRECT_PACKET;
			#if (SX1272_debug_mode > 0)
				printf("## Packet correctly received in LoRa mode ##\n");
			#endif
		}
		else
		{
			if( bitRead(value, 5) != 0 )
			{ // CRC incorrect
				_reception = INCORRECT_PACKET;
				state = 3;
				#if (SX1272_debug_mode > 0)
					printf("** The CRC is incorrect **\n");
					printf("\n");
				#endif
			}
		}
		//writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Setting standby LoRa mode
	}
	else
	{ // FSK mode
		value = readRegister(REG_IRQ_FLAGS2);
		while( (bitRead(value, 2) == 0) && (millis() - previous < wait) )
		{
			value = readRegister(REG_IRQ_FLAGS2);
			// Condition to avoid an overflow (DO NOT REMOVE)
			if( millis() < previous )
			{
				previous = millis();
			}
		} // end while (millis)
		if( bitRead(value, 2) == 1 )
		{ // packet received
 			if( bitRead(value, 1) == 1 )
			{ // CRC correct
				_reception = CORRECT_PACKET;
				p_received = true;
				#if (SX1272_debug_mode > 0)
					printf("## Packet correctly received in FSK mode ##\n");
				#endif
			}
			else
			{ // CRC incorrect
				_reception = INCORRECT_PACKET;
				state = 3;
				p_received = false;
				#if (SX1272_debug_mode > 0)
					printf("## Packet incorrectly received in FSK mode ##\n");
				#endif
			}
		}
		else
		{
			#if (SX1272_debug_mode > 0)
				printf("** The timeout has expired **\n");
				printf("\n");
			#endif
		}
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Setting standby FSK mode
	}
	if( p_received )
	{
		// Store the packet
		if( _modem == LORA )
		{
			writeRegister(REG_FIFO_ADDR_PTR, 0x00);  		// Setting address pointer in FIFO data buffer
			packet_received.dst = readRegister(REG_FIFO);	// Storing first byte of the received packet
		}
		else
		{
			value = readRegister(REG_PACKET_CONFIG1);
			if( (bitRead(value, 2) == 0) && (bitRead(value, 1) == 0) )
			{
				packet_received.dst = readRegister(REG_FIFO); // Storing first byte of the received packet
			}
			else
			{
				packet_received.dst = _destination;			// Storing first byte of the received packet
			}
		}
		packet_received.src = readRegister(REG_FIFO);		// Reading second byte of the received packet
		packet_received.packnum = readRegister(REG_FIFO);	// Reading third byte of the received packet
		packet_received.length = readRegister(REG_FIFO);	// Reading fourth byte of the received packet
		if( _modem == LORA )
		{
			_payloadlength = packet_received.length - OFFSET_PAYLOADLENGTH;
		}
		if( packet_received.length > (MAX_LENGTH + 1) )
		{
			#if (SX1272_debug_mode > 0)
				printf("Corrupted packet, length must be less than 256\n");
			#endif
		}
		else
		{
			for(unsigned int i = 0; i < _payloadlength; i++)
			{
				packet_received.data[i] = readRegister(REG_FIFO); // Storing payload
			}
			packet_received.retry = readRegister(REG_FIFO);
			// Print the packet if debug_mode
			#if (SX1272_debug_mode > 0)
				printf("## Packet received:\n");
				printf("Destination: ");
				printf("%d\n", packet_received.dst);			 	// Printing destination
				printf("Source: ");
				printf("%d\n", packet_received.src);			 	// Printing source
				printf("Packet number: ");
				printf("%d\n", packet_received.packnum);			// Printing packet number
				printf("Packet length: ");
				printf("%d\n", packet_received.length);			// Printing packet length
				printf("Data: ");
				for(unsigned int i = 0; i < _payloadlength; i++)
				{
					printf("%c", packet_received.data[i]);		// Printing payload
				}
				printf("\n");
				printf("Retry number: ");
				printf("%d\n", packet_received.retry);			// Printing number retry
				printf(" ##\n");
				printf("\n");
			#endif
			state = 0;
		}
	}
	else
	{
		state = 1;
		if( (_reception == INCORRECT_PACKET) && (_retries < _maxRetries) )
		{
			_retries++;
			#if (SX1272_debug_mode > 0)
				printf("## Retrying to send the last packet ##\n");
				printf("\n");
			#endif
		}
	}
	if( _modem == LORA )
	{
		writeRegister(REG_FIFO_ADDR_PTR, 0x00);  // Setting address pointer in FIFO data buffer
	}
	clearFlags();	// Initializing flags
	if( wait > MAX_WAIT )
	{
		state = -1;
		#if (SX1272_debug_mode > 0)
			printf("** The timeout must be smaller than 12.5 seconds **\n");
			printf("\n");
		#endif
	}

	return state;
}

/*
 Function: It sets the packet destination.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
 Parameters:
   dest: destination value of the packet sent.
*/
int8_t SX1272::setDestination(uint8_t dest)
{
	int8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setDestination'\n");
	#endif

	state = 1;
	_destination = dest; // Storing destination in a global variable
	packet_sent.dst = dest;	 // Setting destination in packet structure
	packet_sent.src = _nodeAddress; // Setting source in packet structure
	packet_sent.packnum = _packetNumber;	// Setting packet number in packet structure
	_packetNumber++;
	state = 0;

	#if (SX1272_debug_mode > 1)
		printf("## Destination ");
		printf("%X", _destination);
		printf(" successfully set ##\n");
		printf("## Source ");
		printf("%d", packet_sent.src);
		printf(" successfully set ##\n");
		printf("## Packet number ");
		printf("%d", packet_sent.packnum);
		printf(" successfully set ##\n");
		printf("\n");
	#endif
	return state;
}

/*
 Function: It sets the timeout according to the configured mode.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setTimeout()
{
	uint8_t state = 2;
	uint16_t delay;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setTimeout'\n");
	#endif

	state = 1;
	if( _modem == LORA )
	{
		switch(_spreadingFactor)
		{	// Choosing Spreading Factor
			case SF_6:	switch(_bandwidth)
						{	// Choosing bandwidth
							case BW_125:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 335;
														break;
												case CR_6: _sendTime = 352;
														break;
												case CR_7: _sendTime = 368;
														break;
												case CR_8: _sendTime = 386;
														break;
											}
											break;
							case BW_250:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 287;
														break;
												case CR_6: _sendTime = 296;
														break;
												case CR_7: _sendTime = 305;
														break;
												case CR_8: _sendTime = 312;
														break;
											}
											break;
							case BW_500:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 242;
														break;
												case CR_6: _sendTime = 267;
														break;
												case CR_7: _sendTime = 272;
														break;
												case CR_8: _sendTime = 276;
														break;
											}
											break;
						}
						break;

			case SF_7:	switch(_bandwidth)
						{	// Choosing bandwidth
							case BW_125:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 408;
														break;
												case CR_6: _sendTime = 438;
														break;
												case CR_7: _sendTime = 468;
														break;
												case CR_8: _sendTime = 497;
														break;
											}
											break;
							case BW_250:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 325;
														break;
												case CR_6: _sendTime = 339;
														break;
												case CR_7: _sendTime = 355;
														break;
												case CR_8: _sendTime = 368;
														break;
											}
											break;
							case BW_500:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 282;
														break;
												case CR_6: _sendTime = 290;
														break;
												case CR_7: _sendTime = 296;
														break;
												case CR_8: _sendTime = 305;
														break;
											}
											break;
						}
						break;

			case SF_8:	switch(_bandwidth)
						{	// Choosing bandwidth
							case BW_125:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 537;
														break;
												case CR_6: _sendTime = 588;
														break;
												case CR_7: _sendTime = 640;
														break;
												case CR_8: _sendTime = 691;
														break;
											}
											break;
							case BW_250:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 388;
														break;
												case CR_6: _sendTime = 415;
														break;
												case CR_7: _sendTime = 440;
														break;
												case CR_8: _sendTime = 466;
														break;
											}
											break;
							case BW_500:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 315;
														break;
												case CR_6: _sendTime = 326;
														break;
												case CR_7: _sendTime = 340;
														break;
												case CR_8: _sendTime = 352;
														break;
											}
											break;
						}
						break;

			case SF_9:	switch(_bandwidth)
						{	// Choosing bandwidth
							case BW_125:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 774;
														break;
												case CR_6: _sendTime = 864;
														break;
												case CR_7: _sendTime = 954;
														break;
												case CR_8: _sendTime = 1044;
														break;
											}
											break;
							case BW_250:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 506;
														break;
												case CR_6: _sendTime = 552;
														break;
												case CR_7: _sendTime = 596;
														break;
												case CR_8: _sendTime = 642;
														break;
											}
											break;
							case BW_500:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 374;
														break;
												case CR_6: _sendTime = 396;
														break;
												case CR_7: _sendTime = 418;
														break;
												case CR_8: _sendTime = 441;
														break;
											}
											break;
						}
						break;

			case SF_10:	switch(_bandwidth)
						{	// Choosing bandwidth
							case BW_125:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 1226;
														break;
												case CR_6: _sendTime = 1388;
														break;
												case CR_7: _sendTime = 1552;
														break;
												case CR_8: _sendTime = 1716;
														break;
											}
											break;
							case BW_250:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 732;
														break;
												case CR_6: _sendTime = 815;
														break;
												case CR_7: _sendTime = 896;
														break;
												case CR_8: _sendTime = 977;
														break;
											}
											break;
							case BW_500:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 486;
														break;
												case CR_6: _sendTime = 527;
														break;
												case CR_7: _sendTime = 567;
														break;
												case CR_8: _sendTime = 608;
														break;
											}
											break;
						}
						break;

			case SF_11:	switch(_bandwidth)
						{	// Choosing bandwidth
							case BW_125:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 2375;
														break;
												case CR_6: _sendTime = 2735;
														break;
												case CR_7: _sendTime = 3095;
														break;
												case CR_8: _sendTime = 3456;
														break;
											}
											break;
							case BW_250:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 1144;
														break;
												case CR_6: _sendTime = 1291;
														break;
												case CR_7: _sendTime = 1437;
														break;
												case CR_8: _sendTime = 1586;
														break;
											}
											break;
							case BW_500:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 691;
														break;
												case CR_6: _sendTime = 766;
														break;
												case CR_7: _sendTime = 838;
														break;
												case CR_8: _sendTime = 912;
														break;
											}
											break;
						}
						break;

			case SF_12: switch(_bandwidth)
						{	// Choosing bandwidth
							case BW_125:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 4180;
														break;
												case CR_6: _sendTime = 4836;
														break;
												case CR_7: _sendTime = 5491;
														break;
												case CR_8: _sendTime = 6146;
														break;
											}
											break;
							case BW_250:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 1965;
														break;
												case CR_6: _sendTime = 2244;
														break;
												case CR_7: _sendTime = 2521;
														break;
												case CR_8: _sendTime = 2800;
														break;
											}
											break;
							case BW_500:	switch(_codingRate)
											{	// Choosing coding rate
												case CR_5: _sendTime = 1102;
														break;
												case CR_6: _sendTime = 1241;
														break;
												case CR_7: _sendTime = 1381;
														break;
												case CR_8: _sendTime = 1520;
														break;
											}
											break;
						}
						break;
			default: _sendTime = MAX_TIMEOUT;
		}
	}
	else
	{
		_sendTime = MAX_TIMEOUT;
	}
	delay = ((0.1*_sendTime) + 1);
	_sendTime = (uint16_t) ((_sendTime * 1.2) + (rand()%delay));
	#if (SX1272_debug_mode > 1)
		printf("Timeout to send/receive is: ");
		printf("%d",_sendTime);
		printf("\n");
	#endif
	state = 0;
	return state;
}

/*
 Function: It sets a char array payload packet in a packet struct.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setPayload(char *payload)
{
	uint8_t state = 2;
	uint8_t state_f = 2;
	uint16_t length16;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setPayload'\n");
	#endif

	state = 1;
	length16 = (uint16_t)strlen(payload);
	state = truncPayload(length16);
	if( state == 0 )
	{
		// fill data field until the end of the string
		for(unsigned int i = 0; i < _payloadlength; i++)
		{
			packet_sent.data[i] = payload[i];
		}
	}
	else
	{
		state_f = state;
	}
	if( ( _modem == FSK ) && ( _payloadlength > MAX_PAYLOAD_FSK ) )
	{
		_payloadlength = MAX_PAYLOAD_FSK;
		state = 1;
		#if (SX1272_debug_mode > 1)
			printf("In FSK, payload length must be less than 60 bytes.\n");
			printf("\n");
		#endif
	}
	// set length with the actual counter value
	state_f = setPacketLength();	// Setting packet length in packet structure
	return state_f;
}

/*
 Function: It sets an uint8_t array payload packet in a packet struct.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setPayload(uint8_t *payload)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setPayload'\n");
	#endif

	state = 1;
	if( ( _modem == FSK ) && ( _payloadlength > MAX_PAYLOAD_FSK ) )
	{
		_payloadlength = MAX_PAYLOAD_FSK;
		state = 1;
		#if (SX1272_debug_mode > 1)
			printf("In FSK, payload length must be less than 60 bytes.\n");
			printf("\n");
		#endif
	}
	for(unsigned int i = 0; i < _payloadlength; i++)
	{
		packet_sent.data[i] = payload[i];	// Storing payload in packet structure
	}
	// set length with the actual counter value
    state = setPacketLength();	// Setting packet length in packet structure
	return state;
}

/*
 Function: It sets a packet struct in FIFO in order to sent it.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setPacket(uint8_t dest, char *payload)
{
	int8_t state = 2;


	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setPacket'\n");
	#endif

	clearFlags();	// Initializing flags

	if( _modem == LORA )
	{ // LoRa mode
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby LoRa mode to write in FIFO
	}
	else
	{ // FSK mode
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Stdby FSK mode to write in FIFO
	}

	_reception = CORRECT_PACKET;	// Updating incorrect value
	if( _retries == 0 )
	{ // Updating this values only if is not going to re-send the last packet
		state = setDestination(dest);	// Setting destination in packet structure
		if( state == 0 )
		{
			state = setPayload(payload);
		}
	}
	else
	{
		if( _retries == 1 )
		{
			packet_sent.length++;
		}
		state = setPacketLength();
		packet_sent.retry = _retries;
		#if (SX1272_debug_mode > 0)
			printf("** Retrying to send last packet ");
			printf("%d", _retries);
			printf(" time **\n");
		#endif
	}
	writeRegister(REG_FIFO_ADDR_PTR, 0x80);  // Setting address pointer in FIFO data buffer
	if( state == 0 )
	{
		state = 1;
		// Writing packet to send in FIFO
		writeRegister(REG_FIFO, packet_sent.dst); 		// Writing the destination in FIFO
		writeRegister(REG_FIFO, packet_sent.src);		// Writing the source in FIFO
		writeRegister(REG_FIFO, packet_sent.packnum);	// Writing the packet number in FIFO
		writeRegister(REG_FIFO, packet_sent.length); 	// Writing the packet length in FIFO
		for(unsigned int i = 0; i < _payloadlength; i++)
		{
			writeRegister(REG_FIFO, packet_sent.data[i]);  // Writing the payload in FIFO
		}
		writeRegister(REG_FIFO, packet_sent.retry);		// Writing the number retry in FIFO
		state = 0;
		#if (SX1272_debug_mode > 0)
				printf("## Packet set and written in FIFO ##\n");
				// Print the complete packet if debug_mode
				printf("## Packet to send:  \n");
				printf("Destination: ");
				printf("%d\n", packet_sent.dst);			 	// Printing destination
				printf("Source: ");
				printf("%d\n", packet_sent.src);			 	// Printing source
				printf("Packet number: ");
				printf("%d\n", packet_sent.packnum);			// Printing packet number
				printf("Packet length: ");
				printf("%d\n", packet_sent.length);			// Printing packet length
				printf("Data: ");
				for(unsigned int i = 0; i < _payloadlength; i++)
				{
					printf("%c", packet_sent.data[i]);		// Printing payload
				}
				printf("\n");
				printf("Retry number: ");
				printf("%d\n", packet_sent.retry);			// Printing number retry
				printf(" ##\n");
				printf("\n");
			#endif
	}

	return state;
}

/*
 Function: It sets a packet struct in FIFO in order to sent it.
 Returns:  Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::setPacket(uint8_t dest, uint8_t *payload)
{
	int8_t state = 2;
	byte st0;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'setPacket'\n");
	#endif

	st0 = readRegister(REG_OP_MODE);	// Save the previous status
	clearFlags();	// Initializing flags

	if( _modem == LORA )
	{ // LoRa mode
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby LoRa mode to write in FIFO
	}
	else
	{ // FSK mode
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Stdby FSK mode to write in FIFO
	}

	_reception = CORRECT_PACKET;	// Updating incorrect value to send a packet (old or new)
	if( _retries == 0 )
	{ // Sending new packet
		state = setDestination(dest);	// Setting destination in packet structure
		if( state == 0 )
		{
			state = setPayload(payload);
		}
	}
	else
	{
		if( _retries == 1 )
		{
			packet_sent.length++;
		}
		state = setPacketLength();
		packet_sent.retry = _retries;
		#if (SX1272_debug_mode > 0)
			printf("** Retrying to send last packet ");
			printf("%d", _retries);
			printf(" time **\n");
		#endif
	}
	writeRegister(REG_FIFO_ADDR_PTR, 0x80);  // Setting address pointer in FIFO data buffer
	if( state == 0 )
	{
		state = 1;
		// Writing packet to send in FIFO
		writeRegister(REG_FIFO, packet_sent.dst); 		// Writing the destination in FIFO
		writeRegister(REG_FIFO, packet_sent.src);		// Writing the source in FIFO
		writeRegister(REG_FIFO, packet_sent.packnum);	// Writing the packet number in FIFO
		writeRegister(REG_FIFO, packet_sent.length); 	// Writing the packet length in FIFO
		for(unsigned int i = 0; i < _payloadlength; i++)
		{
			writeRegister(REG_FIFO, packet_sent.data[i]);  // Writing the payload in FIFO
		}
		writeRegister(REG_FIFO, packet_sent.retry);		// Writing the number retry in FIFO
		state = 0;
			#if (SX1272_debug_mode > 0)
				printf("## Packet set and written in FIFO ##\n");
				// Print the complete packet if debug_mode
				printf("## Packet to send:  \n");
				printf("Destination: ");
				printf("%d\n", packet_sent.dst);			 	// Printing destination
				printf("Source: ");
				printf("%d\n", packet_sent.src);			 	// Printing source
				printf("Packet number: ");
				printf("%d\n", packet_sent.packnum);			// Printing packet number
				printf("Packet length: ");
				printf("%d\n", packet_sent.length);			// Printing packet length
				printf("Data: ");
				for(unsigned int i = 0; i < _payloadlength; i++)
				{
					printf("%c", packet_sent.data[i]);		// Printing payload
				}
				printf("\n");
				printf("Retry number: ");
				printf("%d\n", packet_sent.retry);			// Printing number retry
				printf(" ##\n");
				printf("\n");
			#endif
	}
	writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
	return state;
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendWithMAXTimeout()
{
	return sendWithTimeout(MAX_TIMEOUT);
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendWithTimeout()
{
	setTimeout();
	return sendWithTimeout(_sendTime);
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendWithTimeout(uint16_t wait)
{
	  uint8_t state = 2;
	  byte value = 0x00;
	  unsigned long previous;

	  #if (SX1272_debug_mode > 1)
		  printf("\n");
	      printf("Starting 'sendWithTimeout'\n");
	  #endif

	 // clearFlags();	// Initializing flags

	  // wait to TxDone flag
	  previous = millis();
	  if( _modem == LORA )
	  { // LoRa mode
		  clearFlags();	// Initializing flags
		  writeRegister(REG_OP_MODE, LORA_TX_MODE);  // LORA mode - Tx

		  value = readRegister(REG_IRQ_FLAGS);
		  // Wait until the packet is sent (TX Done flag) or the timeout expires
		  while ((bitRead(value, 3) == 0) && (millis() - previous < wait))
		  {
			  value = readRegister(REG_IRQ_FLAGS);
			  // Condition to avoid an overflow (DO NOT REMOVE)
			  if( millis() < previous )
			  {
				  previous = millis();
			  }
		  }
		  state = 1;
	  }
	  else
	  { // FSK mode
		  writeRegister(REG_OP_MODE, FSK_TX_MODE);  // FSK mode - Tx

		  value = readRegister(REG_IRQ_FLAGS2);
		  // Wait until the packet is sent (Packet Sent flag) or the timeout expires
		  while ((bitRead(value, 3) == 0) && (millis() - previous < wait))
		  {
			  value = readRegister(REG_IRQ_FLAGS2);
			  // Condition to avoid an overflow (DO NOT REMOVE)
			  if( millis() < previous )
			  {
				  previous = millis();
			  }
		  }
		  state = 1;
	  }
	  if( bitRead(value, 3) == 1 )
	  {
		  state = 0;	// Packet successfully sent
		  #if (SX1272_debug_mode > 1)
			  printf("## Packet successfully sent ##\n");
			  printf("\n");
		  #endif
	  }
	  else
	  {
		  if( state == 1 )
		  {
			  #if (SX1272_debug_mode > 1)
				  printf("** Timeout has expired **\n");
				  printf("\n");
			  #endif
		  }
		  else
		  {
			  #if (SX1272_debug_mode > 1)
				  printf("** There has been an error and packet has not been sent **\n");
				  printf("\n");
			  #endif
		  }
	  }

	  clearFlags();		// Initializing flags
	  return state;
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketMAXTimeout(uint8_t dest, char *payload)
{
	return sendPacketTimeout(dest, payload, MAX_TIMEOUT);
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketMAXTimeout(uint8_t dest,  uint8_t *payload, uint16_t length16)
{
	return sendPacketTimeout(dest, payload, length16, MAX_TIMEOUT);
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeout(uint8_t dest, char *payload)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeout'\n");
	#endif

	state = setPacket(dest, payload);	// Setting a packet with 'dest' destination
	if (state == 0)								// and writing it in FIFO.
	{
		state = sendWithTimeout();	// Sending the packet
	}
	return state;
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeout(uint8_t dest, uint8_t *payload, uint16_t length16)
{
	uint8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeout'\n");
	#endif

	state = truncPayload(length16);
	if( state == 0 )
	{
		state_f = setPacket(dest, payload);	// Setting a packet with 'dest' destination
	}												// and writing it in FIFO.
	else
	{
		state_f = state;
	}
	if( state_f == 0 )
	{
		state_f = sendWithTimeout();	// Sending the packet
	}
	return state_f;
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeout(uint8_t dest, char *payload, uint16_t wait)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeout'\n");
	#endif

	state = setPacket(dest, payload);	// Setting a packet with 'dest' destination
	if (state == 0)								// and writing it in FIFO.
	{
		state = sendWithTimeout(wait);	// Sending the packet
	}
	return state;
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeout(uint8_t dest, uint8_t *payload, uint16_t length16, uint16_t wait)
{
	uint8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeout'\n");
	#endif

	state = truncPayload(length16);
	if( state == 0 )
	{
		state_f = setPacket(dest, payload);	// Setting a packet with 'dest' destination
	}
	else
	{
		state_f = state;
	}
	if( state_f == 0 )								// and writing it in FIFO.
	{
		state_f = sendWithTimeout(wait);	// Sending the packet
	}
	return state_f;
}

/*
 Function: Configures the module to transmit information.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketMAXTimeoutACK(uint8_t dest, char *payload)
{
	return sendPacketTimeoutACK(dest, payload, MAX_TIMEOUT);
}

/*
 Function: Configures the module to transmit information and receive an ACK.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketMAXTimeoutACK(uint8_t dest, uint8_t *payload, uint16_t length16)
{
	return sendPacketTimeoutACK(dest, payload, length16, MAX_TIMEOUT);
}

/*
 Function: Configures the module to transmit information and receive an ACK.
 Returns: Integer that determines if there has been any error
   state = 3  --> Packet has been sent but ACK has not been received
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACK(uint8_t dest, char *payload)
{
	uint8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACK'\n");
	#endif

	state = sendPacketTimeout(dest, payload);	// Sending packet to 'dest' destination
	if( state == 0 )
	{
		state = receive();	// Setting Rx mode to wait an ACK
	}
	else
	{
		state_f = state;
	}
	if( state == 0 )
	{
		if( availableData() )
		{
			state_f = getACK();	// Getting ACK
		}
		else
		{
			state_f = 3;
		}
	}
	else
	{
		state_f = state;
	}

	return state_f;
}

/*
 Function: Configures the module to transmit information and receive an ACK.
 Returns: Integer that determines if there has been any error
   state = 3  --> Packet has been sent but ACK has not been received
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACK(uint8_t dest, uint8_t *payload, uint16_t length16)
{
	uint8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACK'\n");
	#endif

	// Sending packet to 'dest' destination
	state = sendPacketTimeout(dest, payload, length16);
	// Trying to receive the ACK
	if( state == 0 )
	{
		state = receive();	// Setting Rx mode to wait an ACK
	}
	else
	{
		state_f = state;
	}
	if( state == 0 )
	{
		if( availableData() )
		{
			state_f = getACK();	// Getting ACK
		}
		else
		{
			state_f = 3;
		}
	}
	else
	{
		state_f = state;
	}

	return state_f;
}

/*
 Function: Configures the module to transmit information and receive an ACK.
 Returns: Integer that determines if there has been any error
   state = 3  --> Packet has been sent but ACK has not been received
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACK(uint8_t dest, char *payload, uint16_t wait)
{
	uint8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACK'\n");
	#endif

	state = sendPacketTimeout(dest, payload, wait);	// Sending packet to 'dest' destination
	if( state == 0 )
	{
		state = receive();	// Setting Rx mode to wait an ACK
	}
	else
	{
		state_f = 1;
	}
	if( state == 0 )
	{
		if( availableData() )
		{
			state_f = getACK();	// Getting ACK
		}
		else
		{
			state_f = 3;
		}
	}
	else
	{
		state_f = 1;
	}

	return state_f;
}

/*
 Function: Configures the module to transmit information and receive an ACK.
 Returns: Integer that determines if there has been any error
   state = 3  --> Packet has been sent but ACK has not been received
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACK(uint8_t dest, uint8_t *payload, uint16_t length16, uint16_t wait)
{
	uint8_t state = 2;
	uint8_t state_f = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACK'\n");
	#endif

	state = sendPacketTimeout(dest, payload, length16, wait);	// Sending packet to 'dest' destination
	if( state == 0 )
	{
		state = receive();	// Setting Rx mode to wait an ACK
	}
	else
	{
		state_f = 1;
	}
	if( state == 0 )
	{
		if( availableData() )
		{
			state_f = getACK();	// Getting ACK
		}
		else
		{
			state_f = 3;
		}
	}
	else
	{
		state_f = 1;
	}

	return state_f;
}

/*
 Function: It gets and stores an ACK if it is received.
 Returns:
*/
uint8_t SX1272::getACK()
{
	return getACK(MAX_TIMEOUT);
}

/*
 Function: It gets and stores an ACK if it is received, before ending 'wait' time.
 Returns: Integer that determines if there has been any error
   state = 2  --> The ACK has not been received
   state = 1  --> The N-ACK has been received with no errors
   state = 0  --> The ACK has been received with no errors
 Parameters:
   wait: time to wait while there is no a valid header received.
*/
uint8_t SX1272::getACK(uint16_t wait)
{
	uint8_t state = 2;
	byte value = 0x00;
	unsigned long previous;
	boolean a_received = false;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getACK'\n");
	#endif

    previous = millis();

	if( _modem == LORA )
	{ // LoRa mode
	    value = readRegister(REG_IRQ_FLAGS);
		// Wait until the ACK is received (RxDone flag) or the timeout expires
		while ((bitRead(value, 6) == 0) && (millis() - previous < wait))
		{
			value = readRegister(REG_IRQ_FLAGS);
			if( millis() < previous )
			{
				previous = millis();
			}
		}
		if( bitRead(value, 6) == 1 )
		{ // ACK received
			a_received = true;
		}
		// Standby para minimizar el consumo
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Setting standby LoRa mode
	}
	else
	{ // FSK mode
		value = readRegister(REG_IRQ_FLAGS2);
		// Wait until the packet is received (RxDone flag) or the timeout expires
		while ((bitRead(value, 2) == 0) && (millis() - previous < wait))
		{
			value = readRegister(REG_IRQ_FLAGS2);
			if( millis() < previous )
			{
				previous = millis();
			}
		}
		if( bitRead(value, 2) == 1 )
		{ // ACK received
			a_received = true;
		}
		// Standby para minimizar el consumo
		writeRegister(REG_OP_MODE, FSK_STANDBY_MODE);	// Setting standby FSK mode
	}

	if( a_received )
	{
		// Storing the received ACK
		ACK.dst = _destination;
		ACK.src = readRegister(REG_FIFO);
		ACK.packnum = readRegister(REG_FIFO);
		ACK.length = readRegister(REG_FIFO);
		ACK.data[0] = readRegister(REG_FIFO);

		// Checking the received ACK
		if( ACK.dst == packet_sent.src )
		{
			if( ACK.src == packet_sent.dst )
			{
				if( ACK.packnum == packet_sent.packnum )
				{
					if( ACK.length == 0 )
					{
						if( ACK.data[0] == CORRECT_PACKET )
						{
							state = 0;
							#if (SX1272_debug_mode > 0)
								// Printing the received ACK
								printf("## ACK received:\n");
								printf("Destination: ");
								printf("%d\n", ACK.dst);			 	// Printing destination
								printf("Source: ");
								printf("%d\n", ACK.src);			 	// Printing source
								printf("ACK number: ");
								printf("%d\n", ACK.packnum);			// Printing ACK number
								printf("ACK length: ");
								printf("%d\n", ACK.length);				// Printing ACK length
								printf("ACK payload: ");
								printf("%d\n", ACK.data[0]);			// Printing ACK payload
								printf(" ##\n");
								printf("\n");
							#endif
						}
						else
						{
							state = 1;
							#if (SX1272_debug_mode > 0)
								printf("** N-ACK received **\n");
								printf("\n");
							#endif
						}
					}
					else
					{
						state = 1;
						#if (SX1272_debug_mode > 0)
							printf("** ACK length incorrectly received **\n");
							printf("\n");
						#endif
					}
				}
				else
				{
					state = 1;
					#if (SX1272_debug_mode > 0)
						printf("** ACK number incorrectly received **\n");
						printf("\n");
					#endif
				}
			}
			else
			{
				state = 1;
				#if (SX1272_debug_mode > 0)
					printf("** ACK source incorrectly received **\n");
					printf("\n");
				#endif
			}
		}
		else
		{
			state = 1;
			#if (SX1272_debug_mode > 0)
				printf("** ACK destination incorrectly received **\n");
				printf("\n");
			#endif
		}
	}
	else
	{
		state = 1;
		#if (SX1272_debug_mode > 0)
			printf("** ACK lost **\n");
			printf("\n");
		#endif
	}
	clearFlags();	// Initializing flags
	return state;
}

/*
 Function: Configures the module to transmit information with retries in case of error.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketMAXTimeoutACKRetries(uint8_t dest, char  *payload)
{
	return sendPacketTimeoutACKRetries(dest, payload, MAX_TIMEOUT);
}

/*
 Function: Configures the module to transmit information with retries in case of error.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketMAXTimeoutACKRetries(uint8_t dest, uint8_t *payload, uint16_t length16)
{
	return sendPacketTimeoutACKRetries(dest, payload, length16, MAX_TIMEOUT);
}

/*
 Function: Configures the module to transmit information with retries in case of error.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACKRetries(uint8_t dest, char *payload)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACKRetries'\n");
	#endif

	// Sending packet to 'dest' destination and waiting an ACK response.
	state = 1;
	while( (state != 0) && (_retries <= _maxRetries) )
	{
		state = sendPacketTimeoutACK(dest, payload);
		_retries++;
	}
	_retries = 0;

	return state;
}

/*
 Function: Configures the module to transmit information with retries in case of error.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACKRetries(uint8_t dest, uint8_t *payload, uint16_t length16)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACKRetries'\n");
	#endif

	// Sending packet to 'dest' destination and waiting an ACK response.
	state = 1;
	while((state != 0) && (_retries <= _maxRetries))
	{
		state = sendPacketTimeoutACK(dest, payload, length16);
		_retries++;

	}
	_retries = 0;

	return state;
}

/*
 Function: Configures the module to transmit information with retries in case of error.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACKRetries(uint8_t dest, char *payload, uint16_t wait)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACKRetries'\n");
	#endif

	// Sending packet to 'dest' destination and waiting an ACK response.
	state = 1;
	while((state != 0) && (_retries <= _maxRetries))
	{
		state = sendPacketTimeoutACK(dest, payload, wait);
		_retries++;
	}
	_retries = 0;

	return state;
}

/*
 Function: Configures the module to transmit information with retries in case of error.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::sendPacketTimeoutACKRetries(uint8_t dest, uint8_t *payload, uint16_t length16, uint16_t wait)
{
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'sendPacketTimeoutACKRetries'\n");
	#endif

	// Sending packet to 'dest' destination and waiting an ACK response.
	state = 1;
	while((state != 0) && (_retries <= _maxRetries))
	{
		state = sendPacketTimeoutACK(dest, payload, length16, wait);
		_retries++;
	}
	_retries = 0;

	return state;
}

/*
 Function: It gets the temperature from the measurement block module.
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
*/
uint8_t SX1272::getTemp()
{
	byte st0;
	uint8_t state = 2;

	#if (SX1272_debug_mode > 1)
		printf("\n");
		printf("Starting 'getTemp'\n");
	#endif

	st0 = readRegister(REG_OP_MODE);	// Save the previous status

	if( _modem == LORA )
	{ // Allowing access to FSK registers while in LoRa standby mode
		writeRegister(REG_OP_MODE, LORA_STANDBY_FSK_REGS_MODE);
	}

	state = 1;
	// Saving temperature value
	_temp = readRegister(REG_TEMP);
	if( _temp & 0x80 ) // The SNR sign bit is 1
	{
		// Invert and divide by 4
		_temp = ( ( ~_temp + 1 ) & 0xFF );
    }
    else
    {
		// Divide by 4
		_temp = ( _temp & 0xFF );
	}


	#if (SX1272_debug_mode > 1)
		printf("## Temperature is: ");
		printf("%d", _temp);
		printf(" ##\n");
		printf("\n");
	#endif

	if( _modem == LORA )
	{
		writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
	}

	state = 0;
	return state;
}


SX1272 sx1272 = SX1272();
