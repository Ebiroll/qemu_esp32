/*
 * 
 * HIGH SPEED LOW LEVEL DISPLAY FUNCTIONS USING DIRECT TRANSFER MODE
 *
*/

#ifndef _TFTFUNC_H_
#define _TFTFUNC_H_

#include "spi_master_nodma.h"

// Define ESP32 SPI pins to which the display is attached
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
// Reset and backlit pins are not used
//#define PIN_NUM_RST  18
//#define PIN_NUM_BCKL 5

// Display command/data pin
#define PIN_NUM_DC   26

// Touch screen CS pin
#define PIN_NUM_TCS  25

// Display constants
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160
#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320
#define XADOW_WIDTH  240
#define XADOW_HEIGHT 240

#define INITR_GREENTAB 0x0
#define INITR_REDTAB   0x1
#define INITR_BLACKTAB 0x2

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define TFT_INVOFF     0x20
#define TFT_INVONN     0x21
#define TFT_DISPOFF    0x28
#define TFT_DISPON     0x29
#define TFT_CASET      0x2A
#define TFT_PASET      0x2B
#define TFT_RAMWR      0x2C
#define TFT_RAMRD      0x2E
#define TFT_MADCTL	   0x36
#define TFT_PTLAR 	   0x30
#define TFT_ENTRYM 	   0xB7

#define ST7735_COLMOD  0x3A

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0D
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_GAMMASET 0x26

#define ILI9341_PIXFMT  0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define ILI9341_POWERA	0xCB
#define ILI9341_POWERB	0xCF
#define ILI9341_POWER_SEQ       0xED
#define ILI9341_DTCA	0xE8
#define ILI9341_DTCB	0xEA
#define ILI9341_PRC	0xF7
#define ILI9341_3GAMMA_EN	0xF2

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04


void disp_spi_transfer_start(spi_nodma_device_handle_t handle, int bits);
void disp_spi_transfer_cmd(spi_nodma_device_handle_t handle, int8_t cmd);
void disp_spi_transfer_addrwin(spi_nodma_device_handle_t handle, uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2);
void disp_spi_transfer_pixel(spi_nodma_device_handle_t handle, uint16_t color);
void disp_spi_set_pixel(spi_nodma_device_handle_t handle, uint16_t x, uint16_t y, uint16_t color);
void disp_spi_transfer_color_rep(spi_nodma_device_handle_t handle, uint8_t *color, uint32_t len, uint8_t rep);
void disp_spi_transfer_cmd_data(spi_nodma_device_handle_t handle, int8_t cmd, uint8_t *data, uint32_t len);
int disp_spi_read_data(spi_nodma_device_handle_t handle, int x1, int y1, int x2, int y2, int len, uint8_t *buf);

#endif