#ifndef __SSD1306_H__
#define __SSD1306_H__

#define FONT_START          ' '  /* First character value in the font table */

/** SSD1306 Controller Driver
  *
  * This class provides a buffered display for the SSD1306 OLED controller.
  * 
  * TODO: 
  *   - At the moment, the driver assumes a 128x64 pixel display.
  *   - Only fonts of 8 pixel height are supported (different widths can be used).
  *   - Pretty much no drawing functions are provided as yet.
  *   - Possible "auto-update", automatically calling update() after a printf etc.
  *
  * Information taken from the datasheet at:
  *   http://www.adafruit.com/datasheets/SSD1306.pdf
  *
  */
class SSD1306
{
public:
    /** Construct a new SSD1306 object.
     *  @param cs The connected C/S pin.
     *  @param rs The connected RS pin.
     *  @param dc The connected DC pin.
     *  @param clk The connected CLK pin.
     *  @param data The connected Data pin.
     */
    SSD1306(PinName cs, PinName rs, PinName dc, PinName clk, PinName data);

    // ----- HARDWARE CONTROL -----

    /** Initialise the display with defaults.*/
    void initialise();
    
    /** Force a refresh of the display.  Copies the buffer to the controller. */
    void update();

    /** Turn the whole display off.  This will reset all configuration settings on the controller to their defaults. */
    void off();
    
    /** Turn the whole display on.  Used during initialisation. */
    void on();

    /** Sends the display to sleep, but leaves RAM intact. */
    void sleep();

    /** Wakes up this display following a sleep() call.
     *  @see sleep()
     */
    void wake();

    /** Set the display contrast.
     *  @param value The contrast, from 1 to 256.
     */
    void set_contrast(unsigned char value); // 1-256
    
    /** Set the display to normal or inverse.
     *  @param value 0 for normal mode, or 1 for inverse mode.
     */
    void set_inverse(unsigned char value); // 0 or 1
    
    /** Set the display start line.  This is the line at which the display will start rendering.
     *  @param value A value from 0 to 63 denoting the line to start at.
     */
    void set_display_start_line(unsigned char value); // 0-63
    
    /** Set the segment remap state.  This allows the module to be addressed as if flipped horizontally.
      * NOTE: Changing this setting has no effect on data already in the module's GDDRAM.
      * @param value 0 = column address 0 = segment 0 (the default), 1 = column address 127 = segment 0 (flipped).
      */
    void set_segment_remap(unsigned char value); // 0 or 1
    
    /** Set the vertical shift by COM.
      * @param value The number of rows to shift, from 0 - 63.
      */
    void set_display_offset(unsigned char value); // 0-63
    
    /** Set the multiplex ratio.
     *  @param value MUX will be set to (value+1). Valid values range from 15 to 63 - MUX 16 to 64.
     */
    void set_multiplex_ratio(unsigned char value); // 15-63 (value+1 mux)
    
    /** Set COM output scan direction.  If the display is active, this will immediately vertically
      * flip the display.
      * @param value 0 = Scan from COM0 (default), 1 = reversed (scan from COM[N-1]).
      */
    void set_com_output_scan_direction(unsigned char value); // 0 or 1
    
    /** Set COM pins hardware configuration.
      * @param sequential 0 = Sequental COM pin configuration, 1 = Alternative COM pin configuration (default).
      * @param lr_remap 0 = Disable COM left/right remap (default), 1 = enable COM left/right remap.
      */
    void set_com_pins_hardware_configuration(unsigned char sequential, unsigned char lr_remap); // 0 or 1 for both parametrs

    /** Set up and start a continuous horizontal scroll.
      * Once you have set up the scrolling, you can deactivate it with stop_scroll().
      * @param direction 0 for right, 1 for left.
      * @param start Start page address, 0 - 5.
      * @param end End page address, 0 - 5.
      * @param interval Interval in frame frequency.  Valid values are: 2, 3, 4, 5, 25, 64, 128, 256.
      * @see stop_scrol
      */
    void start_horizontal_scroll(unsigned char direction, unsigned char start, unsigned char end, unsigned char interval);

