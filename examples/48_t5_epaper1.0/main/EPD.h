/*
 * High level EPD functions
 * Author:  LoBo 06/2017, https://github/loboris
 * 
 */

#ifndef _EPD_H_
#define _EPD_H_

#include <stdlib.h>
#include "EPDspi.h"

typedef uint8_t color_t;

typedef struct {
	uint16_t        x1;
	uint16_t        y1;
	uint16_t        x2;
	uint16_t        y2;
} dispWin_t;

typedef struct {
	uint8_t 	*font;
	uint8_t 	x_size;
	uint8_t 	y_size;
	uint8_t	    offset;
	uint16_t	numchars;
    uint16_t	size;
	uint8_t 	max_x_size;
    uint8_t     bitmap;
	color_t     color;
} Font_t;



//==========================================================================================
// ==== Global variables ===================================================================
//==========================================================================================
uint8_t   orientation;		// current screen orientation
uint16_t  font_rotate;   	// current font font_rotate angle (0~395)
uint8_t   font_transparent;	// if not 0 draw fonts transparent
uint8_t   font_forceFixed;  // if not zero force drawing proportional fonts with fixed width
uint8_t   font_buffered_char;
uint8_t   font_line_space;	// additional spacing between text lines; added to font height
uint8_t   text_wrap;        // if not 0 wrap long text to the new line, else clip
color_t   _fg;            	// current foreground color for fonts
color_t   _bg;            	// current background for non transparent fonts
dispWin_t dispWin;			// display clip window
float	  _angleOffset;		// angle offset for arc, polygon and line by angle functions

Font_t cfont;					// Current font structure
uint8_t image_debug;

int	EPD_X;					// X position of the next character after EPD_print() function
int	EPD_Y;					// Y position of the next character after EPD_print() function
// =========================================================================================


// Buffer is created during jpeg decode for sending data
// Total size of the buffer is  2 * (JPG_IMAGE_LINE_BUF_SIZE * 3)
// The size must be multiple of 256 bytes !!
#define JPG_IMAGE_LINE_BUF_SIZE 512

// --- Constants for ellipse function ---
#define EPD_ELLIPSE_UPPER_RIGHT 0x01
#define EPD_ELLIPSE_UPPER_LEFT  0x02
#define EPD_ELLIPSE_LOWER_LEFT  0x04
#define EPD_ELLIPSE_LOWER_RIGHT 0x08

// Constants for Arc function
// number representing the maximum angle (e.g. if 100, then if you pass in start=0 and end=50, you get a half circle)
// this can be changed with setArcParams function at runtime
#define DEFAULT_ARC_ANGLE_MAX 360
// rotational offset in degrees defining position of value 0 (-90 will put it at the top of circle)
// this can be changed with setAngleOffset function at runtime
#define DEFAULT_ANGLE_OFFSET -90

#define PI 3.14159265359

#define MIN_POLIGON_SIDES	3
#define MAX_POLIGON_SIDES	60

// === Color names constants ===
#define EPD_BLACK 15
#define EPD_WHITE 0

// === Color invert constants ===
#define INVERT_ON		1
#define INVERT_OFF		0

// === Screen orientation constants ===
#define LANDSCAPE_0		1
#define LANDSCAPE_180	2

// === Special coordinates constants ===
#define CENTER	-9003
#define RIGHT	-9004
#define BOTTOM	-9004

#define LASTX	7000
#define LASTY	8000

// === Embedded fonts constants ===
#define DEFAULT_FONT	0
#define DEJAVU18_FONT	1
#define DEJAVU24_FONT	2
#define UBUNTU16_FONT	3
#define COMIC24_FONT	4
#define MINYA24_FONT	5
#define TOONEY32_FONT	6
#define SMALL_FONT		7
#define FONT_7SEG		8
#define USER_FONT		9  // font will be read from file



// ===== PUBLIC FUNCTIONS =========================================================================

/*
 * Draw pixel at given x,y coordinates
 * 
 * Params:
 *       x: horizontal position
 *       y: vertical position
 *   color: pixel color
*/
//------------------------------------------------------
void EPD_drawPixel(int16_t x, int16_t y, color_t color);

/*
 * Read pixel color value from display GRAM at given x,y coordinates
 * 
 * Params:
 *       x: horizontal position
 *       y: vertical position
 * 
 * Returns:
 *      pixel color at x,y
*/
//------------------------------------------
color_t EPD_readPixel(int16_t x, int16_t y);

