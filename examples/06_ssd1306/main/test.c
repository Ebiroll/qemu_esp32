
#include <stdlib.h>
#include "esp32-hal-i2c.h"

unsigned char Contrast_level=0x8F;

#define Write_Address 0x78/*slave addresses with write*/
#define Read_Address 0x79/*slave addresses with read*/

#define Start_column	0x00
#define Start_page		0x00
#define	StartLine_set	0x00

#include <stdio.h>
#include "Arduino.h"


unsigned char 	    num[]={0x00,0xF8,0xFC,0x04,0x04,0xFC,0xF8,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,  	   /*--    0-9  --*/
0x00,0x00,0x08,0xFC,0xFC,0x00,0x00,0x00,0x00,0x00,0x04,0x07,0x07,0x04,0x00,0x00,  
0x00,0x18,0x1C,0x84,0xC4,0x7C,0x38,0x00,0x00,0x06,0x07,0x05,0x04,0x04,0x04,0x00,
0x00,0x08,0x0C,0x24,0x24,0xFC,0xD8,0x00,0x00,0x02,0x06,0x04,0x04,0x07,0x03,0x00,
0x80,0xE0,0x70,0x18,0xFC,0xFC,0x00,0x00,0x00,0x01,0x01,0x05,0x07,0x07,0x04,0x00,
0x00,0x7C,0x7C,0x24,0x24,0xE4,0xC4,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,
0x00,0xF0,0xF8,0x6C,0x24,0xEC,0xCC,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,
0x00,0x0C,0x0C,0xC4,0xFC,0x3C,0x04,0x00,0x00,0x00,0x00,0x07,0x07,0x00,0x00,0x00,
0x00,0x98,0xFC,0x64,0x64,0xFC,0x98,0x00,0x00,0x03,0x07,0x04,0x04,0x07,0x03,0x00,
0x00,0x78,0xFC,0x84,0xC4,0xFC,0xF8,0x00,0x00,0x06,0x06,0x04,0x06,0x03,0x01,0x00};


i2c_t* my_i2c=NULL;
int bytes_in_fifo=0;

// This must be called first
void set_i2c(i2c_t *i2c) {
	my_i2c=i2c;
}


// Start condition
void Start(void)
{
	i2cResetFiFo(my_i2c);
    i2cResetCmd(my_i2c);
	bytes_in_fifo=0;

    //CMD START
    i2cSetCmd(my_i2c, 0, I2C_CMD_RSTART, 0, false, false, false);

}

// Stop condition
void Stop(void)
{
	i2cSetCmd(my_i2c, 1, I2C_CMD_WRITE, bytes_in_fifo, false, false, true);
    i2cSetCmd(my_i2c, 2, I2C_CMD_STOP, 0, false, false, false);


    my_i2c->dev->ctr.trans_start = 1;
	bytes_in_fifo=0;


	uint32_t startAt = millis();
	while(!my_i2c->dev->command[2].done) {
		// BUSY WAIT UNTIL COMMAND DONE
		if((millis() - startAt)>200){
			//printf("Timeout! Addr: %x", address >> 1);
			printf("Stop Timeout, not finished!\n");
			return;
		}
	}


}

// Stop condition
void SentByte(unsigned char data) 
{
   // ONLY 32 bytes in fifo 
   if (bytes_in_fifo>=31) 
   {
	   	printf("i2c sending more than 32 bytes!\n");
	   	i2cSetCmd(my_i2c, 1, I2C_CMD_WRITE, bytes_in_fifo, false, false, true);
        i2cSetCmd(my_i2c, 2, I2C_CMD_END, 0, false, false, false);

        //START Transmission
        my_i2c->dev->ctr.trans_start = 1;
		uint32_t startAt = millis();
		while(!my_i2c->dev->command[2].done) {
			// BUSY WAIT UNTIL COMMAND DONE
            if((millis() - startAt)>2000){
                //printf("Timeout! Addr: %x", address >> 1);
				printf("Test Timeout, never done!\n");
                return;
            }
		}
		bytes_in_fifo=0;
   }
   my_i2c->dev->fifo_data.val=data;
   bytes_in_fifo++;
}

// Set page address 0~15
void Set_Page_Address(unsigned char add)
{	
	Start();
	SentByte(Write_Address);
	SentByte(0x80);
         add=0xb0|add;
	SentByte(add);
 	Stop();
    //_nop_();
	return;
}


void Set_Column_Address(unsigned char add)
{	Start();
	SentByte(Write_Address);
	SentByte(0x80);
	SentByte((0x10|(add>>4)));
	SentByte(0x80);
	SentByte((0x0f&add));
 	Stop();
	return;
}


void Set_Contrast_Control_Register(unsigned char mod)
{
	Start();
	SentByte(Write_Address);
	SentByte(0x80);
	SentByte(0x81);
	SentByte(0x80);
	SentByte(mod);
 	Stop();
	return;
}



void Write_number(unsigned char *n,unsigned char k,unsigned char station_dot)
{
	    unsigned char i; 
        Start();
        SentByte(Write_Address);
        SentByte(0x40);
	  for(i=0;i<8;i++)
	  {
	    SentByte(*(n+16*k+i));
	  }
      Stop();
				

	Set_Page_Address(Start_page+1)	;
        Set_Column_Address(Start_column+station_dot*8);	
			Start();
			SentByte(Write_Address);
			SentByte(0x40);
			for(i=8;i<16;i++)
				{
				SentByte(*(n+16*k+i));
				}
			Stop();

}

void display_three_numbers(unsigned char number)
{	unsigned char number1,number2,number3;
	number1=number/100;number2=number%100/10;number3=number%100%10;
	Set_Column_Address(Start_column+0*8);
	Set_Page_Address(Start_page);
         Write_number(num,number1,0);
	Set_Column_Address(Start_column+1*8);
	Set_Page_Address(Start_page);
	Write_number(num,number2,1);
	Set_Column_Address(Start_column+2*8);
	Set_Page_Address(Start_page);
	Write_number(num,number3,2);

}


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