    /** Set up and start a continuous horizontal and vertical scroll.
      * NOTE: No continuous vertical scroll is available.
      * Once you have set up the scrolling, you can deactivate it with stop_scroll().
      * @param direction 0 for vertical and right horizontal scroll, 1 for vertical and left horizontal scroll.
      * @param start Start page address, 0 - 5.
      * @param end End page address, 0 - 5.
      * @param interval Interval in frame frequency.  Valid values are: 2, 3, 4, 5, 25, 64, 128, 256.
      * @param vertical_offset Offset of vertical scroll, 1 - 63.
      * @see stop_scroll
      */
    void start_vertical_and_horizontal_scroll(unsigned char direction, unsigned char start, unsigned char end, unsigned char interval, unsigned char vertical_offset);
    
    /** Deactivate the continuous scroll set up with start_horizontal_scroll() or 
      * start_vertical_and_horizontal_scroll().
      * @see set_horizontal_scroll, set_vertical_and_horizontal_scroll
      */
    void stop_scroll();
    
    // ----- ADDRESSING -----
    
    /** Set memory addressing mode to the given value.
      * @param mode 0 for Horizontal addressing mode, 1 for Vertical addressing mode, or 2 for Page addressing mode (PAM).  2 is the default.
      */
    void set_memory_addressing_mode(unsigned char mode);
    
    /** Page Addressing Mode: Set the column start address register for
      * page addressing mode.
      * @param address The address (full byte).
      */
    void pam_set_start_address(unsigned char address);       
    
    /** Set the GDDRAM page start address for page addressing mode.
      * @param address The start page, 0 - 7.
      */
    void pam_set_page_start(unsigned char address);
    
    /** Set page start and end address for horizontal/vertical addressing mode.
      * @param start The start page, 0 - 7.
      * @param end The end page, 0 - 7.
      */
    void hv_set_page_address(unsigned char start, unsigned char end);
    
    /** Set column address range for horizontal/vertical addressing mode.
      * @param start Column start address, 0 - 127.
      * @param end Column end address, 0 - 127.
      */
    void hv_set_column_address(unsigned char start, unsigned char end);
    
    // ----- TIMING & DRIVING -----
    /** Set the display clock divide ratio and the oscillator frequency.
      * @param ratio The divide ratio, default is 0.
      * @param frequency The oscillator frequency, 0 - 127. Default is 8.  
      */
    void set_display_clock_ratio_and_frequency(unsigned char ratio, unsigned char frequency);
    
    /** Set the precharge period.
      * @param phase1 Phase 1 period in DCLK clocks.  1 - 15, default is 2.
      * @param phase2 Phase 2 period in DCLK clocks.  1 - 15, default is 2.
      */
    void set_precharge_period(unsigned char phase1, unsigned char phase2);
    
    /** Set the Vcomh deselect level.
      * @param level 0 = 0.65 x Vcc, 1 = 0.77 x Vcc (default), 2 = 0.83 x Vcc.
      */
    void set_vcomh_deselect_level(unsigned char level);
    
    /** Perform a "no operation".
      */
    void nop();
    
    /** Enable/disable charge pump.
      @param enable 0 to disable, 1 to enable the internal charge pump.
      */
    void set_charge_pump_enable(unsigned char enable);
    
    // ----- BUFFER EDITING -----

    void clear();
    void set_pixel(int x, int y);
    void clear_pixel(int x, int y);
    void line(int x0, int y0, int x1, int y1);

    /** Set the current console font.
      * @param font Font data, layed out vertically!
      * @param width Width of the font characters in pixels.
      * Fonts are always (at present) 8 pixels in height.
      */
    void set_font(unsigned char *font, unsigned int width);
    
    /** Set double height text output.
      * @param double_height If 1, calls to putc(), printf() etc will
      * result in text taking up 2 lines instead of 1.
      */
    void set_double_height_text(unsigned int double_height);
    
    /** Put a single character to the screen buffer.
      * Repeated calls to putc() will cause the cursor to move across and
      * then down as needed, with scrolling.
      * @param c The character to write.
      */
    void putc(unsigned char c);
    
    /** Print to the screen buffer.
      * printf() will wrap and scroll the screen as needed to display the text given.
      * @param format Format specifier, same as printf() in normal C.
      */
    void printf(const char *format, ...);

    /** Scroll the screen buffer up by one line. */
    void scroll_up();

private:
    SPI _spi;
    DigitalOut _cs, _reset, _dc;
    unsigned char _screen[1024];
    
    int _cursor_x, _cursor_y;

    void _send_command(unsigned char code);
    void _send_data(unsigned char value);
    
    unsigned char *_console_font_data;  
    unsigned int _console_font_width;
    unsigned int _double_height_text;
};

#define SSD1306_LCDWIDTH 128
#define SSD1306_LCDHEIGHT 64

#endif
