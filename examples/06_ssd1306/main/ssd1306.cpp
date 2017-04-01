#include "ssd1306.hpp"

OLED::OLED(gpio_num_t scl, gpio_num_t sda, ssd1306_panel_type_t type,
		uint8_t address) {
	i2c = new I2C(scl, sda);
	this->type = type;
	this->address = address;

	switch (type) {
	case SSD1306_128x64:
		buffer = NULL;
		width = 128;
		height = 64;
		break;
	case SSD1306_128x32:
		buffer = NULL;
		width = 128;
		height = 32;
		break;
	}
}

OLED::OLED(gpio_num_t scl, gpio_num_t sda, ssd1306_panel_type_t type) {
	i2c = new I2C(scl, sda);
	this->type = type;
	this->address = 0x78;

	switch (type) {
	case SSD1306_128x64:
		buffer = (uint8_t*) malloc(1024); // 128 * 64 / 8
		width = 128;
		height = 64;
		break;
	case SSD1306_128x32:
		buffer = (uint8_t*) malloc(512);  // 128 * 32 / 8
		width = 128;
		height = 32;
		break;
	}

}

void OLED::command(uint8_t adress, uint8_t c) {
	bool ret;
	i2c->start();
	ret = i2c->write(adress);
	if (!ret) // NACK
		i2c->stop();
	i2c->write(0x00);    // Co = 0, D/C = 0
	i2c->write(c);
	i2c->stop();
}

void OLED::data(uint8_t adress, uint8_t d) {
	bool ret;
	i2c->start();
	ret = i2c->write(adress);
	if (!ret) // NACK
		i2c->stop();
	i2c->write(0x40);    // Co = 0, D/C = 1
	i2c->write(d);
	i2c->stop();
}

bool OLED::init() {
	term();

	switch (type) {
	case SSD1306_128x64:
		buffer = (uint8_t*) malloc(1024); // 128 * 64 / 8
		break;
	case SSD1306_128x32:
		buffer = (uint8_t*) malloc(512);  // 128 * 32 / 8
		break;
	}

	if (buffer == NULL) {
		ESP_LOGE("oled", "Alloc OLED buffer failed.");
		goto oled_init_fail;
	}

	// Panel initialization
	// Try send I2C address check if the panel is connected
	i2c->start();
	if (!i2c->write(address)) {
		i2c->stop();
		ESP_LOGE("oled", "OLED I2C bus not responding.");
		goto oled_init_fail;
	}
	i2c->stop();

	// Now we assume all sending will be successful
	if (type == SSD1306_128x64) {
		command(address, 0xae); // SSD1306_DISPLAYOFF
		command(address, 0xd5); // SSD1306_SETDISPLAYCLOCKDIV
		command(address, 0x80); // Suggested value 0x80
		command(address, 0xa8); // SSD1306_SETMULTIPLEX
		command(address, 0x3f); // 1/64
		command(address, 0xd3); // SSD1306_SETDISPLAYOFFSET
		command(address, 0x00); // 0 no offset
		command(address, 0x40); // SSD1306_SETSTARTLINE line #0
		command(address, 0x20); // SSD1306_MEMORYMODE
		command(address, 0x00); // 0x0 act like ks0108
		command(address, 0xa1); // SSD1306_SEGREMAP | 1
		command(address, 0xc8); // SSD1306_COMSCANDEC
		command(address, 0xda); // SSD1306_SETCOMPINS
		command(address, 0x12);
		command(address, 0x81); // SSD1306_SETCONTRAST
		command(address, 0xcf);
		command(address, 0xd9); // SSD1306_SETPRECHARGE
		command(address, 0xf1);
		command(address, 0xdb); // SSD1306_SETVCOMDETECT
		command(address, 0x30);
		command(address, 0x8d); // SSD1306_CHARGEPUMP
		command(address, 0x14); // Charge pump on
		command(address, 0x2e); // SSD1306_DEACTIVATE_SCROLL
		command(address, 0xa4); // SSD1306_DISPLAYALLON_RESUME
		command(address, 0xa6); // SSD1306_NORMALDISPLAY
	} else if (type == SSD1306_128x32) {
		command(address, 0xae); // SSD1306_DISPLAYOFF
		command(address, 0xd5); // SSD1306_SETDISPLAYCLOCKDIV
		command(address, 0x80); // Suggested value 0x80
		command(address, 0xa8); // SSD1306_SETMULTIPLEX
		command(address, 0x1f); // 1/32
		command(address, 0xd3); // SSD1306_SETDISPLAYOFFSET
		command(address, 0x00); // 0 no offset
		command(address, 0x40); // SSD1306_SETSTARTLINE line #0
		command(address, 0x8d); // SSD1306_CHARGEPUMP
		command(address, 0x14); // Charge pump on
		command(address, 0x20); // SSD1306_MEMORYMODE
		command(address, 0x00); // 0x0 act like ks0108
		command(address, 0xa1); // SSD1306_SEGREMAP | 1
		command(address, 0xc8); // SSD1306_COMSCANDEC
		command(address, 0xda); // SSD1306_SETCOMPINS
		command(address, 0x02);
		command(address, 0x81); // SSD1306_SETCONTRAST
		command(address, 0x2f);
		command(address, 0xd9); // SSD1306_SETPRECHARGE
		command(address, 0xf1);
		command(address, 0xdb); // SSD1306_SETVCOMDETECT
		command(address, 0x40);
		command(address, 0x2e); // SSD1306_DEACTIVATE_SCROLL
		command(address, 0xa4); // SSD1306_DISPLAYALLON_RESUME
		command(address, 0xa6); // SSD1306_NORMALDISPLAY
	}

	clear();
	refresh(true);

	command(address, 0xaf); // SSD1306_DISPLAYON

	return true;

	oled_init_fail: if (buffer)
		free(buffer);
	return false;
}