/*
 * Draw vertical line at given x,y coordinates
 * 
 * Params:
 *       x: horizontal start position
 *       y: vertical start position
 *       h: line height in pixels
 *   color: line color
*/
//---------------------------------------------------------------------
void EPD_drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color);

/*
 * Draw horizontal line at given x,y coordinates
 * 
 * Params:
 *       x: horizontal start position
 *       y: vertical start position
 *       w: line width in pixels
 *   color: line color
*/
//---------------------------------------------------------------------
void EPD_drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color);

/*
 * Draw line on screen
 * 
 * Params:
 *       x0: horizontal start position
 *       y0: vertical start position
 *       x1: horizontal end position
 *       y1: vertical end position
 *   color: line color
*/
//-------------------------------------------------------------------------------
void EPD_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color);


/*
 * Draw line on screen from (x,y) point at given angle
 * Line drawing angle starts at lower right quadrant of the screen and is offseted by
 * '_angleOffset' global variable (default: -90 degrees)
 *
 * Params:
 *       x: horizontal start position
 *       y: vertical start position
 *   start: start offset from (x,y)
 *     len: length of the line
 *   angle: line angle in degrees
 *   color: line color
*/
//-----------------------------------------------------------------------------------------------------------
void EPD_drawLineByAngle(uint16_t x, uint16_t y, uint16_t start, uint16_t len, uint16_t angle, color_t color);

/*
 * Fill given rectangular screen region with color
 * 
 * Params:
 *       x: horizontal rect start position
 *       y: vertical rect start position
 *       w: rectangle width
 *       h: rectangle height
 *   color: fill color
*/
//---------------------------------------------------------------------------
void EPD_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color);

/*
 * Draw rectangle on screen
 * 
 * Params:
 *       x: horizontal rect start position
 *       y: vertical rect start position
 *       w: rectangle width
 *       h: rectangle height
 *   color: rect line color
*/
//------------------------------------------------------------------------------
void EPD_drawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, color_t color);

/*
 * Draw rectangle with rounded corners on screen
 * 
 * Params:
 *       x: horizontal rect start position
 *       y: vertical rect start position
 *       w: rectangle width
 *       h: rectangle height
 *       r: corner radius
 *   color: rectangle color
*/
//----------------------------------------------------------------------------------------------
void EPD_drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, color_t color);

/*
 * Fill given rectangular screen region with rounded corners with color
 * 
 * Params:
 *       x: horizontal rect start position
 *       y: vertical rect start position
 *       w: rectangle width
 *       h: rectangle height
 *       r: corner radius
 *   color: fill color
*/
//----------------------------------------------------------------------------------------------
void EPD_fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, color_t color);

/*
 * Fill the whole screen with color
 * 
 * Params:
 *   color: fill color
*/
//--------------------------------
void EPD_fillScreen(color_t color);

/*
 * Fill the current clip window with color
 *
 * Params:
 *   color: fill color
*/
//---------------------------------
void EPD_fillWindow(color_t color);

/*
 * Draw triangle on screen
 * 
 * Params:
 *       x0: first triangle point x position
 *       y0: first triangle point y position
 *       x0: second triangle point x position
 *       y0: second triangle point y position
 *       x0: third triangle point x position
 *       y0: third triangle point y position
 *   color: triangle color
*/
//-----------------------------------------------------------------------------------------------------------------
void EPD_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);

/*
 * Fill triangular screen region with color
 * 
 * Params:
 *       x0: first triangle point x position
 *       y0: first triangle point y position
 *       x0: second triangle point x position
 *       y0: second triangle point y position
 *       x0: third triangle point x position
 *       y0: third triangle point y position
 *   color: fill color
*/
//-----------------------------------------------------------------------------------------------------------------
void EPD_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);

/*
 * Draw circle on screen
 * 
 * Params:
 *       x: circle center x position
 *       y: circle center x position
 *       r: circle radius
 *   color: circle color
*/
//-------------------------------------------------------------------
void EPD_drawCircle(int16_t x, int16_t y, int radius, color_t color);

/*
 * Fill circle on screen with color
 * 
 * Params:
 *       x: circle center x position
 *       y: circle center x position
 *       r: circle radius
 *   color: circle fill color
*/
//-------------------------------------------------------------------
void EPD_fillCircle(int16_t x, int16_t y, int radius, color_t color);

