
#include <stdlib.h>
#include "esp32_i2c.h"
#include "1306.h"

unsigned char Contrast_level=0x8F;

#define Write_Address 0x78 /*slave addresses with write*/
#define Read_Address 0x79 /*slave addresses with read*/
#define Slave_Address 0x3C

#define Start_column	0x00
#define Start_page		0x00
#define	StartLine_set	0x00

#include <stdio.h>

//esp_err_t i2c_1306_write_data(uint8_t* data,int len);

//esp_err_t i2c_1306_write_command(uint8_t* data,int len);


#define I2C_1306_C(c0)				{ unsigned char com[]= { c0 }; i2c_1306_write_command(com,1);   };   
//#define I2C_1306_A(a0)				{ unsigned char data[]= { a0 }; i2c_1306_write_data(data,1);   };
#define I2C_1306_CA(c0,a0)			{ unsigned char com[]= { c0,a0 }; i2c_1306_write_command(com,2);   };
#define I2C_1306_CAA(c0,a0,a1)		{ unsigned char com[]= { c0,a0,a1 }; i2c_1306_write_command(com,3);   };

/*-- Bitmaps for numbers 0-9 -- 8*16 bits*/
unsigned char 	    num[]={
0x00,0xF8,0xFC,0x04,0x04,0xFC,0xF8,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,  	   
0x00,0x00,0x08,0xFC,0xFC,0x00,0x00,0x00,0x00,0x00,0x04,0x07,0x07,0x04,0x00,0x00,  
0x00,0x18,0x1C,0x84,0xC4,0x7C,0x38,0x00,0x00,0x06,0x07,0x05,0x04,0x04,0x04,0x00,
0x00,0x08,0x0C,0x24,0x24,0xFC,0xD8,0x00,0x00,0x02,0x06,0x04,0x04,0x07,0x03,0x00,
0x80,0xE0,0x70,0x18,0xFC,0xFC,0x00,0x00,0x00,0x01,0x01,0x05,0x07,0x07,0x04,0x00,
0x00,0x7C,0x7C,0x24,0x24,0xE4,0xC4,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,
0x00,0xF0,0xF8,0x6C,0x24,0xEC,0xCC,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,
0x00,0x0C,0x0C,0xC4,0xFC,0x3C,0x04,0x00,0x00,0x00,0x00,0x07,0x07,0x00,0x00,0x00,
0x00,0x98,0xFC,0x64,0x64,0xFC,0x98,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,
0x00,0x78,0xFC,0x84,0xC4,0xFC,0xF8,0x00,0x00,0x06,0x06,0x04,0x06,0x03,0x01,0x00};

// 4 pixels centered
const unsigned char dot[] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x60,
    0x60,
    0x00,
    0x00
};


/* more or less generic setup of all these small OLEDs */
void ssd1306_128x64_noname_init()  {
    
  //1306_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  I2C_1306_C(0x0ae);		                /* display off */
  I2C_1306_CA(0x0d5, 0x080);		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  I2C_1306_CA(0x0a8, 0x03f);		/* multiplex ratio */
  I2C_1306_CA(0x0d3, 0x000);		/* display offset */
  I2C_1306_C(0x040);		                /* set display start line to 0 */
  I2C_1306_CA(0x08d, 0x014);		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable */
  I2C_1306_CA(0x020, 0x000);		/* page addressing mode */
  
  I2C_1306_C(0x0a1);				/* segment remap a0/a1*/
  I2C_1306_C(0x0c8);				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // I2C_1306_C(0x0a0),				/* segment remap a0/a1*/
  // I2C_1306_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  I2C_1306_CA(0x0da, 0x012);		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  I2C_1306_CA(0x081, 0x0cf); 		/* [2] set contrast control */
  I2C_1306_CA(0x0d9, 0x0f1); 		/* [2] pre-charge period 0x022/f1*/
  I2C_1306_CA(0x0db, 0x040); 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  I2C_1306_C(0x02e);				/* Deactivate scroll */ 
  I2C_1306_C(0x0a4);				/* output ram to display */
  I2C_1306_C(0x0a6);				/* none inverted normal display mode */
    
  //1306_END()             			/* end of sequence */
};

void ssd1306_128x64_noname_powersave_off() 
{
  //U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  I2C_1306_C(0x0af);		                /* display on */
  //U8X8_END_TRANSFER(),             	/* disable chip */
  //U8X8_END()             			/* end of sequence */
};

void ssd1306_128x64_noname_powersave_on()
{
  //U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  I2C_1306_C(0x0ae);		                /* display off */
  //U8X8_END_TRANSFER(),             	/* disable chip */
  //U8X8_END()             			/* end of sequence */
};



// Set page address 0~15
void Set_Page_Address(unsigned char add)
{	
    unsigned char com[2];
    com[0]=0xb0|add;
    i2c_1306_write_command(com,1);

	return;
}


void Set_Column_Address(unsigned char add)
{
    unsigned char com[2];

	com[0]=(0x10|(add>>4));
    i2c_1306_write_command(com,1);

	com[0]=(0x0f&add);
    i2c_1306_write_command(com,1);

	return;
}