void OLED::term() {
	command(address, 0xae); // SSD_DISPLAYOFF
	command(address, 0x8d); // SSD1306_CHARGEPUMP
	command(address, 0x10); // Charge pump off

	if (buffer)
		free(buffer);
}

uint8_t OLED::get_width() {
	return width;
}

uint8_t OLED::get_height() {
	return height;
}

void OLED::clear() {
	switch (type) {
	case SSD1306_128x64:
		memset(buffer, 0, 1024);
		break;
	case SSD1306_128x32:
		memset(buffer, 0, 512);
		break;
	}
	refresh_right = width - 1;
	refresh_bottom = height - 1;
	refresh_top = 0;
	refresh_left = 0;
}

void OLED::refresh( bool force) {
	uint8_t i, j;
	uint16_t k;
	uint8_t page_start, page_end;

	if (force) {
		switch (type) {
		case SSD1306_128x64:
			command(address, 0x21); // SSD1306_COLUMNADDR
			command(address, 0);    // column start
			command(address, 127);  // column end
			command(address, 0x22); // SSD1306_PAGEADDR
			command(address, 0);    // page start
			command(address, 7);    // page end (8 pages for 64 rows OLED)
			for (k = 0; k < 1024; k++) {
				i2c->start();
				i2c->write(address);
				i2c->write(0x40);
				for (j = 0; j < 16; ++j) {
					i2c->write(buffer[k]);
					++k;
				}
				--k;
				i2c->stop();
			}
			break;
		case SSD1306_128x32:
			command(address, 0x21); // SSD1306_COLUMNADDR
			command(address, 0);    // column start
			command(address, 127);  // column end
			command(address, 0x22); // SSD1306_PAGEADDR
			command(address, 0);    // page start
			command(address, 3);    // page end (4 pages for 32 rows OLED)
			for (k = 0; k < 512; k++) {
				i2c->start();
				i2c->write(address);
				i2c->write(0x40);
				for (j = 0; j < 16; ++j) {
					i2c->write(buffer[k]);
					++k;
				}
				--k;
				i2c->stop();
			}
			break;
		}
	} else {
		if ((refresh_top <= refresh_bottom)
				&& (refresh_left <= refresh_right)) {
			page_start = refresh_top / 8;
			page_end = refresh_bottom / 8;
			command(address, 0x21); // SSD1306_COLUMNADDR
			command(address, refresh_left);    // column start
			command(address, refresh_right);   // column end
			command(address, 0x22); // SSD1306_PAGEADDR
			command(address, page_start);    // page start
			command(address, page_end); // page end
			k = 0;
			for (i = page_start; i <= page_end; ++i) {
				for (j = refresh_left; j <= refresh_right; ++j) {
					if (k == 0) {
						i2c->start();
						i2c->write(address);
						i2c->write(0x40);
					}
					i2c->write(buffer[i * width + j]);
					++k;
					if (k == 16) {
						i2c->stop();
						k = 0;
					}

				}
			}
			if (k != 0) // for last batch if stop was not sent
				i2c->stop();
		}
	}
	// reset dirty area
	refresh_top = 255;
	refresh_left = 255;
	refresh_right = 0;
	refresh_bottom = 0;
}

