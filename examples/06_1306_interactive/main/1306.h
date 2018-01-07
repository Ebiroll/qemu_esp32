

#ifndef _1306_H_
#define _1306_H_


#ifdef __cplusplus
extern "C" {
#endif

void ssd1306_128x64_noname_init();

void ssd1306_128x64_noname_powersave_off();

void ssd1306_128x64_noname_powersave_on();


void display_dot(unsigned char start_col);

  void display_three_numbers(unsigned char number,unsigned char start_col,unsigned char page);

void Display_Chess(unsigned char value);

void clear_display();

// Set page address 0~15
void Set_Page_Address(unsigned char add);

void Set_Column_Address(unsigned char add);

void Write_data(unsigned char *data,unsigned char len);

void Write_command(unsigned char command);
  
void Write_multi_command(unsigned char *data,unsigned char len);
  
#ifdef __cplusplus
}
#endif


#endif