/*
 * Draw ellipse on screen
 * 
 * Params:
 *       x0: ellipse center x position
 *       y0: ellipse center x position
 *       rx: ellipse horizontal radius
 *       ry: ellipse vertical radius
 *   option: drawing options, multiple options can be combined
                1 (TFT_ELLIPSE_UPPER_RIGHT) draw upper right corner
                2 (TFT_ELLIPSE_UPPER_LEFT)  draw upper left corner
                4 (TFT_ELLIPSE_LOWER_LEFT)  draw lower left corner
                8 (TFT_ELLIPSE_LOWER_RIGHT) draw lower right corner
             to draw the whole ellipse use option value 15 (1 | 2 | 4 | 8)
 * 
 *   color: circle color
*/
//------------------------------------------------------------------------------------------------------
void EPD_drawEllipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, color_t color, uint8_t option);

/*
 * Fill elliptical region on screen
 * 
 * Params:
 *       x0: ellipse center x position
 *       y0: ellipse center x position
 *       rx: ellipse horizontal radius
 *       ry: ellipse vertical radius
 *   option: drawing options, multiple options can be combined
                1 (TFT_ELLIPSE_UPPER_RIGHT) fill upper right corner
                2 (TFT_ELLIPSE_UPPER_LEFT)  fill upper left corner
                4 (TFT_ELLIPSE_LOWER_LEFT)  fill lower left corner
                8 (TFT_ELLIPSE_LOWER_RIGHT) fill lower right corner
             to fill the whole ellipse use option value 15 (1 | 2 | 4 | 8)
 * 
 *   color: fill color
*/
//------------------------------------------------------------------------------------------------------
void EPD_fillEllipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, color_t color, uint8_t option);


/*
 * Draw circle arc on screen
 * Arc drawing angle starts at lower right quadrant of the screen and is offseted by
 * '_angleOffset' global variable (default: -90 degrees)
 *
 * Params:
 *        cx: arc center X position
 *        cy: arc center Y position
 *        th: thickness of the drawn arc
 *        ry: arc vertical radius
 *     start: arc start angle in degrees
 *       end: arc end angle in degrees
 *     color: arc outline color
 * fillcolor: arc fill color
*/
//----------------------------------------------------------------------------------------------------------------------------
void EPD_drawArc(uint16_t cx, uint16_t cy, uint16_t r, uint16_t th, float start, float end, color_t color, color_t fillcolor);


/*
 * Draw polygon on screen
 *
 * Params:
 *        cx: polygon center X position
 *        cy: arc center Y position
 *     sides: number of polygon sides; MAX_POLIGON_SIDES ~ MAX_POLIGON_SIDES (3 ~ 60)
 *  diameter: diameter of the circle inside which the polygon is drawn
 *     color: polygon outline color
 *      fill: polygon fill color; if same as color, polygon is not filled
 *       deg: polygon rotation angle; 0 ~ 360
 *        th: thickness of the polygon outline
*/
//--------------------------------------------------------------------------------------------------------------
void EPD_drawPolygon(int cx, int cy, int sides, int diameter, color_t color, color_t fill, int deg, uint8_t th);


//--------------------------------------------------------------------------------------
//void EPD_drawStar(int cx, int cy, int diameter, color_t color, bool fill, float factor);


/*
 * Set the font used for writing the text to display.
 *
 * ------------------------------------------------------------------------------------
 * For 7 segment font only characters 0,1,2,3,4,5,6,7,8,9, . , - , : , / are available.
 *   Character ‘/‘ draws the degree sign.
 * ------------------------------------------------------------------------------------
 *
 * Params:
 *			 font: font number; use defined font names
 *		font_file: pointer to font file name; NULL for embeded fonts
 */
//----------------------------------------------------
void EPD_setFont(uint8_t font, const char *font_file);

/*
 * Returns current font height & width in pixels.
 *
 * Params:
 *		 width: pointer to returned font width
 *		height: pointer to returned font height
 */
//-------------------------------------------
int EPD_getfontsize(int *width, int* height);


/*
 * Returns current font height in pixels.
 *
 */
//----------------------
int EPD_getfontheight();

