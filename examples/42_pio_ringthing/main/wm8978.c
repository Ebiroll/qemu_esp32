#include "wm8978.h"
#include "hal_i2c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hal_i2s.h"


#define bit0  0x001
#define bit1  0x002
#define bit2  0x004 
#define bit3  0x008 
#define bit4  0x010 
#define bit5  0x020 
#define bit6  0x040 
#define bit7  0x080 
#define bit8  0x100
 	


//WM8978?Ĵ??ֵ?????(???58???Ĵ??,0~57),ռ?116???ڴ?
//?ΪWM8978??IC?????֧?ֶ????,??????ر??????Ĵ??ֵ
//дWM8978?Ĵ??ʱ,ͬ??????????ؼĴ??ֵ,???Ĵ??ʱ,ֱ?ӷ??ر??ر????Ĵ??ֵ.
//ע?:WM8978?ļĴ??ֵ?9λ????Ҫ?uint16_t???洢. 
static uint16_t WM8978_REGVAL_TBL[58]=
{
	0X0000,0X0000,0X0000,0X0000,0X0050,0X0000,0X0140,0X0000,
	0X0000,0X0000,0X0000,0X00FF,0X00FF,0X0000,0X0100,0X00FF,
	0X00FF,0X0000,0X012C,0X002C,0X002C,0X002C,0X002C,0X0000,
	0X0032,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,
	0X0038,0X000B,0X0032,0X0000,0X0008,0X000C,0X0093,0X00E9,
	0X0000,0X0000,0X0000,0X0000,0X0003,0X0010,0X0010,0X0100,
	0X0100,0X0002,0X0001,0X0001,0X0039,0X0039,0X0039,0X0039,
	0X0001,0X0001
}; 

