/*
 *  Author: LoBo (loboris@gmail.com, loboris.github)
 *
 *  Module supporting SPI ePaper displays
 *
 * HIGH SPEED LOW LEVEL DISPLAY FUNCTIONS
 * USING DIRECT or DMA SPI TRANSFER MODEs
 *
*/


#ifndef _EPDSPI_H_
#define _EPDSPI_H_

#include <stdint.h>
#include "spi_master_lobo.h"

#define EPD_DISPLAY_WIDTH	296
#define EPD_DISPLAY_HEIGHT	128

#define SCK_Pin		18
#define MOSI_Pin	23
#define MISO_Pin	
#define DC_Pin		17
#define BUSY_Pin	4
#define RST_Pin		16
#define CS_Pin		5
// ePaper display can be powered from GPIO
// if powered directly from Vcc, set this to 0
#define POWER_Pin	0

#define DC_VAL (1 << DC_Pin)

#define EPD_CS_0	gpio_set_level(CS_Pin, 0)
#define EPD_CS_1	gpio_set_level(CS_Pin, 1)
#define isEPD_CS	gpio_get_level(CS_Pin)

#define EPD_RST_0	gpio_set_level(RST_Pin, 0)
#define EPD_RST_1	gpio_set_level(RST_Pin, 1)
#define isEPD_RST	gpio_get_level(RST_Pin)

#define EPD_DC_0	gpio_set_level(DC_Pin, 0)
#define EPD_DC_1	gpio_set_level(DC_Pin, 1)

#define isEPD_BUSY  gpio_get_level(BUSY_Pin)

#define EPD_BUSY_LEVEL 0

// ==================================================
// Define which spi bus to use VSPI_HOST or HSPI_HOST
#define SPI_BUS L_VSPI_HOST
// ==================================================

spi_lobo_device_handle_t disp_spi;
uint8_t *gs_disp_buffer;
uint8_t *disp_buffer;
uint8_t *gs_drawBuff;
uint8_t *drawBuff;
int _width;
int _height;
uint16_t gs_used_shades;
uint8_t _gs;
uint8_t *LUT_part;
uint8_t LUTDefault_fastest[31];
uint8_t LUTDefault_part[31];
uint8_t LUT_gs[31];
uint8_t LUTDefault_full[31];
uint8_t lvl_buf[16];
uint8_t lvl_buf_jpg[16];

void EPD_wait(uint32_t ms);
void EPD_DisplaySetFull(uint8_t val);
void EPD_DisplaySetPart(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd, uint8_t val);
void EPD_DisplayClearFull();
void EPD_DisplayClearPart();
void EPD_DisplayFull(uint8_t *DisBuffer);
void EPD_DisplayPart(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd, uint8_t *DisBuffer);
void EPD_gsUpdate(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd, uint8_t gs);
void EPD_Update(int xStart, int xEnd, uint8_t yStart, uint8_t yEnd);
void EPD_UpdateScreen();
void EPD_Cls();
void EPD_PowerOn();
void EPD_PowerOff();


#endif