/*
 * Write text to display.
 *
 * Rotation of the displayed text depends on 'font_rotate' variable (0~360)
 * if 'font_transparent' variable is set to 1, no background pixels will be printed
 *
 * If the text does not fit the screen width it will be clipped (if text_wrap=0),
 * or continued on next line (if text_wrap=1)
 *
 * Two special characters are allowed in strings:
 * 		‘\r’ CR (0x0D), clears the display to EOL
 * 		‘\n’ LF (ox0A), continues to the new line, x=0
 *
 * Params:
 *	   st:	pointer to null terminated string to be printed
 *		x:	horizontal position of the upper left point in pixels
 *				Special values can be entered:
 *					CENTER, centers the text
 *					RIGHT, right justifies the text
 *					LASTX, continues from last X position; offset can be used: LASTX+n
 *		y: vertical position of the upper left point in pixels
 *				Special values can be entered:
 *					CENTER, centers the text
 *					BOTTOM, bottom justifies the text
 *					LASTY, continues from last Y position; offset can be used: LASTY+n
 *
 */
//-------------------------------------
void EPD_print(char *st, int x, int y);

/*
 * Set atributes for 7 segment vector font
 * == 7 segment font must be the current font to this function to have effect ==
 *
 * Params:
 *	   	   l:	6~40; distance between bars in pixels
 *	   	   w:	1~12, max l/2;  bar width in pixels
 *   outline:	draw font outline if set to 1
 *	   color:	font outline color, only used if outline=1
 *
 */
//-------------------------------------------------------------------------
void set_7seg_font_atrib(uint8_t l, uint8_t w, int outline, color_t color);

/*
 * Sets the clipping area coordinates.
 * All writing to screen is clipped to that area.
 * Starting x & y in all functions will be adjusted to the clipping area.
 *
 * Params:
 *		x1,y1:	upper left point of the clipping area
 *		x2,y2:	bottom right point of the clipping area
 *
 */
//----------------------------------------------------------------------
void EPD_setclipwin(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/*
 * Resets the clipping area to full screen (0,0),(_wodth,_height)
 *
 */
//----------------------
void EPD_resetclipwin();

/*
 * Save current clipping area to temporary variable
 *
 */
//---------------------
void EPD_saveClipWin();

/*
 * Restore current clipping area from temporary variable
 *
 */
//------------------------
void EPD_restoreClipWin();

/*
 * returns the string width in pixels.
 * Useful for positions strings on the screen.
 */
//--------------------------------
int EPD_getStringWidth(char* str);


/*
 * Fills the rectangle occupied by string with current background color
 */
void EPD_clearStringRect(int x, int y, char *str);


/*
 * Compile font c source file to .fnt file
 * which can be used in EPD_setFont() function to select external font
 * Created file have the same name as source file and extension .fnt
 *
 * Params:
 *		fontfile: pointer to c source font file name; must have .c extension
 *			 dbg: if set to 1, prints debug information
 *
 * Returns:
 * 		0 on success
 * 		err no on error
 *
 */
//------------------------------------------------
int compile_font_file(char *fontfile, uint8_t dbg);

/*
 * Get all font's characters to buffer
 */
void getFontCharacters(uint8_t *buf);

/*
 * Decodes and displays JPG image. RGB colors are converted to 4-bit Gray scale
 * Limits:
 * 		Baseline only. Progressive and Lossless JPEG format are not supported.
 *		Image size: Up to 65520 x 65520 pixels
 *		Color space: YCbCr three components only. Gray scale image is not supported.
 *		Sampling factor: 4:4:4, 4:2:2 or 4:2:0.
 *
 * Params:
 *       x: image left position; constants CENTER & RIGHT can be used; negative value is accepted
 *       y: image top position;  constants CENTER & BOTTOM can be used; negative value is accepted
 *   scale: image scale factor: 0~3; if scale>0, image is scaled by factor 1/(2^scale) (1/2, 1/4 or 1/8)
 *   fname: pointer to the name of the file from which the image will be read
 *   		if set to NULL, image will be read from memory buffer pointed to by 'buf'
 *     buf: pointer to the memory buffer from which the image will be read; used if fname=NULL
 *    size: size of the memory buffer from which the image will be read; used if fname=NULL & buf!=NULL
 *
 */
int EPD_jpg_image(int x, int y, uint8_t scale, char *fname, uint8_t *buf, int size);

#endif