static void wm8979_interface()
{

	//WM8978_REGVAL_TBL[WM8978_AUDIO_INTERFACE]|=bit0;//mono left phase
	WM8978_REGVAL_TBL[WM8978_AUDIO_INTERFACE] &=~(bit6|bit5);//16bit
	//WM8978_REGVAL_TBL[WM8978_AUDIO_INTERFACE] |=bit6;//16bit
	//WM8978_REGVAL_TBL[WM8978_AUDIO_INTERFACE]|=(bit6|bit5);//32bit
	WM8978_Write_Reg(WM8978_AUDIO_INTERFACE,WM8978_REGVAL_TBL[WM8978_AUDIO_INTERFACE]);
	//WM8978_REGVAL_TBL[WM8978_CLOCKING]|=bit0; //the codec ic is master mode 
	//WM8978_REGVAL_TBL[WM8978_CLOCKING]|=bit3|bit2;// 100 256/16=16; 16 bit
	WM8978_REGVAL_TBL[WM8978_CLOCKING]&=~(bit7|bit6|bit5);//000 
	//WM8978_REGVAL_TBL[WM8978_CLOCKING]|=(bit7|bit5);// 101 48/6=8
	WM8978_REGVAL_TBL[WM8978_CLOCKING]&=~bit8;//mclk
	WM8978_Write_Reg(WM8978_CLOCKING,WM8978_REGVAL_TBL[WM8978_CLOCKING]);
	//WM8978_REGVAL_TBL[WM8978_CLOCKING]&=~bit8;//mclk is the clk source
}
static void wm8979_pll(uint32_t k,uint8_t n)
{
	WM8978_REGVAL_TBL[WM8978_ADDITIONAL_CONTROL]&=~(bit3|bit2|bit1);
	WM8978_REGVAL_TBL[WM8978_ADDITIONAL_CONTROL]|=(bit3|bit1);
	WM8978_Write_Reg(WM8978_ADDITIONAL_CONTROL,WM8978_REGVAL_TBL[WM8978_ADDITIONAL_CONTROL]);//sr 8K

	WM8978_REGVAL_TBL[WM8978_POWER_MANAGEMENT_1]|=bit5;//enable pll
	WM8978_Write_Reg(WM8978_POWER_MANAGEMENT_1,WM8978_REGVAL_TBL[WM8978_POWER_MANAGEMENT_1]);
	WM8978_REGVAL_TBL[WM8978_PLL_N]|=bit4;//prescale enable /2
	WM8978_REGVAL_TBL[WM8978_PLL_N]&=0x1f0;
	WM8978_REGVAL_TBL[WM8978_PLL_N]|=n;//7
	WM8978_Write_Reg(WM8978_PLL_N,WM8978_REGVAL_TBL[WM8978_PLL_N]);
	//k=EE009F
	WM8978_REGVAL_TBL[WM8978_PLL_K1]=(k>>18);
	WM8978_Write_Reg(WM8978_PLL_K1,WM8978_REGVAL_TBL[WM8978_PLL_K1]);
	WM8978_REGVAL_TBL[WM8978_PLL_K2]=(k>>9);
	WM8978_Write_Reg(WM8978_PLL_K2,WM8978_REGVAL_TBL[WM8978_PLL_K2]);
	WM8978_REGVAL_TBL[WM8978_PLL_K3]=k;					
	WM8978_Write_Reg(WM8978_PLL_K3,WM8978_REGVAL_TBL[WM8978_PLL_K3]);
}
static void wm8979_loopback()
{
	WM8978_REGVAL_TBL[WM8978_COMPANDING_CONTROL]|=bit0; //start loopback
	WM8978_Write_Reg(WM8978_COMPANDING_CONTROL,WM8978_REGVAL_TBL[WM8978_COMPANDING_CONTROL]);

}
//WM8978 init
//返回值:0,初始化正常
//    其他,错误代码
uint8_t WM8978_Init(void)
{
	//??Ϊͨ???
	//wm8979_pll(0X3126E9,0x08);
	wm8979_interface();
	WM8978_Write_Reg(1,0X3B);	//R1,MICEN??Ϊ1(MICʹ?),BIASEN??Ϊ1(ģ?????),VMIDSEL[1:0]??Ϊ:11(5K)
	WM8978_Write_Reg(2,0X1B0);	//R2,ROUT1,LOUT1???ʹ?(??????Թ??),BOOSTENR,BOOSTENLʹ?
	WM8978_Write_Reg(3,0X6C);	//R3,LOUT2,ROUT2???ʹ?(???ȹ??),RMIX,LMIXʹ?	//0x6c
	//WM8978_Write_Reg(6,0);		//R6,MCLK???????
	WM8978_Write_Reg(43,1<<4);	//R43,INVROUT2???,???????
	WM8978_Write_Reg(47,1<<8);	//R47??,PGABOOSTL,?ͨ??MIC??20????
	WM8978_Write_Reg(48,1<<8);	//R48??,PGABOOSTR,?ͨ??MIC??20????
	WM8978_Write_Reg(49,1<<1);	//R49,TSDEN,?????????? 
	WM8978_Write_Reg(10,1<<3);	//R10,SOFTMUTE?ر?128x???,???NR 
	WM8978_Write_Reg(14,1<<3);	//R14,ADC 128x????
	//wm8979_loopback();
	return 0;
} 
//WM8978写寄存器
//reg:寄存器地址
//val:要写入寄存器的值 
//返回值:0,成功;
//其他,错误代码
uint8_t WM8978_Write_Reg(uint8_t reg,uint16_t val)
{ 
	uint8_t buf[2];
	buf[0]=((val&0x0100)>>8)|(reg<<1);
	buf[1]=(uint8_t)(val&0xff);
	hal_i2c_master_mem_write(0,WM8978_ADDR,buf[0],buf+1,1);
	WM8978_REGVAL_TBL[reg]=val;
	return 0;	
}  

// WM8978 read register
// Reads the value  of the local register buffer zone
// reg: Register Address
// Return Value: Register value
uint16_t WM8978_Read_Reg(uint8_t reg)
{  
	return WM8978_REGVAL_TBL[reg];	
} 
//WM8978 DAC/ADC配置
//adcen:adc使能(1)/关闭(0)
//dacen:dac使能(1)/关闭(0)

