/*
 *  Author: LoBo (loboris@gmail.com, loboris.github)
 *
 *  Module supporting SPI ePaper displays
 *
 * HIGH SPEED LOW LEVEL DISPLAY FUNCTIONS
 * USING DIRECT or DMA SPI TRANSFER MODEs
 *
*/


#include "spi_master_lobo.h"
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "soc/spi_reg.h"
#include "EPDspi.h"
#include "driver/gpio.h"

#define EPD_DEBUG 1

#define EPD2X9 1

	#define xDot 128
	#define yDot 296
	#define DELAYTIME 1500

static uint8_t GDOControl[] = {0x01, (yDot-1)%256, (yDot-1)/256, 0x00};
static uint8_t softstart[4] = {0x0c, 0xd7, 0xd6, 0x9d};
static uint8_t VCOMVol[2] = {0x2c, 0xa8};			// VCOM 7c
static uint8_t DummyLine[2] = {0x3a, 0x1a};			// 4 dummy line per gate
static uint8_t Gatetime[2] = {0x3b, 0x08};			// 2us per line
static uint8_t RamDataEntryMode[2] = {0x11, 0x01};	// Ram data entry mode
static uint8_t Border[2] = {0x3c, 0x61};			// Border control ( 0x61: white border; 0x51: black border