void OLED::draw_pixel(int8_t x, int8_t y, ssd1306_color_t color) {
	uint16_t index;

	if ((x >= width) || (x < 0) || (y >= height) || (y < 0))
		return;

	index = x + (y / 8) * width;
	switch (color) {
	case WHITE:
		buffer[index] |= (1 << (y & 7));
		break;
	case BLACK:
		buffer[index] &= ~(1 << (y & 7));
		break;
	case INVERT:
		buffer[index] ^= (1 << (y & 7));
		break;
	default:
		break;
	}
	if (refresh_left > x)
		refresh_left = x;
	if (refresh_right < x)
		refresh_right = x;
	if (refresh_top > y)
		refresh_top = y;
	if (refresh_bottom < y)
		refresh_bottom = y;
}

void OLED::draw_hline(int8_t x, int8_t y, uint8_t w, ssd1306_color_t color) {
	uint16_t index;
	uint8_t mask, t;
	// boundary check
	if ((x >= width) || (x < 0) || (y >= height) || (y < 0))
		return;
	if (w == 0)
		return;
	if (x + w > width)
		w = width - x;

	t = w;
	index = x + (y / 8) * width;
	mask = 1 << (y & 7);
	switch (color) {
	case WHITE:
		while (t--) {
			buffer[index] |= mask;
			++index;
		}
		break;
	case BLACK:
		mask = ~mask;
		while (t--) {
			buffer[index] &= mask;
			++index;
		}
		break;
	case INVERT:
		while (t--) {
			buffer[index] ^= mask;
			++index;
		}
		break;
	default:
		break;
	}
	if (refresh_left > x)
		refresh_left = x;
	if (refresh_right < x + w - 1)
		refresh_right = x + w - 1;
	if (refresh_top > y)
		refresh_top = y;
	if (refresh_bottom < y)
		refresh_bottom = y;
}

void OLED::draw_vline(int8_t x, int8_t y, uint8_t h, ssd1306_color_t color) {
	uint16_t index;
	uint8_t mask, mod, t;

	// boundary check
	if ((x >= width) || (x < 0) || (y >= height) || (y < 0))
		return;
	if (h == 0)
		return;
	if (y + h > height)
		h = height - y;

	t = h;
	index = x + (y / 8) * width;
	mod = y & 7;
	if (mod) // partial line that does not fit into byte at top
	{
		// Magic from Adafruit
		mod = 8 - mod;
		static const uint8_t premask[8] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8,
				0xFC, 0xFE };
		mask = premask[mod];
		if (t < mod)
			mask &= (0xFF >> (mod - t));
		switch (color) {
		case WHITE:
			buffer[index] |= mask;
			break;
		case BLACK:
			buffer[index] &= ~mask;
			break;
		case INVERT:
			buffer[index] ^= mask;
			break;
		default:
			break;
		}
		if (t < mod)
			goto draw_vline_finish;
		t -= mod;
		index += width;
	}
	if (t >= 8) // byte aligned line at middle
			{
		switch (color) {
		case WHITE:
			do {
				buffer[index] = 0xff;
				index += width;
				t -= 8;
			} while (t >= 8);
			break;
		case BLACK:
			do {
				buffer[index] = 0x00;
				index += width;
				t -= 8;
			} while (t >= 8);
			break;
		case INVERT:
			do {
				buffer[index] = ~buffer[index];
				index += width;
				t -= 8;
			} while (t >= 8);
			break;
		default:
			break;
		}
	}
	if (t) // // partial line at bottom
	{
		mod = t & 7;
		static const uint8_t postmask[8] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F,
				0x3F, 0x7F };
		mask = postmask[mod];
		switch (color) {
		case WHITE:
			buffer[index] |= mask;
			break;
		case BLACK:
			buffer[index] &= ~mask;
			break;
		case INVERT:
			buffer[index] ^= mask;
			break;
		default:
			break;
		}
	}
	draw_vline_finish: if (refresh_left > x)
		refresh_left = x;
	if (refresh_right < x)
		refresh_right = x;
	if (refresh_top > y)
		refresh_top = y;
	if (refresh_bottom < y + h - 1)
		refresh_bottom = y + h - 1;
	return;
}