void WM8978_ADDA_Cfg(uint8_t dacen,uint8_t adcen)
{
	uint16_t regval;
	regval=WM8978_Read_Reg(3);	//??ȡR3
	if(dacen)regval|=3<<0;		//R3?????λ??Ϊ1,???DACR&DACL
	else regval&=~(3<<0);		//R3?????λ????ر?ACR&DACL.
	WM8978_Write_Reg(3,regval);	//??R3
	regval=WM8978_Read_Reg(2);	//??ȡR2
	if(adcen)regval|=3<<0;		//R2?????λ??Ϊ1,???ADCR&ADCL
	else regval&=~(3<<0);		//R2?????λ????ر?DCR&ADCL.
	WM8978_Write_Reg(2,regval);	//??R2	
}
//WM8978 输入通道配置 
//micen:MIC开启(1)/关闭(0)
//lineinen:Line In开启(1)/关闭(0)
//auxen:aux开启(1)/关闭(0) 
void WM8978_Input_Cfg(uint8_t micen,uint8_t lineinen,uint8_t auxen)
{
	uint16_t regval;  
	regval=WM8978_Read_Reg(2);	//??ȡR2
	if(micen)regval|=3<<2;		//???INPPGAENR,INPPGAENL(MIC??GA?Ŵ?
	else regval&=~(3<<2);		//?ر?NPPGAENR,INPPGAENL.
 	WM8978_Write_Reg(2,regval);	//??R2 
	
	regval=WM8978_Read_Reg(44);	//??ȡR44
	if(micen)regval|=3<<4|3<<0;	//???LIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
	else regval&=~(3<<4|3<<0);	//?ر?IN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
	WM8978_Write_Reg(44,regval);//??R44
	
	if(lineinen)WM8978_LINEIN_Gain(5);//LINE IN 0dB??
	else WM8978_LINEIN_Gain(0);	//?ر?INE IN
	if(auxen)WM8978_AUX_Gain(7);//AUX 6dB??
	else WM8978_AUX_Gain(0);	//?ر?UX??  
}
//WM8978 输出配置 
//dacen:DAC输出(放音)开启(1)/关闭(0)
//bpsen:Bypass输出(录音,包括MIC,LINE IN,AUX等)开启(1)/关闭(0) 
void WM8978_Output_Cfg(uint8_t dacen,uint8_t bpsen)
{
	uint16_t regval=0;
	if(dacen)regval|=1<<0;	//DAC???ʹ?
	if(bpsen)
	{
		regval|=1<<1;		//BYPASSʹ?
		regval|=5<<2;		//0dB??
	} 
	WM8978_Write_Reg(50,regval);//R50??
	WM8978_Write_Reg(51,regval);//R51?? 
}
//WM8978 MIC增益设置(不包括BOOST的20dB,MIC-->ADC输入部分的增益)
//gain:0~63,对应-12dB~35.25dB,0.75dB/Step
void WM8978_MIC_Gain(uint8_t gain)
{
	gain&=0X3F;
	WM8978_Write_Reg(45,gain);		//R45,?ͨ??PGA?? 
	WM8978_Write_Reg(46,gain|1<<8);	//R46,?ͨ??PGA??
}
//WM8978 L2/R2(也就是Line In)增益设置(L2/R2-->ADC输入部分的增益)
//gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_LINEIN_Gain(uint8_t gain)
{
	uint16_t regval;
	gain&=0X07;
	regval=WM8978_Read_Reg(47);	//??ȡR47
	regval&=~(7<<4);			//???ԭ??????
 	WM8978_Write_Reg(47,regval|gain<<4);//??R47
	regval=WM8978_Read_Reg(48);	//??ȡR48
	regval&=~(7<<4);			//???ԭ??????
 	WM8978_Write_Reg(48,regval|gain<<4);//??R48
} 

//WM8978 AUXR,AUXL(PWM音频部分)增益设置(AUXR/L-->ADC输入部分的增益)
//gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_AUX_Gain(uint8_t gain)
{
	uint16_t regval;
	gain&=0X07;
	regval=WM8978_Read_Reg(47);	//??ȡR47
	regval&=~(7<<0);			//???ԭ??????
 	WM8978_Write_Reg(47,regval|gain<<0);//??R47
	regval=WM8978_Read_Reg(48);	//??ȡR48
	regval&=~(7<<0);			//???ԭ??????
 	WM8978_Write_Reg(48,regval|gain<<0);//??R48
}  
//设置I2S工作模式
//fmt:0,LSB(右对齐);1,MSB(左对齐);2,飞利浦标准I2S;3,PCM/DSP;
//len:0,16位;1,20位;2,24位;3,32位;  
void WM8978_I2S_Cfg(uint8_t fmt,uint8_t len)
{
	fmt&=0X03;
	len&=0X03;//?????Χ
	WM8978_Write_Reg(4,(fmt<<3)|(len<<5));	//R4,WM8978???ģʽ??	
}	

