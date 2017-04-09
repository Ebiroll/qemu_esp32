

#ifndef _1306_H_
#define _1306_H_


#ifdef __cplusplus
extern "C" {
#endif

void ssd1306_128x64_noname_init();

void display_dot(unsigned char start_col);

void display_three_numbers(unsigned char number,unsigned char start_col);

void Display_Chess(unsigned char value);

void clear_display();

// Set page address 0~15
void Set_Page_Address(unsigned char add);

void Set_Column_Address(unsigned char add);

void Write_data(unsigned char *data,unsigned char len);

#ifdef __cplusplus
}
#endif


#endif