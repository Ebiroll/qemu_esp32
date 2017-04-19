
#include "mbed.h"
#include "ssd1306.h"

#include <stdarg.h>

SSD1306::SSD1306(PinName cs, PinName rs, PinName dc, PinName clk, PinName data)
    : _spi(data, NC, clk), 
      _cs(cs), 
      _reset(rs), 
      _dc(dc),
      _cursor_x(0),
      _cursor_y(0)
{
}

void SSD1306::off()
{
    _send_command(0xAE);
}

void SSD1306::on()
{
    _send_command(0xAF);
}

void SSD1306::sleep()
{
    _send_command(0xAE);
}

void SSD1306::wake()
{
    _send_command(0xAF);
}

void SSD1306::set_inverse(unsigned char value)
{
    _send_command(value ? 0xA7 : 0xA6);
}

void SSD1306::set_display_offset(unsigned char value)
{
    _send_command(0xD3);
    _send_command(value & 0x3F); 
}

void SSD1306::set_contrast(unsigned char value) 
{
    _send_command(0x81);
    _send_command(value);
}

void SSD1306::set_display_start_line(unsigned char value)
{
    _send_command(0x40 | value);
}

void SSD1306::set_segment_remap(unsigned char value)
{
    _send_command(value ? 0xA1 : 0xA0);
}

void SSD1306::set_multiplex_ratio(unsigned char value)
{
    _send_command(0xA8);
    _send_command(value & 0x3F);
}

void SSD1306::set_com_output_scan_direction(unsigned char value)
{
    _send_command(value ? 0xC8 : 0xC0);
}

void SSD1306::set_com_pins_hardware_configuration(unsigned char sequential, unsigned char lr_remap)
{
    _send_command(0xDA);
    _send_command(0x02 | ((sequential & 1) << 4) | ((lr_remap & 1) << 5));
}

void SSD1306::start_horizontal_scroll(unsigned char direction, unsigned char start, unsigned char end, unsigned char interval) 
{
    _send_command(direction ? 0x27 : 0x26);
    _send_command(0x00);
    _send_command(start & 0x07);
    switch (interval) {
        case   2: _send_command(0x07); break; // 111b
        case   3: _send_command(0x04); break; // 100b
        case   4: _send_command(0x05); break; // 101b
        case   5: _send_command(0x00); break; // 000b
        case  25: _send_command(0x06); break; // 110b
        case  64: _send_command(0x01); break; // 001b
        case 128: _send_command(0x02); break; // 010b
        case 256: _send_command(0x03); break; // 011b
        default:
            // default to 2 frame interval
            _send_command(0x07); break;
    }
    _send_command(end & 0x07);
    _send_command(0x00);
    _send_command(0xFF);
    
    // activate scroll
    _send_command(0x2F);
}

void SSD1306::start_vertical_and_horizontal_scroll(unsigned char direction, unsigned char start, unsigned char end, unsigned char interval, unsigned char vertical_offset)
{
    _send_command(direction ? 0x2A : 0x29);
    _send_command(0x00);
    _send_command(start & 0x07);
    switch (interval) {
        case   2: _send_command(0x07); break; // 111b
        case   3: _send_command(0x04); break; // 100b
        case   4: _send_command(0x05); break; // 101b
        case   5: _send_command(0x00); break; // 000b
        case  25: _send_command(0x06); break; // 110b
        case  64: _send_command(0x01); break; // 001b
        case 128: _send_command(0x02); break; // 010b
        case 256: _send_command(0x03); break; // 011b
        default:
            // default to 2 frame interval
            _send_command(0x07); break;
    }
    _send_command(end & 0x07);
    _send_command(vertical_offset);    
    
    // activate scroll
    _send_command(0x2F);
}

void SSD1306::stop_scroll()
{
    // all scroll configurations are removed from the display when executing this command.
    _send_command(0x2E);
}

void SSD1306::pam_set_start_address(unsigned char address)
{
    // "Set Lower Column Start Address for Page Addressing Mode"
    _send_command(address & 0x0F);
    
    // "Set Higher Column Start Address for Page Addressing Mode"
    _send_command((address << 4) & 0x0F);
}

void SSD1306::set_memory_addressing_mode(unsigned char mode)
{
    _send_command(0x20);
    _send_command(mode & 0x3);
}

void SSD1306::hv_set_column_address(unsigned char start, unsigned char end)
{
    _send_command(0x21);
    _send_command(start & 0x7F);
    _send_command(end & 0x7F);
}

void SSD1306::hv_set_page_address(unsigned char start, unsigned char end)
{
    _send_command(0x22);
    _send_command(start & 0x07);
    _send_command(end & 0x07);
}

void SSD1306::pam_set_page_start(unsigned char address)
{
    _send_command(0xB0 | (address & 0x07));
}

void SSD1306::set_display_clock_ratio_and_frequency(unsigned char ratio, unsigned char frequency)
{
    _send_command(0xD5);
    _send_command((ratio & 0x0F) | ((frequency & 0x0F) << 4));
}