//设置耳机左右声道音量
//voll:左声道音量(0~63)
//volr:右声道音量(0~63)
void WM8978_HPvol_Set(uint8_t voll,uint8_t volr)
{
	voll&=0X3F;
	volr&=0X3F;//?????Χ
	if(voll==0)voll|=1<<6;//???Ϊ0ʱ,ֱ??ute
	if(volr==0)volr|=1<<6;//???Ϊ0ʱ,ֱ??ute 
	WM8978_Write_Reg(52,voll);			//R52,?????????????
	WM8978_Write_Reg(53,volr|(1<<8));	//R53,?????????????,ͬ?????(HPVU=1)
}
//设置喇叭音量
//voll:左声道音量(0~63) 
void WM8978_SPKvol_Set(uint8_t volx)
{ 
	volx&=0X3F;//?????Χ
	if(volx==0)volx|=1<<6;//???Ϊ0ʱ,ֱ??ute 
 	WM8978_Write_Reg(54,volx);			//R54,?????????????
	WM8978_Write_Reg(55,volx|(1<<8));	//R55,?????????????,ͬ?????(SPKVU=1)	
}
//设置3D环绕声
//depth:0~15(3D强度,0最弱,15最强)
void WM8978_3D_Set(uint8_t depth)
{ 
	depth&=0XF;//?????Χ 
 	WM8978_Write_Reg(41,depth);	//R41,3D????? 	
}
//设置EQ/3D作用方向
//dir:0,在ADC起作用
//    1,在DAC起作用(默认)
void WM8978_EQ_3D_Dir(uint8_t dir)
{
	uint16_t regval; 
	regval=WM8978_Read_Reg(0X12);
	if(dir)regval|=1<<8;
	else regval&=~(1<<8); 
 	WM8978_Write_Reg(18,regval);//R18,EQ1?ĵ?λ???Q/3D???
}

//设置EQ1
//cfreq:截止频率,0~3,分别对应:80/105/135/175Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ1_Set(uint8_t cfreq,uint8_t gain)
{ 
	uint16_t regval;
	cfreq&=0X3;//?????Χ 
	if(gain>24)gain=24;
	gain=24-gain;
	regval=WM8978_Read_Reg(18);
	regval&=0X100;
	regval|=cfreq<<5;	//?????Ƶ? 
	regval|=gain;		//????	
 	WM8978_Write_Reg(18,regval);//R18,EQ1?? 	
}
//设置EQ2
//cfreq:中心频率,0~3,分别对应:230/300/385/500Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ2_Set(uint8_t cfreq,uint8_t gain)
{ 
	uint16_t regval=0;
	cfreq&=0X3;//?????Χ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//?????Ƶ? 
	regval|=gain;		//????	
 	WM8978_Write_Reg(19,regval);//R19,EQ2?? 	
}
//设置EQ3
//cfreq:中心频率,0~3,分别对应:650/850/1100/1400Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ3_Set(uint8_t cfreq,uint8_t gain)
{ 
	uint16_t regval=0;
	cfreq&=0X3;//?????Χ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//?????Ƶ? 
	regval|=gain;		//????	
 	WM8978_Write_Reg(20,regval);//R20,EQ3?? 	
}
//设置EQ4
//cfreq:中心频率,0~3,分别对应:1800/2400/3200/4100Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ4_Set(uint8_t cfreq,uint8_t gain)
{ 
	uint16_t regval=0;
	cfreq&=0X3;//?????Χ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//?????Ƶ? 
	regval|=gain;		//????	
 	WM8978_Write_Reg(21,regval);//R21,EQ4?? 	
}
//设置EQ5
//cfreq:中心频率,0~3,分别对应:5300/6900/9000/11700Hz
//gain:增益,0~24,对应-12~+12dB
void WM8978_EQ5_Set(uint8_t cfreq,uint8_t gain)
{ 
	uint16_t regval=0;
	cfreq&=0X3;//?????Χ 
	if(gain>24)gain=24;
	gain=24-gain; 
	regval|=cfreq<<5;	//?????Ƶ? 
	regval|=gain;		//????	
 	WM8978_Write_Reg(22,regval);//R22,EQ5?? 	
}












