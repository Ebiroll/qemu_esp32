#ifndef __WM8978_H
#define __WM8978_H  

#include "hal_i2c.h"									
//////////////////////////////////////////////////////////////////////////////////	 
//Copyright(C) ¹ãÖÝÊÐÐÇÒíµç×Ó¿Æ¼¼ÓÐÏÞ¹«Ë¾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
 
 
//Èç¹ûAD0½Å(4½Å)½ÓµØ,IICµØÖ·Îª0X4A(²»°üº¬×îµÍÎ»).
//Èç¹û½ÓV3.3,ÔòIICµØÖ·Îª0X4B(²»°üº¬×îµÍÎ»).
#define WM8978_ADDR				0X1A	
 
#define EQ1_80Hz		0X00
#define EQ1_105Hz		0X01
#define EQ1_135Hz		0X02
#define EQ1_175Hz		0X03

#define EQ2_230Hz		0X00
#define EQ2_300Hz		0X01
#define EQ2_385Hz		0X02
#define EQ2_500Hz		0X03

#define EQ3_650Hz		0X00
#define EQ3_850Hz		0X01
#define EQ3_1100Hz		0X02
#define EQ3_14000Hz		0X03

#define EQ4_1800Hz		0X00
#define EQ4_2400Hz		0X01
#define EQ4_3200Hz		0X02
#define EQ4_4100Hz		0X03

#define EQ5_5300Hz		0X00
#define EQ5_6900Hz		0X01
#define EQ5_9000Hz		0X02
#define EQ5_11700Hz		0X03

  
#define WM8978_RESET				0x00

#define WM8978_POWER_MANAGEMENT_1		0x01

#define WM8978_POWER_MANAGEMENT_2		0x02

#define WM8978_POWER_MANAGEMENT_3		0x03

#define WM8978_AUDIO_INTERFACE			0x04

#define WM8978_COMPANDING_CONTROL		0x05

#define WM8978_CLOCKING				0x06

#define WM8978_ADDITIONAL_CONTROL		0x07

#define WM8978_GPIO_CONTROL			0x08

#define WM8978_JACK_DETECT_CONTROL_1		0x09

#define WM8978_DAC_CONTROL			0x0A

#define WM8978_LEFT_DAC_DIGITAL_VOLUME		0x0B

#define WM8978_RIGHT_DAC_DIGITAL_VOLUME		0x0C

#define WM8978_JACK_DETECT_CONTROL_2		0x0D

#define WM8978_ADC_CONTROL			0x0E

#define WM8978_LEFT_ADC_DIGITAL_VOLUME		0x0F

#define WM8978_RIGHT_ADC_DIGITAL_VOLUME		0x10

#define WM8978_EQ1				0x12

#define WM8978_EQ2				0x13

#define WM8978_EQ3				0x14

#define WM8978_EQ4				0x15

#define WM8978_EQ5				0x16

#define WM8978_DAC_LIMITER_1			0x18

#define WM8978_DAC_LIMITER_2			0x19

#define WM8978_NOTCH_FILTER_1			0x1b

#define WM8978_NOTCH_FILTER_2			0x1c

#define WM8978_NOTCH_FILTER_3			0x1d

#define WM8978_NOTCH_FILTER_4			0x1e

#define WM8978_ALC_CONTROL_1			0x20

#define WM8978_ALC_CONTROL_2			0x21

#define WM8978_ALC_CONTROL_3			0x22

#define WM8978_NOISE_GATE			0x23

#define WM8978_PLL_N				0x24

#define WM8978_PLL_K1				0x25

#define WM8978_PLL_K2				0x26

#define WM8978_PLL_K3				0x27

#define WM8978_3D_CONTROL			0x29

#define WM8978_BEEP_CONTROL			0x2b

#define WM8978_INPUT_CONTROL			0x2c

#define WM8978_LEFT_INP_PGA_CONTROL		0x2d

#define WM8978_RIGHT_INP_PGA_CONTROL		0x2e

#define WM8978_LEFT_ADC_BOOST_CONTROL		0x2f

#define WM8978_RIGHT_ADC_BOOST_CONTROL		0x30

#define WM8978_OUTPUT_CONTROL			0x31

#define WM8978_LEFT_MIXER_CONTROL		0x32

#define WM8978_RIGHT_MIXER_CONTROL		0x33

#define WM8978_LOUT1_HP_CONTROL			0x34

#define WM8978_ROUT1_HP_CONTROL			0x35

#define WM8978_LOUT2_SPK_CONTROL		0x36

#define WM8978_ROUT2_SPK_CONTROL		0x37

#define WM8978_OUT3_MIXER_CONTROL		0x38

#define WM8978_OUT4_MIXER_CONTROL		0x39


 
uint8_t WM8978_Init(void); 
void WM8978_ADDA_Cfg(uint8_t dacen,uint8_t adcen);
void WM8978_Input_Cfg(uint8_t micen,uint8_t lineinen,uint8_t auxen);
void WM8978_Output_Cfg(uint8_t dacen,uint8_t bpsen);
void WM8978_MIC_Gain(uint8_t gain);
void WM8978_LINEIN_Gain(uint8_t gain);
void WM8978_AUX_Gain(uint8_t gain);
uint8_t WM8978_Write_Reg(uint8_t reg,uint16_t val); 
uint16_t WM8978_Read_Reg(uint8_t reg);
void WM8978_HPvol_Set(uint8_t voll,uint8_t volr);
void WM8978_SPKvol_Set(uint8_t volx);
void WM8978_I2S_Cfg(uint8_t fmt,uint8_t len);
void WM8978_3D_Set(uint8_t depth);
void WM8978_EQ_3D_Dir(uint8_t dir); 
void WM8978_EQ1_Set(uint8_t cfreq,uint8_t gain); 
void WM8978_EQ2_Set(uint8_t cfreq,uint8_t gain);
void WM8978_EQ3_Set(uint8_t cfreq,uint8_t gain);
void WM8978_EQ4_Set(uint8_t cfreq,uint8_t gain);
void WM8978_EQ5_Set(uint8_t cfreq,uint8_t gain);

#endif





