void SSD1306::set_precharge_period(unsigned char phase1, unsigned char phase2)
{
    _send_command(0xD9);
    _send_command((phase1 & 0x0F) | ((phase2 & 0x0F ) << 4));
}

void SSD1306::set_vcomh_deselect_level(unsigned char level)
{
    _send_command(0xDB);
    _send_command((level & 0x03) << 4);
}

void SSD1306::nop()
{
    _send_command(0xE3);
}

void SSD1306::set_charge_pump_enable(unsigned char enable)
{
    _send_command(0x8D);
    _send_command(enable ? 0x14 : 0x10);
}

void SSD1306::initialise()
{
    // Init
    _reset = 1;
    wait(0.01);
    _reset = 0;
    wait(0.10);
    _reset = 1;
    
    off();

    set_display_clock_ratio_and_frequency(0, 8);
    set_multiplex_ratio(0x3F); // 1/64 duty
    set_precharge_period(0xF, 0x01);
    set_display_offset(0);    
    set_display_start_line(0);  
    set_charge_pump_enable(1);    
    set_memory_addressing_mode(0); // horizontal addressing mode; across then down
    set_segment_remap(1);
    set_com_output_scan_direction(1);
    set_com_pins_hardware_configuration(1, 0);
    set_contrast(0xFF);
    set_vcomh_deselect_level(1);
    
    wake();
    set_inverse(0);
    
    hv_set_column_address(0, 127);
    hv_set_page_address(0, 7);
    
    pam_set_start_address(0);
    pam_set_page_start(0);
    
    // set_precharge_period(2, 2);
}

void SSD1306::update()
{
    hv_set_column_address(0, 127);
    hv_set_page_address(0, 7);
    
    for (int i = 0; i < 1024; i++)
        _send_data(_screen[i]);
}

void SSD1306::set_pixel(int x, int y)
{
    if (x >= SSD1306_LCDWIDTH || y >= SSD1306_LCDHEIGHT) return;
    
    _screen[x + (y / 8) * 128] |= 1 << (y % 8);
}

void SSD1306::clear_pixel(int x, int y)
{
    if (x >= SSD1306_LCDWIDTH || y >= SSD1306_LCDHEIGHT) return;
    
    _screen[x + (y / 8) * 128] &= ~(1 << (y % 8));
}

void SSD1306::line(int x0, int y0, int x1, int y1) {
  int steep = abs(y1 - y0) > abs(x1 - x0);
  int t;
  
  if (steep) {
    t = x0; x0 = y0; y0 = t;
    t = x1; x1 = y1; y1 = t;
  }

  if (x0 > x1) {
    t = x0; x0 = x1; x1 = t;
    t = y0; y0 = y1; y1 = t;
  }

  int dx, dy;
  
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int err = dx / 2;
  int ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;}

  for (; x0<x1; x0++) {
    if (steep) {
      set_pixel(y0, x0);
    } else {
      set_pixel(x0, y0);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void SSD1306::set_font(unsigned char *font, unsigned int width)
{
    _console_font_data = font;
    _console_font_width = width;
}

void SSD1306::set_double_height_text(unsigned int double_height)
{
    _double_height_text = double_height;
}

void SSD1306::putc(unsigned char c)
{
    while (_cursor_x >= (128 / _console_font_width))
    {
        _cursor_x -= (128 / _console_font_width);
        _cursor_y++;            
    }

    while (_cursor_y > 7)
    {
        scroll_up();            
    }

    switch (c)
    {
    case '\n':
        _cursor_y++;
        break;
        
    case '\r':
        _cursor_x = 0;
        break;
        
    case '\t':
        _cursor_x = (_cursor_x + 4) % 4;
        break;
        
    default:
        for (int b = 0; b < _console_font_width; b++)
        {
            _screen[_cursor_x * _console_font_width + _cursor_y * 128 + b] = _console_font_data[(c - FONT_START) * _console_font_width + b];
        }
        
        _cursor_x++;
    }            
}

void SSD1306::scroll_up()
{
    for (int y = 1; y <= 7; y++)
    {
        for (int x = 0; x < 128; x++)
        {
            _screen[x + 128 * (y - 1)] = _screen[x + 128 * y];
        }
    }
    
    for (int x = 0; x < 128; x++)
    {
        _screen[x + 128 * 7] = 0;
    }    

    _cursor_y--;
}

void SSD1306::printf(const char *format, ...)
{
    static char buffer[128];
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    char *c = (char *)&buffer;
    while (*c != 0)
    {
        putc(*c++);
    }
}

void SSD1306::clear()
{
    for (int i = 0; i < 1024; i++)
        _screen[i] = 0;
        
    _cursor_x = 0;
    _cursor_y = 0;
}

void SSD1306::_send_command(unsigned char code)
{
    _cs = 1;
    _dc = 0;
    _cs = 0;
    _spi.write(code);
    _cs = 1;
}

void SSD1306::_send_data(unsigned char value)
{
    _cs = 1;
    _dc = 1;
    _cs = 0;
    _spi.write(value);
    _cs = 1;
}
