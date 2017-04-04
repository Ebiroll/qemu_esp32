
#ifndef TEST_SDD1306
#define TEST_SDD1306

#include "esp32-hal-i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// This must be called first
void set_i2c(i2c_t *i2c);

void Initial(void);
void Display_Chess(unsigned char value);
void display_three_numbers(unsigned char number);

#ifdef __cplusplus
}
#endif


#endif