/*
There are totally 20 phases for programmable Source waveform of different phase length.
The phase period defined as TP [n] * T FRAME , where TP [n] range from 0 to 15.
TP [n] = 0 indicates phase skipped
Source Voltage Level: VS [n-XY] is constant in each phase
VS [n-XY] indicates the voltage in phase n for transition from GS X to GS Y
 00 – VSS
 01 – VSH
 10 – VSL
 11 – NA
VS [n-XY] and TP[n] are stored in waveform lookup table register [LUT].

VS coding: VS[0-11] VS[0-10] VS[0-01] VS[0-00]

*/
//                                   ---                                             VS                                             ----  ----           TP                            ----
//uint8_t LUTDefault_full[31] = {0x32, 0x02,0x02,0x01,0x11,0x12,0x12,0x22,0x22,0x66,0x69,0x69,0x59,0x58,0x99,0x99,0x88,0x00,0x00,0x00,0x00, 0xF8,0xB4,0x13,0x51,0x35,0x51,0x51,0x19,0x01,0x00};
uint8_t LUTDefault_full[31] = {0x32, 0x11,0x11,0x10,0x02,0x02,0x22,0x22,0x22,0x22,0x22,0x51,0x51,0x55,0x88,0x08,0x08,0x88,0x88,0x00,0x00, 0x34,0x23,0x12,0x21,0x24,0x28,0x22,0x21,0xA1,0x01};
uint8_t LUTDefault_part[31] = {0x32, 0x10,0x18,0x18,0x08,0x18,0x18,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x13,0x14,0x44,0x12,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t LUT_gs[31]			= {0x32, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t LUTFastest[31]		= {0x32, 0x99,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

uint8_t lvl_buf[16] = {32,70,110,150,185,210,220,225,230,235,240,243,248,251,253,255};
uint8_t lvl_buf_jpg[16] = {4,8,12,16,22,30,40,60,80,110,140,180,220,240,250,255};

uint8_t *LUT_part = LUTDefault_part;
spi_lobo_device_handle_t disp_spi = NULL;
uint8_t *gs_disp_buffer = NULL;
uint8_t *disp_buffer = NULL;
uint8_t *drawBuff = NULL;
uint8_t *gs_drawBuff = NULL;
int _width = EPD_DISPLAY_WIDTH;
int _height = EPD_DISPLAY_HEIGHT;
uint8_t _gs = 0;

uint16_t gs_used_shades = 0;

//-----------------------------------------------------------
static void IRAM_ATTR _dma_send(uint8_t *data, uint32_t size)
{
    //Fill DMA descriptors
    spi_lobo_dmaworkaround_transfer_active(disp_spi->host->dma_chan); //mark channel as active
    spi_lobo_setup_dma_desc_links(disp_spi->host->dmadesc_tx, size, data, false);
    disp_spi->host->hw->user.usr_mosi_highpart=0;
    disp_spi->host->hw->dma_out_link.addr=(int)(&disp_spi->host->dmadesc_tx[0]) & 0xFFFFF;
    disp_spi->host->hw->dma_out_link.start=1;
    disp_spi->host->hw->user.usr_mosi_highpart=0;

	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = (size * 8) - 1;

	// Start transfer
	disp_spi->host->hw->cmd.usr = 1;
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);

	//Tell common code DMA workaround that our DMA channel is idle. If needed, the code will do a DMA reset.
    if (disp_spi->host->dma_chan) spi_lobo_dmaworkaround_idle(disp_spi->host->dma_chan);

    // Reset DMA
	disp_spi->host->hw->dma_conf.val |= SPI_OUT_RST|SPI_IN_RST|SPI_AHBM_RST|SPI_AHBM_FIFO_RST;
	disp_spi->host->hw->dma_out_link.start=0;
	disp_spi->host->hw->dma_in_link.start=0;
	disp_spi->host->hw->dma_conf.val &= ~(SPI_OUT_RST|SPI_IN_RST|SPI_AHBM_RST|SPI_AHBM_FIFO_RST);
	disp_spi->host->hw->dma_conf.out_data_burst_en=1;
}

//--------------------------------------------------------------------------
static void IRAM_ATTR _direct_send(uint8_t *data, uint32_t len, uint8_t rep)
{
	uint32_t cidx = 0;	// buffer index
	uint32_t wd = 0;
	int idx = 0;
	int bits = 0;
	int wbits = 0;

    taskDISABLE_INTERRUPTS();

	while (len) {

		wd |= (uint32_t)data[idx] << wbits;
		wbits += 8;
		if (wbits == 32) {
			bits += wbits;
			wbits = 0;
			disp_spi->host->hw->data_buf[idx++] = wd;
			wd = 0;
		}
    	len--;					// Decrement data counter
        if (rep == 0) cidx++;	// if not repeating data, increment buffer index
    }
	if (bits) {
		while (disp_spi->host->hw->cmd.usr);						// Wait for SPI bus ready
		// Load send buffer
		disp_spi->host->hw->user.usr_mosi_highpart = 0;
		disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = bits-1;
		disp_spi->host->hw->user.usr_mosi = 1;
		disp_spi->host->hw->miso_dlen.usr_miso_dbitlen = 0;
		disp_spi->host->hw->user.usr_miso = 0;
        disp_spi->host->hw->cmd.usr = 1;							// Start transfer
	}
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);
    taskENABLE_INTERRUPTS();
}

// ================================================================
// === Main function to send data to display ======================
// If  rep==true:  repeat sending data to display 'len' times
// If rep==false:  send 'len' data bytes from buffer to display
// ** Device must already be selected and address window set **
// ================================================================
//---------------------------------------------------------------------------
static void IRAM_ATTR SPI_send_data(uint8_t *data, uint32_t len, uint8_t rep)
{
	if (len == 0) return;

	if ((len*8) <= 512) _direct_send(data, len, rep);
	else if (rep == 0)  _dma_send(data, len);
	else {
		// ==== Repeat data, more than 512 bits total ====
		uint8_t *transbuf = heap_caps_calloc(len,1, MALLOC_CAP_DMA);
		if (transbuf == NULL) return;

		memset(transbuf, data[0], len);
		_dma_send(transbuf, len);
		free(transbuf);
	}
}

// Send one byte to display
//-------------------------------------
void IRAM_ATTR SPI_Write(uint8_t value)
{
	disp_spi->host->hw->data_buf[0] = (uint32_t)value;
	// Load send buffer
	disp_spi->host->hw->user.usr_mosi_highpart = 0;
	disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
	disp_spi->host->hw->user.usr_mosi = 1;
	disp_spi->host->hw->miso_dlen.usr_miso_dbitlen = 0;
	disp_spi->host->hw->user.usr_miso = 0;
	// Start transfer
	disp_spi->host->hw->cmd.usr = 1;
	// Wait for SPI bus ready
	while (disp_spi->host->hw->cmd.usr);
}

// Check display busy line and wait while busy
//-----------------------
static uint8_t ReadBusy()
{
	for (int i=0; i<400; i++){
		if (isEPD_BUSY == EPD_BUSY_LEVEL) return 1;
		vTaskDelay(10 / portTICK_RATE_MS);
	}
	return 0;
}

//-----------------------
static uint8_t WaitBusy()
{
	if (isEPD_BUSY != EPD_BUSY_LEVEL) return 1;
	vTaskDelay(10 / portTICK_RATE_MS);
	if (isEPD_BUSY != EPD_BUSY_LEVEL) return 1;
	return 0;
}

// Write one command without parameters
//---------------------------------------
static void EPD_WriteCMD(uint8_t command)
{
	spi_lobo_device_select(disp_spi, 0);
	EPD_DC_0;		// command write
	SPI_Write(command);
}

// Write command with one paramet
//---------------------------------------
static void EPD_WriteCMD_p1(uint8_t command,uint8_t para)
{
	spi_lobo_device_select(disp_spi, 0);
	//ReadBusy();
	EPD_DC_0;		// command write
	SPI_Write(command);
	EPD_DC_1;		// data write
	SPI_Write(para);
	spi_lobo_device_deselect(disp_spi);
}

//----------------
void EPD_PowerOn()
{
	EPD_WriteCMD_p1(0x22,0xc0);
	EPD_WriteCMD(0x20);
	//EPD_WriteCMD(0xff);
	spi_lobo_device_deselect(disp_spi);
#if EPD_DEBUG
	if (!WaitBusy()) printf("[EPD] NOT BUSY\r\n");
	if (!ReadBusy()) printf("[EPD] NOT READY\r\n");
#else
	WaitBusy();
	ReadBusy();
#endif
}

//-----------------
void EPD_PowerOff()
{
	EPD_WriteCMD_p1(0x22,0x03);
	EPD_WriteCMD(0x20);
	//EPD_WriteCMD(0xff);
	spi_lobo_device_deselect(disp_spi);
#if EPD_DEBUG
	if (!WaitBusy()) printf("[EPD] NOT BUSY\r\n");
	if (!ReadBusy()) printf("[EPD] NOT READY\r\n");
#else
	WaitBusy();
	ReadBusy();
#endif
#if POWER_Pin
	gpio_set_level(DC_Pin, 0);
	gpio_set_level(MOSI_Pin, 0);
	gpio_set_level(SCK_Pin, 0);
	gpio_set_level(RST_Pin, 0);
	gpio_set_level(CS_Pin, 0);
	gpio_set_level(POWER_Pin, 0);
#endif
}

// Send command with multiple parameters
//----------------------------------------------------
static void EPD_Write(uint8_t *value, uint8_t datalen)
{
	uint8_t i = 0;
	uint8_t *ptemp;

	ptemp = value;
	spi_lobo_device_select(disp_spi, 0);
	//ReadBusy();
	EPD_DC_0;			// When DC is 0, write command
	SPI_Write(*ptemp);	//The first byte is written with the command value
	ptemp++;
	EPD_DC_1;			// When DC is 1, write data
	for(i= 0;i<datalen-1;i++){	// sub the data
		SPI_Write(*ptemp);
		ptemp++;
	}
	spi_lobo_device_deselect(disp_spi);
}

// Send data buffer to display
//----------------------------------------------------------------------------
static void EPD_WriteDispRam(uint8_t XSize, uint16_t YSize,	uint8_t *Dispbuff)
{
	if (XSize%8 != 0) XSize = XSize+(8-XSize%8);
	XSize = XSize/8;

	spi_lobo_device_select(disp_spi, 0);
	//ReadBusy();
	EPD_DC_0;		//command write
	SPI_Write(0x24);
	EPD_DC_1;		//data write
	SPI_send_data(Dispbuff, XSize*YSize, 0);
	spi_lobo_device_deselect(disp_spi);
}

// Fill the display with value
//-------------------------------------------------------------------------------
static void EPD_WriteDispRamMono(uint8_t XSize, uint16_t YSize,	uint8_t dispdata)
{
	if (XSize%8 != 0) XSize = XSize+(8-XSize%8);
	XSize = XSize/8;

	spi_lobo_device_select(disp_spi, 0);
	//ReadBusy();
	EPD_DC_0;		// command write
	SPI_Write(0x24);
	EPD_DC_1;		// data write
	SPI_send_data(&dispdata, XSize*YSize, 1);
	spi_lobo_device_deselect(disp_spi);
}

/*
  === Set RAM X - Address Start / End Position (44h) ===
  Specify the start/end positions of the window address in the X direction by 8 times address unit.
  Data is written to the RAM within the area determined by the addresses specified by XSA [4:0] and XEA [4:0].
  These addresses must be set before the RAM write. It allows on XEA [4:0] ≤ XSA [4:0].
  The settings follow the condition on 00h ≤ XSA [4:0], XEA [4:0] ≤ 1Dh.
  The windows is followed by the control setting of Data Entry Setting (R11h)

  === Set RAM Y - Address Start / End Position (45h) ===
  Specify the start/end positions of the window address in the Y direction by an address unit.
  Data is written to the RAM within the area determined by the addresses specified by YSA [8:0] and YEA [8:0].
  These addresses must be set before the RAM write.
  It allows YEA [8:0] ≤ YSA [8:0].
  The settings follow the condition on 00h ≤ YSA [8:0], YEA [8:0] ≤ 13Fh.
  The windows is followed by the control setting of Data Entry Setting (R11h)
 */
//--------------------------------------------------------------------------------------
static void EPD_SetRamArea(uint8_t Xstart, uint8_t Xend, uint16_t Ystart, uint16_t Yend)
{
    uint8_t RamAreaX[3];	// X start and end
	uint8_t RamAreaY[5]; 	// Y start and end
	RamAreaX[0] = 0x44;	// command
	RamAreaX[1] = Xstart;
	RamAreaX[2] = Xend;
	RamAreaY[0] = 0x45;	// command
	RamAreaY[1] = Ystart & 0xFF;
	RamAreaY[2] = Ystart >> 8;
	RamAreaY[3] = Yend & 0xFF;
    RamAreaY[4] = Yend >> 8;
	EPD_Write(RamAreaX, sizeof(RamAreaX));
	EPD_Write(RamAreaY, sizeof(RamAreaY));
}

//Set RAM X and Y address counter
/*
  === Set RAM Address Counter (4Eh-4Fh) ===
  adrX[4:0]: Make initial settings for the RAM X address in the address counter (AC).
  adrY[8:0]: Make initial settings for the RAM Y address in the address counter (AC).
  After RAM data is written, the address counter is automatically updated according to the settings with AM, ID
   bits and setting for a new RAM address is not required in the address counter.
  Therefore, data is written consecutively without setting an address.
  The address counter is not automatically updated when data is read out from the RAM.
  RAM address setting cannot be made during the standby mode.
  The address setting should be made within the area designated with window addresses which is controlled
   by the Data Entry Setting (R11h) {AM, ID[1:0]} ; RAM Address XStart / XEnd Position (R44h) and RAM Address Ystart /Yend Position (R45h).
   Otherwise undesirable image will be displayed on the Panel.
*/
//----------------------------------------------------------
static void EPD_SetRamPointer(uint8_t addrX, uint16_t addrY)
{
    uint8_t RamPointerX[2];	// default (0,0)
	uint8_t RamPointerY[3];
	//Set RAM X address counter
	RamPointerX[0] = 0x4e;
	RamPointerX[1] = addrX;
	//Set RAM Y address counter
	RamPointerY[0] = 0x4f;
	RamPointerY[1] = addrY & 0xFF;
	RamPointerY[2] = addrY >> 8;

	EPD_Write(RamPointerX, sizeof(RamPointerX));
	EPD_Write(RamPointerY, sizeof(RamPointerY));
}


//Set RAM X and Y address Start / End position
//Set RAM X and Y address counter
//----------------------------------------------------------------------------------------------
static void part_display(uint8_t RAM_XST, uint8_t RAM_XEND ,uint16_t RAM_YST, uint16_t RAM_YEND)
{
	EPD_SetRamArea(RAM_XST, RAM_XEND, RAM_YST, RAM_YEND);
    EPD_SetRamPointer (RAM_XST, RAM_YST);
}

//Initialize the display
//--------------------
static void EPD_Init()
{
#if POWER_Pin
	gpio_set_level(POWER_Pin, 1);
	vTaskDelay(100 / portTICK_RATE_MS);
#else
	vTaskDelay(10 / portTICK_RATE_MS);
#endif
	// reset
	EPD_RST_0;
	vTaskDelay(10 / portTICK_RATE_MS);
#if EPD_DEBUG
	uint32_t t1 = clock();
#endif
	EPD_RST_1;
	for (int n=0; n<50; n++) {
		vTaskDelay(10 / portTICK_RATE_MS);
		if (isEPD_BUSY == EPD_BUSY_LEVEL) break;
	}

	SPI_Write(0x12); // software reset
	vTaskDelay(10 / portTICK_RATE_MS);
	ReadBusy();

	// set registers
	EPD_Write(GDOControl, sizeof(GDOControl));	// Panel configuration, Gate selection
	EPD_Write(softstart, sizeof(softstart));	// X decrease, Y decrease
	EPD_Write(VCOMVol, sizeof(VCOMVol));		// VCOM setting
	EPD_Write(DummyLine, sizeof(DummyLine));	// dummy line per gate
	EPD_Write(Gatetime, sizeof(Gatetime));		// Gate time setting
	EPD_Write(Border, sizeof(Border));
	EPD_Write(RamDataEntryMode, sizeof(RamDataEntryMode));	// X increase, Y decrease

	EPD_SetRamArea(0x00, (xDot-1)/8, yDot-1, 0);
	EPD_SetRamPointer(0x00, yDot-1);
#if EPD_DEBUG
	t1 = clock() - t1;
	printf("[EPD] Init: %u ms\r\n", t1);
#endif
}

//------------------------------
static void EPD_UpdateFull(void)
{
	/*
	+ Enable Clock Signal,
	+ Then Enable CP
	- Then Load Temperature value
	- Then Load LUT
	- Then INITIAL DISPLAY
	+ Then PATTERN DISPLAY
	+ Then Disable CP
	+ Then Disable OSC
	*/
	EPD_WriteCMD_p1(0x22,0xC7);
	EPD_WriteCMD(0x20);
	//EPD_WriteCMD(0xff);
	spi_lobo_device_deselect(disp_spi);

#if EPD_DEBUG
	if (!WaitBusy()) printf("[EPD] NOT BUSY\r\n");
	if (!ReadBusy()) printf("[EPD] NOT READY\r\n");
#else
	WaitBusy();
	ReadBusy();
#endif
}

//-------------------------------
static void EPD_Update_Part(void)
{
	/*
	- Enable Clock Signal,
	- Then Enable CP
	- Then Load Temperature value
	- Then Load LUT
	- Then INITIAL DISPLAY
	+ Then PATTERN DISPLAY
	- Then Disable CP
	- Then Disable OSC
	*/
	EPD_WriteCMD_p1(0x22,0x04);
	EPD_WriteCMD(0x20);
	//EPD_WriteCMD(0xff);
	spi_lobo_device_deselect(disp_spi);

#if EPD_DEBUG
	if (!WaitBusy()) printf("[EPD] NOT BUSY\r\n");
	if (!ReadBusy()) printf("[EPD] NOT READY\r\n");
#else
	WaitBusy();
	ReadBusy();
#endif
}

/*******************************************************************************
Full screen initialization
********************************************************************************/
static void EPD_init_Full(void)
{
	EPD_Init();			// Reset and set register
	EPD_Write((uint8_t *)LUTDefault_full,sizeof(LUTDefault_full));

	EPD_PowerOn();
}

/*******************************************************************************
Part screen initialization
********************************************************************************/
static void EPD_init_Part(void)
{
	EPD_Init();			// display
	EPD_Write((uint8_t *)LUT_part, 31);
	EPD_PowerOn();
}
/********************************************************************************
parameter:
	Label  :
       		=1 Displays the contents of the DisBuffer
	   		=0 Displays the contents of the first byte in DisBuffer,
********************************************************************************/
static void EPD_Dis_Full(uint8_t *DisBuffer,uint8_t type)
{
    EPD_SetRamPointer(0x00, yDot-1);	// set ram pointer
	if (type == 0){
		// Fill screen with white
		EPD_WriteDispRamMono(xDot, yDot, 0xff);
	}
	else {
		// Fill screen from buffer
		EPD_WriteDispRam(xDot, yDot, (uint8_t *)DisBuffer);
	}
	EPD_UpdateFull();

}

/********************************************************************************
WARNING: X is smaller screen dimension (0~127) !
         Y is larger screen dimension (0~295) !
parameter:
		xStart :   X direction Start coordinate
		xEnd   :   X direction end coordinate
		yStart :   Y direction Start coordinate
		yEnd   :   Y direction end coordinate
		DisBuffer : Display content
		type  :
       		=1 Displays the contents of the DisBuffer
	   		=0 Displays the contents of the first byte in DisBuffer,
********************************************************************************/
static void EPD_Dis_Part(uint8_t xStart, uint8_t xEnd, uint16_t yStart, uint16_t yEnd, uint8_t *DisBuffer, uint8_t type)
{
	if (type == 0) {
		// Repeated color
		part_display(xStart/8, xEnd/8, yEnd, yStart);
		EPD_WriteDispRamMono(xEnd-xStart+1, yEnd-yStart+1, DisBuffer[0]);
 		EPD_Update_Part();
		part_display(xStart/8, xEnd/8, yEnd, yStart);
		EPD_WriteDispRamMono(xEnd-xStart+1, yEnd-yStart+1, DisBuffer[0]);
	}
	else {
		// From buffer
		part_display(xStart/8, xEnd/8, yEnd, yStart);
		EPD_WriteDispRam(xEnd-xStart+1, yEnd-yStart+1,DisBuffer);
		EPD_Update_Part();
		part_display(xStart/8, xEnd/8, yEnd, yStart);
		EPD_WriteDispRam(xEnd-xStart+1, yEnd-yStart+1,DisBuffer);
	}
}

//======================================================================================================================================

// Clear full screen
//=========================
void EPD_DisplayClearFull()
{
	uint8_t m;
	EPD_init_Full();

#if EPD_DEBUG
	uint32_t t1 = clock();
#endif
	m = 0x00;
	EPD_Dis_Full(&m, 0);  //all black
#if EPD_DEBUG
	t1 = clock() - t1;
	printf("[EPD] Clear black: %u ms\r\n", t1);
	t1 = clock();
#endif
	m = 0xff;
	EPD_Dis_Full(&m, 0);  //all white
#if EPD_DEBUG
	t1 = clock() - t1;
	printf("[EPD] Clear white: %u ms\r\n", t1);
#endif
}

// Partial clear screen
//=========================
void EPD_DisplayClearPart()
{
	uint8_t m = 0xFF;
	EPD_init_Part();
#if EPD_DEBUG
	uint32_t t1 = clock();
	EPD_Dis_Part(0, xDot-1, 0, yDot-1, &m, 0);	//all white
	m = 0x00;
	EPD_Dis_Part(0, xDot-1, 0, yDot-1, &m, 0);	//all black
	m = 0xFF;
	EPD_Dis_Part(0, xDot-1, 0, yDot-1, &m, 0);	//all white
	t1 = clock() - t1;
	printf("[EPD] Part Clear: %u ms\r\n", t1);
#else
	EPD_Dis_Part(0, xDot-1, 0, yDot-1, &m, 0);	//all white
	m = 0x00;
	EPD_Dis_Part(0, xDot-1, 0, yDot-1, &m, 0);	//all black
	m = 0xFF;
	EPD_Dis_Part(0, xDot-1, 0, yDot-1, &m, 0);	//all white
#endif
}

//==================================
void EPD_DisplaySetFull(uint8_t val)
{
	EPD_Write((uint8_t *)LUTDefault_full,sizeof(LUTDefault_full));
#if EPD_DEBUG
	uint32_t t1 = clock();
	EPD_Dis_Full(&val, 0);
	t1 = clock() - t1;
	printf("[EPD] Display Set Full: %u ms [%02x]\r\n", t1, val);
#else
	EPD_Dis_Full(&val, 0);
#endif
}

//======================================================================================
void EPD_DisplaySetPart(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd, uint8_t val)
{
	EPD_Write((uint8_t *)LUT_part, 31);
#if EPD_DEBUG
	uint32_t t1 = clock();
	EPD_Dis_Part(yStart,yEnd,xStart,xEnd, &val,0);
	t1 = clock() - t1;
	printf("[EPD] Display Set Part: %u ms [%02x]\r\n", t1, val);
#else
	EPD_Dis_Part(yStart,yEnd,xStart,xEnd, &val,0);
#endif
}

//======================================
void EPD_DisplayFull(uint8_t *DisBuffer)
{
	EPD_Write((uint8_t *)LUTDefault_full,sizeof(LUTDefault_full));
#if EPD_DEBUG
	uint32_t t1 = clock();
	EPD_Dis_Full((uint8_t *)DisBuffer,1);
	t1 = clock() - t1;
	printf("[EPD] Display Full: %u ms\r\n", t1);
#else
	EPD_Dis_Full((uint8_t *)DisBuffer,1);
#endif
}

//==========================================================================================
void EPD_DisplayPart(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd, uint8_t *DisBuffer)
{
	EPD_Write((uint8_t *)LUT_part, 31);
#if EPD_DEBUG
	uint32_t t1 = clock();
	EPD_Dis_Part(yStart,yEnd,xStart,xEnd,(uint8_t *)DisBuffer,1);
	t1 = clock() - t1;
	printf("[EPD] Display Part: %u ms [%02x:%02x]\r\n", t1, LUT_gs[1], LUT_gs[21]);
#else
	EPD_Dis_Part(yStart,yEnd,xStart,xEnd,(uint8_t *)DisBuffer,1);
#endif
}

//============
void EPD_Cls()
{
	EPD_DisplaySetPart(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1, 0xFF);
	memset(disp_buffer, 0xFF, _width * (_height/8));
	memset(gs_disp_buffer, 0, _width * _height);
	gs_used_shades = 0;
}

//-------------------------------------------------------------------------------
void EPD_gsUpdate(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd, uint8_t gs)
{
	uint8_t val, buf_val, new_val;
	int count=0, changed=0;
	int x;
	uint8_t y;
	for (x=xStart; x<=xEnd; x++) {
		for (y=yStart; y<=yEnd; y++) {
			val = gs_drawBuff[(y * (xEnd-xStart+1)) + x];
			if (val > 15) val >>= 4;
			if (val == gs) {
				buf_val = drawBuff[(x * ((yEnd-yStart+1)>>3)) + (y>>3)];
				new_val = buf_val;
				if (gs > 0) new_val &= (0x80 >> (y % 8)) ^ 0xFF;
				else new_val |= (0x80 >> (y % 8));
				if (new_val != buf_val) {
					drawBuff[(x * (_height>>3)) + (y>>3)] = new_val;
					changed++;
				}
				count++;
			}
		}
	}

	if (changed) {
		#if EPD_DEBUG
		printf("[EPD] GS Update %02x, count=%d changed=%d\r\n", gs, count, changed);
		#endif
		uint8_t *_lutPart = LUT_part;
		memset(LUT_gs+1, 0, 30);
		if (gs > 0) {
			if (gs > 0) {
				LUT_gs[1] = 0x18;
				LUT_gs[21] = gs;
			}
		}
		else {
			LUT_gs[1] = 0x28;
			LUT_gs[2] = 0x00;
			LUT_gs[21] = 15;
		}
		LUT_part = LUT_gs;
		EPD_DisplayPart(xStart, xEnd, yStart, yEnd, drawBuff);

		LUT_part = _lutPart;
	}
}

//-----------------------------------------------------------------------
void EPD_Update(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd)
{
	if (_gs == 0) EPD_DisplayPart(xStart, xEnd, yStart, yEnd, drawBuff);
	else {
		for (int n=0; n<16; n++) {
			if (gs_used_shades & (1<<n)) EPD_gsUpdate(xStart, xEnd, yStart, yEnd, n);
		}
	}
}

//-------------------
void EPD_UpdateScreen()
{
	EPD_Update(0, EPD_DISPLAY_WIDTH-1, 0, EPD_DISPLAY_HEIGHT-1);
}

//------------------------
void EPD_wait(uint32_t ms)
{
	if (ms < 100) ms = 100;
	uint32_t n = 0;
	while (n < ms) {
		vTaskDelay(100 / portTICK_RATE_MS);
		n += 100;
	}
}