void Set_Contrast_Control_Register(unsigned char mod)
{
    unsigned char com[2];

	com[0]=0x81;
    i2c_1306_write_command(com,1);

	com[0]=mod;
    i2c_1306_write_command(com,1);

	return;
}



void Write_number(unsigned char *n,unsigned char k,unsigned char station_dot)
{
    unsigned char i; 
    unsigned char data[8];
    for(i=0;i<8;i++)
    {
        data[i]=(*(n+16*k+i));
    }
    i2c_1306_write_data(data,8);
   				
	Set_Page_Address(Start_page+1)	;
    Set_Column_Address(Start_column+station_dot*8);	
    for(i=8;i<16;i++)
    {
            data[i-8]=(*(n+16*k+i));
    }
    i2c_1306_write_data(data,8);
}

void display_dot(unsigned char start_col)
{
	Set_Column_Address(Start_column+start_col*8);
	Set_Page_Address(Start_page);
    i2c_1306_write_data(dot,8);    
}

void display_three_numbers(unsigned char number,unsigned char start_col)
{	unsigned char number1,number2,number3;
	number1=number/100;number2=number%100/10;number3=number%100%10;
	Set_Column_Address(Start_column+0*8+start_col*8);
	Set_Page_Address(Start_page);
    Write_number(num,number1,0+start_col);
	Set_Column_Address(Start_column+1*8+start_col*8);
	Set_Page_Address(Start_page);
	Write_number(num,number2,1+start_col);
	Set_Column_Address(Start_column+2*8+start_col*8);
	Set_Page_Address(Start_page);
	Write_number(num,number3,2+start_col);
}

#if 0
void Initial(void)
{
	//RST=1;
	//Delay(2000);
	//RST=0;
	//Delay(2000);
	//RST=1;

	//Delay(2000);

	Start();
	SentByte(Write_Address);
	SentByte(0x80);
	SentByte(0xae);//--turn off oled panel

	SentByte(0x80);
	SentByte(0xd5);//--set display clock divide ratio/oscillator frequency

	SentByte(0x80);
	SentByte(0x80);//--set divide ratio
	SentByte(0x80);
	SentByte(0xa8);//--set multiplex ratio(1 to 64)

	SentByte(0x80);
	SentByte(0x3f);//--1/64 duty

    Stop();
	Start();
	SentByte(Write_Address);


	SentByte(0x80);
	SentByte(0xd3);//-set display offset

	SentByte(0x80);
	SentByte(0x00);//-not offset

	SentByte(0x80);
	SentByte(0x8d);//--set Charge Pump enable/disable

	SentByte(0x80);
	SentByte(0x14);//--set(0x10) disable

	SentByte(0x80);
	SentByte(0x40);//--set start line address

    Stop();
	Start();
	SentByte(Write_Address);

	SentByte(0x80);
	SentByte(0xa6);//--set normal display

	SentByte(0x80);
	SentByte(0xa4);//Disable Entire Display On

	SentByte(0x80);
	SentByte(0xa1);//--set segment re-map 128 to 0

	SentByte(0x80);
	SentByte(0xC8);//--Set COM Output Scan Direction 64 to 0

    Stop();
	Start();
	SentByte(Write_Address);

	SentByte(0x80);
	SentByte(0xda);//--set com pins hardware configuration

	SentByte(0x80);
	SentByte(0x12);
       
    SentByte(0x80);
	SentByte(0x81);//--set contrast control register

	SentByte(0x80);
	SentByte(Contrast_level);

    Stop();
	Start();
	SentByte(Write_Address);

	SentByte(0x80);
	SentByte(0xd9);//--set pre-charge period

	SentByte(0x80);
	SentByte(0xf1);

	SentByte(0x80);
	SentByte(0xdb);//--set vcomh

	SentByte(0x80);
	SentByte(0x40);

    Stop();

	Start();
	SentByte(Write_Address);
	SentByte(0x80);
	SentByte(0xaf);//--turn on oled panel
 	Stop();

	// Turn on twice..
	Start();
	SentByte(Write_Address);
	SentByte(0x80);
	SentByte(0xaf);//--turn on oled panel
 	Stop();


}
#endif

void clear_display()
{
    int i=0;
    unsigned char data[8];
    Set_Page_Address(0);
    Set_Column_Address(0);
    for(i=0;i<8;i++) {
        data[i]=0;
    }
    for (i=0;i<128*8;i++) {
        i2c_1306_write_data(data,8);    
    }
}

void Write_data(unsigned char *data,unsigned char len){
    i2c_1306_write_data(data,len);        
}



#if 0
void Display_Chess(unsigned char value)
{
    unsigned char i,j,k;
    for(i=0;i<0x08;i++)
	{
		Set_Page_Address(i);
        Set_Column_Address(0x00);
		
		for(j=0;j<0x10;j++)
		{		
			Start();
		    SentByte(Write_Address);
		    SentByte(0x40);


		    for(k=0;k<0x04;k++)
				SentByte(value);
		    for(k=0;k<0x04;k++)
				SentByte(~value);

			Stop();		
		}
	}
    return;
}
#endif