void OLED::draw_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h,
		ssd1306_color_t color) {
	draw_hline(x, y, w, color);
	draw_hline(x, y + h - 1, w, color);
	draw_vline(x, y, h, color);
	draw_vline(x + w - 1, y, h, color);
}

void OLED::fill_rectangle(int8_t x, int8_t y, uint8_t w, uint8_t h,
		ssd1306_color_t color) {
	// Can be optimized?
	uint8_t i;
	for (i = x; i < x + w; ++i)
		draw_vline(i, y, h, color);
}

void OLED::draw_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color) {
	// Refer to http://en.wikipedia.org/wiki/Midpoint_circle_algorithm for the algorithm

	int8_t x = r;
	int8_t y = 1;
	int16_t radius_err = 1 - x;

	if (r == 0)
		return;

	draw_pixel(x0 - r, y0, color);
	draw_pixel(x0 + r, y0, color);
	draw_pixel(x0, y0 - r, color);
	draw_pixel(x0, y0 + r, color);

	while (x >= y) {
		draw_pixel(x0 + x, y0 + y, color);
		draw_pixel(x0 - x, y0 + y, color);
		draw_pixel(x0 + x, y0 - y, color);
		draw_pixel(x0 - x, y0 - y, color);
		if (x != y) {
			/* Otherwise the 4 drawings below are the same as above, causing
			 * problem when color is INVERT
			 */
			draw_pixel(x0 + y, y0 + x, color);
			draw_pixel(x0 - y, y0 + x, color);
			draw_pixel(x0 + y, y0 - x, color);
			draw_pixel(x0 - y, y0 - x, color);
		}
		++y;
		if (radius_err < 0) {
			radius_err += 2 * y + 1;
		} else {
			--x;
			radius_err += 2 * (y - x + 1);
		}

	}
}

void OLED::fill_circle(int8_t x0, int8_t y0, uint8_t r, ssd1306_color_t color) {
	int8_t x = 1;
	int8_t y = r;
	int16_t radius_err = 1 - y;
	int8_t x1;

	if (r == 0)
		return;

	draw_vline(x0, y0 - r, 2 * r + 1, color); // Center vertical line
	while (y >= x) {
		draw_vline(x0 - x, y0 - y, 2 * y + 1, color);
		draw_vline(x0 + x, y0 - y, 2 * y + 1, color);
		if (color != INVERT) {
			draw_vline(x0 - y, y0 - x, 2 * x + 1, color);
			draw_vline(x0 + y, y0 - x, 2 * x + 1, color);
		}
		++x;
		if (radius_err < 0) {
			radius_err += 2 * x + 1;
		} else {
			--y;
			radius_err += 2 * (x - y + 1);
		}
	}

	if (color == INVERT) {
		x1 = x; // Save where we stopped

		y = 1;
		x = r;
		radius_err = 1 - x;
		draw_hline(x0 + x1, y0, r - x1 + 1, color);
		draw_hline(x0 - r, y0, r - x1 + 1, color);
		while (x >= y) {
			draw_hline(x0 + x1, y0 - y, x - x1 + 1, color);
			draw_hline(x0 + x1, y0 + y, x - x1 + 1, color);
			draw_hline(x0 - x, y0 - y, x - x1 + 1, color);
			draw_hline(x0 - x, y0 + y, x - x1 + 1, color);
			++y;
			if (radius_err < 0) {
				radius_err += 2 * y + 1;
			} else {
				--x;
				radius_err += 2 * (y - x + 1);
			}
		}
	}
}

