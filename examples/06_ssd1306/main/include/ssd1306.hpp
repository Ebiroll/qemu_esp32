#ifndef SSD1306_H
#define SSD1306_H

#include "fonts.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "esp_log.h"
#include "i2c.hpp"
#include <string>

//! @brief Drawing color
typedef enum {
	TRANSPARENT = -1, //!< Transparent (not drawing)
	BLACK = 0,        //!< Black (pixel off)
	WHITE = 1,        //!< White (or blue, yellow, pixel on)
	INVERT = 2,       //!< Invert pixel (XOR)
} ssd1306_color_t;

//! @brief Panel type
typedef enum {
	SSD1306_128x64 = 1,	//!< 128x32 panel
	SSD1306_128x32 = 2	//!< 128x64 panel
} ssd1306_panel_type_t;

class OLED {
private:
	I2C *i2c;
	ssd1306_panel_type_t type;
	uint8_t address;        // I2C address
	uint8_t *buffer;        // display buffer
	uint8_t width;          // panel width (128)
	uint8_t height;         // panel height (32 or 64)
	uint8_t refresh_top = 0;    // "Dirty" window
	uint8_t refresh_left = 0;
	uint8_t refresh_right = 0;
	uint8_t refresh_bottom = 0;
	const font_info_t* font = NULL;    // current font
	void command(uint8_t adress, uint8_t c);
	void data(uint8_t adress, uint8_t d);
public:
	/**
	 * @brief   Constructor
	 * @param   scl_pin  SCL Pin
	 * @param   sda_pin  SDA Pin
	 * @param   type     Panel type
	 */
	OLED(gpio_num_t scl, gpio_num_t sda, ssd1306_panel_type_t type);
	/**
	 * @brief   Constructor
	 * @param   scl_pin  SCL Pin
	 * @param   sda_pin  SDA Pin
	 * @param   type     Panel type
	 * @param   address  I2C Address(usually 0x78)
	 */
	OLED(gpio_num_t scl, gpio_num_t sda, ssd1306_panel_type_t type,
			uint8_t address);
	/**
	 * @brief   Initialize OLED panel
	 * @return  true if successful
	 * @remark  Possible reasons for failure include non-configured panel type, out of memory or I2C not responding
	 */
	bool init();
	/**
	 * @brief   De-initialize OLED panel, turn off power and free memory
	 * @return  true if successful
	 * @remark  Possible reasons for failure include non-configured panel type, out of memory or I2C not responding
	 */
	void term();
	/**
	 * @brief   Return OLED panel height
	 * @return  Panel height, or return 0 if failed (panel not initialized)
	 */
	uint8_t get_width();
	/**
	 * @brief   Return OLED panel height
	 * @return  Panel height, or return 0 if failed (panel not initialized)
	 */
	uint8_t get_height();
	/**
	 * @brief   Clear display buffer (fill with black)
	 */
	void clear();
	/**
	 * @brief   Refresh display (send display buffer to the panel)
	 * @param   force   The program automatically tracks "dirty" region to minimize refresh area. Set #force to true
	 *                  ignores the dirty region and refresh the whole screen.
	 */
	void refresh(bool force);
	/**
	 * @brief   Draw one pixel
	 * @param   x       X coordinate
	 * @param   y       Y coordinate
	 * @param   color   Color of the pixel
	 */
	void draw_pixel(int8_t x, int8_t y, ssd1306_color_t color);
	/**
	 * @brief   Draw horizontal line
	 * @param   x       X coordinate or starting (left) point
	 * @param   y       Y coordinate or starting (left) point
	 * @param   w       Line width
	 * @param   color   Color of the line
	 */
	void draw_hline(int8_t x, int8_t y, uint8_t w, ssd1306_color_t color);
	/**
	 * @brief   Draw vertical line
	 * @param   x       X coordinate or starting (top) point
	 * @param   y       Y coordinate or starting (top) point
	 * @param   h       Line height
	 * @param   color   Color of the line
	 */
	void draw_vline(int8_t x, int8_t y, uint8_t h, ssd1306_color_t color);/**
	 * @brief   Draw a rectangle
	 * @param   x       X coordinate or starting (top left) point
	 * @param   y       Y coordinate or starting (top left) point
	 * @param   w       Rectangle width
	 * @param   h       Rectangle height
	 * @param   color   Color of the rectangle border
	 */
	void draw_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h,
			ssd1306_color_t color);
	/**
	 * @brief   Draw a filled rectangle
	 * @param   x       X coordinate or starting (top left) point
	 * @param   y       Y coordinate or starting (top left) point
	 * @param   w       Rectangle width
	 * @param   h       Rectangle height
	 * @param   color   Color of the rectangle
	 */
	void fill_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h,
			ssd1306_color_t color);
	/**
	 * @brief   Draw a filled circle
	 * @param   x0      X coordinate or center
	 * @param   y0      Y coordinate or center
	 * @param   r       Radius
	 * @param   color   Color of the circle
	 */
	void draw_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color);
	/**
	 * @brief   Draw a filled circle
	 * @param   x0      X coordinate or center
	 * @param   y0      Y coordinate or center
	 * @param   r       Radius
	 * @param   color   Color of the circle
	 */
	void fill_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color);
	/**
	 * @brief   Select font for drawing
	 * @param   idx     Font index, see fonts.c
	 */
	void select_font(uint8_t idx);
	/**
	 * @brief   Draw one character using currently selected font
	 * @param   x           X position of character (top-left corner)
	 * @param   y           Y position of character (top-left corner)
	 * @param   c           The character to draw
	 * @param   foreground  Character color
	 * @param   background  Background color
	 * @return  Width of the character
	 */
	uint8_t draw_char(uint8_t x, uint8_t y, unsigned char c,
			ssd1306_color_t foreground, ssd1306_color_t background);
	/**
	 * @brief   Draw string using currently selected font
	 * @param   x           X position of string (top-left corner)
	 * @param   y           Y position of string (top-left corner)
	 * @param   str         The string to draw
	 * @param   foreground  Character color
	 * @param   background  Background color
	 * @return  Width of the string (out-of-display pixels also included)
	 */
	uint8_t draw_string(uint8_t x, uint8_t y, std::string str,
			ssd1306_color_t foreground, ssd1306_color_t background);
	/**
	 * @brief   Measure width of string with current selected font
	 * @param   str         String to measure
	 * @return  Width of the string
	 */
	uint8_t measure_string(std::string str);
	/**
	 * @brief   Get the height of current selected font
	 * @return  Height of the font (in pixels) or 0 if none font selected
	 */
	uint8_t get_font_height();
	/**
	 * @brief   Get the "C" value (space between adjacent characters)
	 * @return  "C" value
	 */
	uint8_t get_font_c();
	/**
	 * @brief   Set normal or inverted display
	 * @param   invert      Invert display?
	 */
	void invert_display(bool invert);/**
	 * @brief   Direct update display buffer
	 * @param   data        Data to fill display buffer, no length check is performed!
	 * @param   length      Length of data
	 */
	void update_buffer(uint8_t* data, uint16_t length);
};

#endif  /* SSD1306_H */