void OLED::select_font(uint8_t idx) {
	if (idx < NUM_FONTS)
		font = fonts[idx];
}

// return character width
uint8_t OLED::draw_char(uint8_t x, uint8_t y, unsigned char c,
		ssd1306_color_t foreground, ssd1306_color_t background) {
	uint8_t i, j;
	const uint8_t *bitmap;
	uint8_t line = 0;

	if (font == NULL)
		return 0;

	// we always have space in the font set
	if ((c < font->char_start) || (c > font->char_end))
		c = ' ';
	c = c - font->char_start;   // c now become index to tables
	bitmap = font->bitmap + font->char_descriptors[c].offset;
	for (j = 0; j < font->height; ++j) {
		for (i = 0; i < font->char_descriptors[c].width; ++i) {
			if (i % 8 == 0) {
				line = bitmap[(font->char_descriptors[c].width + 7) / 8 * j
						+ i / 8]; // line data
			}
			if (line & 0x80) {
				draw_pixel(x + i, y + j, foreground);
			} else {
				switch (background) {
				case TRANSPARENT:
					// Not drawing for transparent background
					break;
				case WHITE:
				case BLACK:
					draw_pixel(x + i, y + j, background);
					break;
				case INVERT:
					// I don't know why I need invert background
					break;
				}
			}
			line = line << 1;
		}
	}
	return (font->char_descriptors[c].width);
}

uint8_t OLED::draw_string(uint8_t x, uint8_t y, std::string str,
		ssd1306_color_t foreground, ssd1306_color_t background) {
	uint8_t t = x;

	if (font == NULL)
		return 0;

	//	if(str==NULL)
	//		return 0;

	if (str.empty())
		return 0;

	for (std::string::iterator i = str.begin(); i < str.end(); i++) {
		x += draw_char(x, y, *i, foreground, background);
		if (*i)
			x += font->c;
	}

//	while (*str) {
//		x += draw_char(x, y, *str, foreground, background);
//		++str;
//		if (*str)
//			x += font->c;
//	}

	return (x - t);
}

// return width of string
uint8_t OLED::measure_string(std::string str) {
	uint8_t w = 0;
	unsigned char c;

	if (str.empty())
		return 0;

//	if(str==NULL)
//		return 0;

	if (font == NULL)
		return 0;

	for (std::string::iterator i = str.begin(); i < str.end(); i++) {
		c = *i;
		// we always have space in the font set
		if ((c < font->char_start) || (c > font->char_end))
			c = ' ';
		c = c - font->char_start;   // c now become index to tables
		w += font->char_descriptors[c].width;
		if (*i)
			w += font->c;
	}

//	while (*str) {
//		c = *str;
//		// we always have space in the font set
//		if ((c < font->char_start) || (c > font->char_end))
//			c = ' ';
//		c = c - font->char_start;   // c now become index to tables
//		w += font->char_descriptors[c].width;
//		++str;
//		if (*str)
//			w += font->c;
//	}
	return w;
}

uint8_t OLED::get_font_height() {
	if (font == NULL)
		return 0;

	return (font->height);
}

uint8_t OLED::get_font_c() {
	if (font == NULL)
		return 0;

	return (font->c);
}

void OLED::invert_display(bool invert) {
	if (invert)
		command(address, 0xa7); // SSD1306_INVERTDISPLAY
	else
		command(address, 0xa6); // SSD1306_NORMALDISPLAY

}

void OLED::update_buffer(uint8_t* data, uint16_t length) {
	if (type == SSD1306_128x64) {
		memcpy(buffer, data, (length < 1024) ? length : 1024);

	} else if (type == SSD1306_128x32) {
		memcpy(buffer, data, (length < 512) ? length : 512);
	}
	refresh_right = width - 1;
	refresh_bottom = height - 1;
	refresh_top = 0;
	refresh_left = 0;
}
