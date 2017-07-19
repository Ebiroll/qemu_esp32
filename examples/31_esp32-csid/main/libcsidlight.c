// Based of cSID light - an attempt at a usable simple API

// cSID by Hermit (Mihaly Horvath), (Year 2017) http://hermit.sidrip.com
// (based on jsSID, this version has much lower CPU-usage, as mainloop runs at samplerate)
// License: WTF - Do what the fuck you want with this code, but please mention me as its original author.

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libcsid.h>

typedef unsigned char byte;
typedef unsigned char Uint8;

//global constants and variables
#define C64_PAL_CPUCLK 985248.0
#define SID_CHANNEL_AMOUNT 3
#define MAX_FILENAME_LEN 512
#define MAX_DATA_LEN 65536
#define PAL_FRAMERATE 50.06 //50.0443427 //50.1245419 //(C64_PAL_CPUCLK/63/312.5), selected carefully otherwise some ADSR-sensitive tunes may suffer more:
#define DEFAULT_SAMPLERATE 44100.0         //(Soldier of Fortune, 2nd Reality, Alliance, X-tra energy, Jackal, Sanxion, Ultravox, Hard Track, Swing, Myth, LN3, etc.)
#define CLOCK_RATIO_DEFAULT C64_PAL_CPUCLK/DEFAULT_SAMPLERATE  //(50.0567520: lowest framerate where Sanxion is fine, and highest where Myth is almost fine)
#define VCR_SHUNT_6581 1500 //kOhm //cca 1.5 MOhm Rshunt across VCR FET drain and source (causing 220Hz bottom cutoff with 470pF integrator capacitors in old C64)
#define VCR_FET_TRESHOLD 192 //Vth (on cutoff numeric range 0..2048) for the VCR cutoff-frequency control FET below which it doesn't conduct
#define CAP_6581 0.470 //nF //filter capacitor value for 6581
#define FILTER_DARKNESS_6581 22.0 //the bigger the value, the darker the filter control is (that is, cutoff frequency increases less with the same cutoff-value)
#define FILTER_DISTORTION_6581 0.0016 //the bigger the value the more of resistance-modulation (filter distortion) is applied for 6581 cutoff-control

int OUTPUT_SCALEDOWN = SID_CHANNEL_AMOUNT * 16 + 26;
// //raw output divided by this after multiplied by main volume, this also compensates for filter-resonance emphasis to avoid distotion

enum { GATE_BITMASK=0x01, SYNC_BITMASK=0x02, RING_BITMASK=0x04, TEST_BITMASK=0x08,
       TRI_BITMASK=0x10, SAW_BITMASK=0x20, PULSE_BITMASK=0x40, NOISE_BITMASK=0x80,
       HOLDZERO_BITMASK=0x10, DECAYSUSTAIN_BITMASK=0x40, ATTACK_BITMASK=0x80,
       LOWPASS_BITMASK=0x10, BANDPASS_BITMASK=0x20, HIGHPASS_BITMASK=0x40, OFF3_BITMASK=0x80 };

float clock_ratio=CLOCK_RATIO_DEFAULT;

//SID-emulation variables:
const byte FILTSW[9] = {1,2,4,1,2,4,1,2,4};
byte ADSRstate[9], expcnt[9], prevSR[9], sourceMSBrise[9];
short int envcnt[9];
unsigned int prevwfout[9], prevwavdata[9], sourceMSB[3], noise_LFSR[9];
int phaseaccu[9], prevaccu[9], prevlowpass[3], prevbandpass[3];;
float ratecnt[9], cutoff_ratio_8580, cutoff_steepness_6581, cap_6581_reciprocal; //, cutoff_ratio_6581, cutoff_bottom_6581, cutoff_top_6581;
//player-related variables:
int SIDamount=1, SID_model[3]={8580,8580,8580}, requested_SID_model=-1, sampleratio;
byte *filedata, *memory, timermode[0x20], SIDtitle[0x20], SIDauthor[0x20], SIDinfo[0x20];
int subtune=0, tunelength=-1, default_tunelength=300, minutes=-1, seconds=-1;
unsigned int initaddr, playaddr, playaddf, SID_address[3]={0xD400,0,0};
int samplerate = DEFAULT_SAMPLERATE;
float framecnt=0, frame_sampleperiod = DEFAULT_SAMPLERATE/PAL_FRAMERATE;
//CPU (and CIA/VIC-IRQ) emulation constants and variables - avoiding internal/automatic variables to retain speed
const byte flagsw[]={0x01,0x21,0x04,0x24,0x00,0x40,0x08,0x28}, branchflag[]={0x80,0x40,0x01,0x02};
unsigned int PC=0, pPC=0, addr=0, storadd=0;
short int A=0, T=0, SP=0xFF;
byte X=0, Y=0, IR=0, ST=0x00;  //STATUS-flags: N V - B D I Z C
float CPUtime=0.0;
unsigned char cycles=0, finished=0, dynCIA=0;

//function prototypes
void cSID_init(int samplerate);
int SID(unsigned char num, unsigned int baseaddr);
void initSID();
void initCPU (unsigned int mempos);
byte CPU ();
void init (byte subtune);  void play(void* userdata, Uint8 *stream, int len );
unsigned int combinedWF(unsigned char num, unsigned char channel, unsigned int* wfarray, int index, unsigned char differ6581, byte freq);
void createCombinedWF(unsigned int* wfarray, float bitmul, float bitstrength, float treshold);



//----------------------------- MAIN thread ----------------------------


void init (byte subt)
{
  static int timeout; subtune = subt; initCPU(initaddr); initSID(); A=subtune; memory[1]=0x37; memory[0xDC05]=0;
  for(timeout=100000;timeout>=0;timeout--) { if (CPU()) break; } 
  if (timermode[subtune] || memory[0xDC05]) { //&& playaddf {   //CIA timing
  if (!memory[0xDC05]) {memory[0xDC04]=0x24; memory[0xDC05]=0x40;} //C64 startup-default
  frame_sampleperiod = (memory[0xDC04]+memory[0xDC05]*256)/clock_ratio; }
  else frame_sampleperiod = samplerate/PAL_FRAMERATE;  //Vsync timing
  printf("Frame-sampleperiod: %.0f samples  (%.1fX speed)\n", round(frame_sampleperiod), samplerate/PAL_FRAMERATE/frame_sampleperiod); 
  //frame_sampleperiod = (memory[0xDC05]!=0 || (!timermode[subtune] && playaddf))? samplerate/PAL_FRAMERATE : (memory[0xDC04] + memory[0xDC05]*256) / clock_ratio; 
  if(playaddf==0) { playaddr = ((memory[1]&3)<2)? memory[0xFFFE]+memory[0xFFFF]*256 : memory[0x314]+memory[0x315]*256; printf("IRQ-playaddress:%4.4X\n",playaddr); }
  else { playaddr=playaddf; if (playaddr>=0xE000 && memory[1]==0x37) memory[1]=0x35; } //player under KERNAL (Crystal Kingdom Dizzy)
  initCPU(playaddr); framecnt=1; finished=0; CPUtime=0; 
}


void play(void* userdata, Uint8 *stream, int len ) //called by SDL at samplerate pace
{ 
  static int i,j, output;
  for(i=0;i<len;i+=2) {
    framecnt--; if (framecnt<=0) { framecnt=frame_sampleperiod; finished=0; PC=playaddr; SP=0xFF; } // printf("%d  %f\n",framecnt,playtime); }
    if (finished==0) { 
      while (CPUtime<=clock_ratio) { 
        pPC=PC; if (CPU()>=0xFE || ( (memory[1]&3)>1 && pPC<0xE000 && (PC==0xEA31 || PC==0xEA81) ) ) {finished=1;break;} else CPUtime+=cycles; //RTS,RTI and IRQ player ROM return handling
        if ( (addr==0xDC05 || addr==0xDC04) && (memory[1]&3) && timermode[subtune] ) {
          frame_sampleperiod = (memory[0xDC04] + memory[0xDC05]*256) / clock_ratio;  //dynamic CIA-setting (Galway/Rubicon workaround)
          if (!dynCIA) {dynCIA=1; printf("( Dynamic CIA settings. New frame-sampleperiod: %.0f samples  (%.1fX speed) )\n", round(frame_sampleperiod), samplerate/PAL_FRAMERATE/frame_sampleperiod);}
        }
        if(storadd>=0xD420 && storadd<0xD800 && (memory[1]&3)) {  //CJ in the USA workaround (writing above $d420, except SID2/SID3)
          if ( !(SID_address[1]<=storadd && storadd<SID_address[1]+0x1F) && !(SID_address[2]<=storadd && storadd<SID_address[2]+0x1F) )
            memory[storadd&0xD41F]=memory[storadd]; //write to $D400..D41F if not in SID2/SID3 address-space
        }
        if(addr==0xD404 && !(memory[0xD404]&GATE_BITMASK)) ADSRstate[0]&=0x3E; //Whittaker player workarounds (if GATE-bit triggered too fast, 0 for some cycles then 1)
        if(addr==0xD40B && !(memory[0xD40B]&GATE_BITMASK)) ADSRstate[1]&=0x3E;
        if(addr==0xD412 && !(memory[0xD412]&GATE_BITMASK)) ADSRstate[2]&=0x3E;
      }
      CPUtime-=clock_ratio;
    }
    output = SID(0,0xD400);
    if (SIDamount>=2) output += SID(1,SID_address[1]); 
    if (SIDamount==3) output += SID(2,SID_address[2]); 
    stream[i]=output&0xFF; stream[i+1]=output>>8;
  }
}



// //--------------------------------- CPU emulation -------------------------------------------


 void initCPU (unsigned int mempos) {
   PC = mempos;
   A = 0;
   X = 0;
   Y = 0;
   ST = 0;
   SP = 0xFF;
 } 

 //My CPU implementation is based on the instruction table by Graham at codebase64.
 //After some examination of the table it was clearly seen that columns of the table (instructions' 2nd nybbles)
 // mainly correspond to addressing modes, and double-rows usually have the same instructions.
 //The code below is laid out like this, with some exceptions present.
 //Thanks to the hardware being in my mind when coding this, the illegal instructions could be added fairly easily...
 byte CPU () //the CPU emulation for SID/PRG playback (ToDo: CIA/VIC-IRQ/NMI/RESET vectors, BCD-mode)
 { //'IR' is the instruction-register, naming after the hardware-equivalent
  IR=memory[PC]; cycles=2; storadd=0; //'cycle': ensure smallest 6510 runtime (for implied/register instructions)
  if(IR&1) {  //nybble2:  1/5/9/D:accu.instructions, 3/7/B/F:illegal opcodes
   switch (IR&0x1F) { //addressing modes (begin with more complex cases), PC wraparound not handled inside to save codespace
    case 1: case 3: PC++; addr = memory[memory[PC]+X] + memory[memory[PC]+X+1]*256; cycles=6; break; //(zp,x)
    case 0x11: case 0x13: PC++; addr = memory[memory[PC]] + memory[memory[PC]+1]*256 + Y; cycles=6; break; //(zp),y (5..6 cycles, 8 for R-M-W)
    case 0x19: case 0x1B: PC++; addr=memory[PC]; PC++; addr+=memory[PC]*256 + Y; cycles=5; break; //abs,y //(4..5 cycles, 7 cycles for R-M-W)
    case 0x1D: PC++; addr=memory[PC]; PC++; addr+=memory[PC]*256 + X; cycles=5; break; //abs,x //(4..5 cycles, 7 cycles for R-M-W)
    case 0xD: case 0xF: PC++; addr=memory[PC]; PC++; addr+=memory[PC]*256; cycles=4; break; //abs
    case 0x15: PC++; addr = memory[PC] + X; cycles=4; break; //zp,x
    case 5: case 7: PC++; addr = memory[PC]; cycles=3; break; //zp
    case 0x17: PC++; if ((IR&0xC0)!=0x80) { addr = memory[PC] + X; cycles=4; } //zp,x for illegal opcodes
               else { addr = memory[PC] + Y; cycles=4; }  break; //zp,y for LAX/SAX illegal opcodes
    case 0x1F: PC++; if ((IR&0xC0)!=0x80) { addr = memory[PC] + memory[++PC]*256 + X; cycles=5; } //abs,x for illegal opcodes
               else { addr = memory[PC] + memory[++PC]*256 + Y; cycles=5; }  break; //abs,y for LAX/SAX illegal opcodes
    case 9: case 0xB: PC++; addr = PC; cycles=2;  //immediate
   }
   addr&=0xFFFF;
   switch (IR&0xE0) {
    case 0x60: if ((IR&0x1F)!=0xB) { if((IR&3)==3) {T=(memory[addr]>>1)+(ST&1)*128; ST&=124; ST|=(T&1); memory[addr]=T; cycles+=2;}   //ADC / RRA (ROR+ADC)
                T=A; A+=memory[addr]+(ST&1); ST&=60; ST|=(A&128)|(A>255); A&=0xFF; ST |= (!A)<<1 | ( !((T^memory[addr])&0x80) & ((T^A)&0x80) ) >> 1; }
               else { A&=memory[addr]; T+=memory[addr]+(ST&1); ST&=60; ST |= (T>255) | ( !((A^memory[addr])&0x80) & ((T^A)&0x80) ) >> 1; //V-flag set by intermediate ADC mechanism: (A&mem)+mem
                T=A; A=(A>>1)+(ST&1)*128; ST|=(A&128)|(T>127); ST|=(!A)<<1; }  break; // ARR (AND+ROR, bit0 not going to C, but C and bit7 get exchanged.)
    case 0xE0: if((IR&3)==3 && (IR&0x1F)!=0xB) {memory[addr]++;cycles+=2;}  T=A; A-=memory[addr]+!(ST&1); //SBC / ISC(ISB)=INC+SBC
               ST&=60; ST|=(A&128)|(A>=0); A&=0xFF; ST |= (!A)<<1 | ( ((T^memory[addr])&0x80) & ((T^A)&0x80) ) >> 1; break; 
    case 0xC0: if((IR&0x1F)!=0xB) { if ((IR&3)==3) {memory[addr]--; cycles+=2;}  T=A-memory[addr]; } // CMP / DCP(DEC+CMP)
               else {X=T=(A&X)-memory[addr];} /*SBX(AXS)*/  ST&=124;ST|=(!(T&0xFF))<<1|(T&128)|(T>=0);  break;  //SBX (AXS) (CMP+DEX at the same time)
    case 0x00: if ((IR&0x1F)!=0xB) { if ((IR&3)==3) {ST&=124; ST|=(memory[addr]>127); memory[addr]<<=1; cycles+=2;}  
                A|=memory[addr]; ST&=125;ST|=(!A)<<1|(A&128); } //ORA / SLO(ASO)=ASL+ORA
               else {A&=memory[addr]; ST&=124;ST|=(!A)<<1|(A&128)|(A>127);}  break; //ANC (AND+Carry=bit7)
    case 0x20: if ((IR&0x1F)!=0xB) { if ((IR&3)==3) {T=(memory[addr]<<1)+(ST&1); ST&=124; ST|=(T>255); T&=0xFF; memory[addr]=T; cycles+=2;}  
                A&=memory[addr]; ST&=125; ST|=(!A)<<1|(A&128); }  //AND / RLA (ROL+AND)
               else {A&=memory[addr]; ST&=124;ST|=(!A)<<1|(A&128)|(A>127);}  break; //ANC (AND+Carry=bit7)
    case 0x40: if ((IR&0x1F)!=0xB) { if ((IR&3)==3) {ST&=124; ST|=(memory[addr]&1); memory[addr]>>=1; cycles+=2;}
                A^=memory[addr]; ST&=125;ST|=(!A)<<1|(A&128); } //EOR / SRE(LSE)=LSR+EOR
                else {A&=memory[addr]; ST&=124; ST|=(A&1); A>>=1; A&=0xFF; ST|=(A&128)|((!A)<<1); }  break; //ALR(ASR)=(AND+LSR)
    case 0xA0: if ((IR&0x1F)!=0x1B) { A=memory[addr]; if((IR&3)==3) X=A; } //LDA / LAX (illegal, used by my 1 rasterline player) 
               else {A=X=SP=memory[addr]&SP;} /*LAS(LAR)*/  ST&=125; ST|=((!A)<<1) | (A&128); break;  // LAS (LAR)
    case 0x80: if ((IR&0x1F)==0xB) { A = X & memory[addr]; ST&=125; ST|=(A&128) | ((!A)<<1); } //XAA (TXA+AND), highly unstable on real 6502!
               else if ((IR&0x1F)==0x1B) { SP=A&X; memory[addr]=SP&((addr>>8)+1); } //TAS(SHS) (SP=A&X, mem=S&H} - unstable on real 6502
               else {memory[addr]=A & (((IR&3)==3)?X:0xFF); storadd=addr;}  break; //STA / SAX (at times same as AHX/SHX/SHY) (illegal) 
   }
  }
  
  else if(IR&2) {  //nybble2:  2:illegal/LDX, 6:A/X/INC/DEC, A:Accu-shift/reg.transfer/NOP, E:shift/X/INC/DEC
   switch (IR&0x1F) { //addressing modes
    case 0x1E: PC++; addr=memory[PC]; PC++; addr+=memory[PC]*256 + ( ((IR&0xC0)!=0x80) ? X:Y ); cycles=5; break; //abs,x / abs,y
    case 0xE: PC++; addr=memory[PC]; PC++; addr+=memory[PC]*256; cycles=4; break; //abs
    case 0x16: PC++; addr = memory[PC] + ( ((IR&0xC0)!=0x80) ? X:Y ); cycles=4; break; //zp,x / zp,y
    case 6: PC++; addr = memory[PC]; cycles=3; break; //zp
    case 2: PC++; addr = PC; cycles=2;  //imm.
   }  
   addr&=0xFFFF; 
   switch (IR&0xE0) {
    case 0x00: ST&=0xFE; case 0x20: if((IR&0xF)==0xA) { A=(A<<1)+(ST&1); ST&=124;ST|=(A&128)|(A>255); A&=0xFF; ST|=(!A)<<1; } //ASL/ROL (Accu)
      else { T=(memory[addr]<<1)+(ST&1); ST&=124;ST|=(T&128)|(T>255); T&=0xFF; ST|=(!T)<<1; memory[addr]=T; cycles+=2; }  break; //RMW (Read-Write-Modify)
    case 0x40: ST&=0xFE; case 0x60: if((IR&0xF)==0xA) { T=A; A=(A>>1)+(ST&1)*128; ST&=124;ST|=(A&128)|(T&1); A&=0xFF; ST|=(!A)<<1; } //LSR/ROR (Accu)
      else { T=(memory[addr]>>1)+(ST&1)*128; ST&=124;ST|=(T&128)|(memory[addr]&1); T&=0xFF; ST|=(!T)<<1; memory[addr]=T; cycles+=2; }  break; //memory (RMW)
    case 0xC0: if(IR&4) { memory[addr]--; ST&=125;ST|=(!memory[addr])<<1|(memory[addr]&128); cycles+=2; } //DEC
      else {X--; X&=0xFF; ST&=125;ST|=(!X)<<1|(X&128);}  break; //DEX
    case 0xA0: if((IR&0xF)!=0xA) X=memory[addr];  else if(IR&0x10) {X=SP;break;}  else X=A;  ST&=125;ST|=(!X)<<1|(X&128);  break; //LDX/TSX/TAX
    case 0x80: if(IR&4) {memory[addr]=X;storadd=addr;}  else if(IR&0x10) SP=X;  else {A=X; ST&=125;ST|=(!A)<<1|(A&128);}  break; //STX/TXS/TXA
    case 0xE0: if(IR&4) { memory[addr]++; ST&=125;ST|=(!memory[addr])<<1|(memory[addr]&128); cycles+=2; } //INC/NOP
   }
  }
  
  else if((IR&0xC)==8) {  //nybble2:  8:register/status
   switch (IR&0xF0) {
    case 0x60: SP++; SP&=0xFF; A=memory[0x100+SP]; ST&=125;ST|=(!A)<<1|(A&128); cycles=4; break; //PLA
    case 0xC0: Y++; Y&=0xFF; ST&=125;ST|=(!Y)<<1|(Y&128); break; //INY
    case 0xE0: X++; X&=0xFF; ST&=125;ST|=(!X)<<1|(X&128); break; //INX
    case 0x80: Y--; Y&=0xFF; ST&=125;ST|=(!Y)<<1|(Y&128); break; //DEY
    case 0x00: memory[0x100+SP]=ST; SP--; SP&=0xFF; cycles=3; break; //PHP
    case 0x20: SP++; SP&=0xFF; ST=memory[0x100+SP]; cycles=4; break; //PLP
    case 0x40: memory[0x100+SP]=A; SP--; SP&=0xFF; cycles=3; break; //PHA
    case 0x90: A=Y; ST&=125;ST|=(!A)<<1|(A&128); break; //TYA
    case 0xA0: Y=A; ST&=125;ST|=(!Y)<<1|(Y&128); break; //TAY
    default: if(flagsw[IR>>5]&0x20) ST|=(flagsw[IR>>5]&0xDF); else ST&=255-(flagsw[IR>>5]&0xDF);  //CLC/SEC/CLI/SEI/CLV/CLD/SED
   }
  }
  
  else {  //nybble2:  0: control/branch/Y/compare  4: Y/compare  C:Y/compare/JMP
   if ((IR&0x1F)==0x10) { PC++; T=memory[PC]; if(T&0x80) T-=0x100; //BPL/BMI/BVC/BVS/BCC/BCS/BNE/BEQ  relative branch 
    if(IR&0x20) {if (ST&branchflag[IR>>6]) {PC+=T;cycles=3;}} else {if (!(ST&branchflag[IR>>6])) {PC+=T;cycles=3;}}  } 
   else {  //nybble2:  0:Y/control/Y/compare  4:Y/compare  C:Y/compare/JMP
    switch (IR&0x1F) { //addressing modes
     case 0: PC++; addr = PC; cycles=2; break; //imm. (or abs.low for JSR/BRK)
     case 0x1C: PC++; addr=memory[PC]; PC++; addr+=memory[PC]*256 + X; cycles=5; break; //abs,x
     case 0xC: PC++; addr=memory[PC]; PC++; addr+=memory[PC]*256; cycles=4; break; //abs
     case 0x14: PC++; addr = memory[PC] + X; cycles=4; break; //zp,x
     case 4: PC++; addr = memory[PC]; cycles=3;  //zp
    }  
    addr&=0xFFFF;  
    switch (IR&0xE0) {
     case 0x00: memory[0x100+SP]=PC%256; SP--;SP&=0xFF; memory[0x100+SP]=PC/256;  SP--;SP&=0xFF; memory[0x100+SP]=ST; SP--;SP&=0xFF; 
       PC = memory[0xFFFE]+memory[0xFFFF]*256-1; cycles=7; break; //BRK
     case 0x20: if(IR&0xF) { ST &= 0x3D; ST |= (memory[addr]&0xC0) | ( !(A&memory[addr]) )<<1; } //BIT
      else { memory[0x100+SP]=(PC+2)%256; SP--;SP&=0xFF; memory[0x100+SP]=(PC+2)/256;  SP--;SP&=0xFF; PC=memory[addr]+memory[addr+1]*256-1; cycles=6; }  break; //JSR
     case 0x40: if(IR&0xF) { PC = addr-1; cycles=3; } //JMP
      else { if(SP>=0xFF) return 0xFE; SP++;SP&=0xFF; ST=memory[0x100+SP]; SP++;SP&=0xFF; T=memory[0x100+SP]; SP++;SP&=0xFF; PC=memory[0x100+SP]+T*256-1; cycles=6; }  break; //RTI
     case 0x60: if(IR&0xF) { PC = memory[addr]+memory[addr+1]*256-1; cycles=5; } //JMP() (indirect)
      else { if(SP>=0xFF) return 0xFF; SP++;SP&=0xFF; T=memory[0x100+SP]; SP++;SP&=0xFF; PC=memory[0x100+SP]+T*256-1; cycles=6; }  break; //RTS
     case 0xC0: T=Y-memory[addr]; ST&=124;ST|=(!(T&0xFF))<<1|(T&128)|(T>=0); break; //CPY
     case 0xE0: T=X-memory[addr]; ST&=124;ST|=(!(T&0xFF))<<1|(T&128)|(T>=0); break; //CPX
     case 0xA0: Y=memory[addr]; ST&=125;ST|=(!Y)<<1|(Y&128); break; //LDY
     case 0x80: memory[addr]=Y; storadd=addr;  //STY
    }
   }
  }
 
  PC++; //PC&=0xFFFF; 
  return 0; 
 } 



// //----------------------------- SID emulation -----------------------------------------

// Arrays to support the emulation:
#include "precalc.inc"

#define PERIOD0 CLOCK_RATIO_DEFAULT //max(round(clock_ratio),9)
#define STEP0 3 //ceil(PERIOD0/9.0)

float ADSRperiods[16] = {PERIOD0, 32, 63, 95, 149, 220, 267, 313, 392, 977, 1954, 3126, 3907, 11720, 19532, 31251};
byte ADSRstep[16] =   {  STEP0, 1,  1,  1,  1,    1,   1,   1,   1,   1,    1,    1,    1,     1,     1,     1};

const byte ADSR_exptable[256] = {1, 30, 30, 30, 30, 30, 30, 16, 16, 16, 16, 16, 16, 16, 16, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4, //pos0:1  pos6:30  pos14:16  pos26:8
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, //pos54:4 //pos93:2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };


void cSID_init(int samplerate)
{
  int i;
  clock_ratio = C64_PAL_CPUCLK/samplerate;
  if (clock_ratio>9) { ADSRperiods[0]=clock_ratio; ADSRstep[0]=ceil(clock_ratio/9.0); }
  else { ADSRperiods[0]=9.0; ADSRstep[0]=1; }
  cutoff_ratio_8580 = -2 * 3.14 * (12500 / 2048) / samplerate; // -2 * 3.14 * ((82000/6.8) / 2048) / samplerate; //approx. 30Hz..12kHz according to datasheet, but only for 6.8nF value, 22nF makes 9Hz...3.7kHz? wrong
  cap_6581_reciprocal = -1000000/CAP_6581; //lighten CPU-load in sample-callback
  cutoff_steepness_6581 = FILTER_DARKNESS_6581*(2048.0-VCR_FET_TRESHOLD); //pre-scale for 0...2048 cutoff-value range //lighten CPU-load in sample-callback
  // cutoff_bottom_6581 = 1 - exp( -1 / (0.000000000470*1500000) / samplerate ); // 1 - exp( -2 * 3.14 * (26000/pow(2,9)/0.47) / samplerate ); //around 140..220Hz cutoff is set by VCR-MOSFET limiter/shunt-resistor (1.5MOhm)
  // cutoff_top_6581 = 20000; //Hz // (26000/0.47);  // 1 - exp( -2 * 3.14 * (26000/0.47) / samplerate);   //cutoff range is 9 octaves stated by datasheet, but process variation might eliminate any filter spec.
  // cutoff_ratio_6581 = -2 * 3.14 * (cutoff_top_6581 / 2048) / samplerate; //(cutoff_top_6581-cutoff_bottom_6581)/(2048.0-192.0); //datasheet: 30Hz..12kHz with 2.2pF -> 140Hz..56kHz with 470pF?

  // createCombinedWF(TriSaw_8580, 0.8, 2.4, 0.64);
  // createCombinedWF(PulseSaw_8580, 1.4, 1.9, 0.68);
  // createCombinedWF(PulseTriSaw_8580, 0.8, 2.5, 0.64);

  for(i = 0; i < 9; i++) {
    ADSRstate[i] = HOLDZERO_BITMASK;
    envcnt[i] = 0;
    ratecnt[i] = 0;
    phaseaccu[i] = 0;
    prevaccu[i] = 0;
    expcnt[i] = 0;
    prevSR[i]=0;
    noise_LFSR[i] = 0x7FFFFF;
    prevwfout[i] = 0;
  }

  for(i = 0; i < 3; i++) {
    sourceMSBrise[i] = 0;
    sourceMSB[i] = 0;
    prevlowpass[i] = 0;
    prevbandpass[i] = 0;
  }

  initSID();
}

//registers: 0:freql1  1:freqh1  2:pwml1  3:pwmh1  4:ctrl1  5:ad1   6:sr1  7:freql2  8:freqh2  9:pwml2 10:pwmh2 11:ctrl2 12:ad2  13:sr 14:freql3 15:freqh3 16:pwml3 17:pwmh3 18:ctrl3 19:ad3  20:sr3 
//           21:cutoffl 22:cutoffh 23:flsw_reso 24:vol_ftype 25:potX 26:potY 27:OSC3 28:ENV3
void initSID() {
  int i;
  for(i=0xD400; i<=0xD7FF; i++) memory[i] = 0;
  for(i=0xDE00; i<=0xDFFF; i++) memory[i] = 0;
  for(i=0; i<9; i++) {
    ADSRstate[i] = HOLDZERO_BITMASK;
    ratecnt[i] = envcnt[i] = expcnt[i] = 0;
  }
}


//My SID implementation is similar to what I worked out in a SwinSID variant during 3..4 months of development. (So jsSID only took 2 weeks armed with this experience.)
//I learned the workings of ADSR/WAVE/filter operations mainly from the quite well documented resid and resid-fp codes.
//(The SID reverse-engineering sites were also good sources.)
//Note that I avoided many internal/automatic variables from the SID function, assuming better speed this way. (Not using stack as much, but I'm not sure and it may depend on platform...)
//(The same is true for CPU emulation and player-code.)
int SID(unsigned char num, unsigned int baseaddr) //the SID emulation itself ('num' is the number of SID to iterate (0..2)
{
  //better keep these variables static so they won't slow down the routine like if they were internal automatic variables always recreated
  static byte channel, ctrl, SR, prevgate, wf, test, *sReg, *vReg;
  static unsigned int accuadd, MSB, pw, wfout;
  static int tmp, step, lim, nonfilt, filtin, filtout, output;
  static float period, steep, rDS_VCR_FET, cutoff[3], resonance[3], ftmp;

 filtin=nonfilt=0; sReg = &memory[baseaddr]; vReg = sReg;

 //treating 2SID and 3SID channels uniformly (0..5 / 0..8), this probably avoids some extra code
 for (channel = num * SID_CHANNEL_AMOUNT ; channel < (num + 1) * SID_CHANNEL_AMOUNT ; channel++, vReg += 7) {
  ctrl = vReg[4];

  //ADSR envelope-generator:
  SR = vReg[6]; tmp = 0;
  prevgate = (ADSRstate[channel] & GATE_BITMASK);
  if (prevgate != (ctrl & GATE_BITMASK)) { //gatebit-change?
   if (prevgate) ADSRstate[channel] &= 0xFF - (GATE_BITMASK | ATTACK_BITMASK | DECAYSUSTAIN_BITMASK);
   else { //falling edge
    ADSRstate[channel] = (GATE_BITMASK | ATTACK_BITMASK | DECAYSUSTAIN_BITMASK); //rising edge, also sets hold_zero_bit=0
    if ((SR & 0xF) > (prevSR[channel] & 0xF)) tmp = 1; //assume SR->GATE write order: workaround to have crisp soundstarts by triggering delay-bug
   }                                                      //(this is for the possible missed CTRL(GATE) vs SR register write order situations (1MHz CPU is cca 20 times faster than samplerate)
  }
  prevSR[channel] = SR; //if(SR&0xF) ratecnt[channel]+=5;  //assume SR->GATE write order: workaround to have crisp soundstarts by triggering delay-bug
  ratecnt[channel] += clock_ratio; if (ratecnt[channel] >= 0x8000) ratecnt[channel] -= 0x8000; //can wrap around (ADSR delay-bug: short 1st frame)
  //set ADSR period that should be checked against rate-counter (depending on ADSR state Attack/DecaySustain/Release)
  if (ADSRstate[channel] & ATTACK_BITMASK) step = vReg[5] >> 4;
  else if (ADSRstate[channel] & DECAYSUSTAIN_BITMASK) step = vReg[5] & 0xF;
  else step = SR & 0xF;
  period = ADSRperiods[step]; step = ADSRstep[step];
  if (ratecnt[channel] >= period && ratecnt[channel] < period + clock_ratio && tmp == 0) { //ratecounter shot (matches rateperiod) (in genuine SID ratecounter is LFSR)
   ratecnt[channel] -= period; //compensation for timing instead of simply setting 0 on rate-counter overload
   if ((ADSRstate[channel] & ATTACK_BITMASK) || ++expcnt[channel] == ADSR_exptable[envcnt[channel]]) {
    if (!(ADSRstate[channel] & HOLDZERO_BITMASK)) {
     if (ADSRstate[channel] & ATTACK_BITMASK) 
     { envcnt[channel]+=step; if (envcnt[channel]>=0xFF) {envcnt[channel]=0xFF; ADSRstate[channel] &= 0xFF-ATTACK_BITMASK;} }
     else if ( !(ADSRstate[channel] & DECAYSUSTAIN_BITMASK) || envcnt[channel] > (SR&0xF0) + (SR>>4) )
     { envcnt[channel]-=step; if (envcnt[channel]<=0 && envcnt[channel]+step!=0) {envcnt[channel]=0; ADSRstate[channel]|=HOLDZERO_BITMASK;} }
    }
    expcnt[channel] = 0;
   }
  }
  envcnt[channel] &= 0xFF;
 
  //WAVE-generation code (phase accumulator and waveform-selector):
  test = ctrl & TEST_BITMASK;  wf = ctrl & 0xF0;  accuadd = (vReg[0] + vReg[1] * 256) * clock_ratio;
  if (test || ((ctrl & SYNC_BITMASK) && sourceMSBrise[num])) phaseaccu[channel] = 0;
  else { phaseaccu[channel] += accuadd; if (phaseaccu[channel] > 0xFFFFFF) phaseaccu[channel] -= 0x1000000; }
  phaseaccu[channel] &= 0xFFFFFF; MSB = phaseaccu[channel] & 0x800000; sourceMSBrise[num] = (MSB > (prevaccu[channel] & 0x800000)) ? 1 : 0;
  if (wf & NOISE_BITMASK) { //noise waveform
   tmp = noise_LFSR[channel];
   if (((phaseaccu[channel] & 0x100000) != (prevaccu[channel] & 0x100000)) || accuadd >= 0x100000) //clock LFSR all time if clockrate exceeds observable at given samplerate 
   { step = (tmp & 0x400000) ^ ((tmp & 0x20000) << 5); tmp = ((tmp << 1) + (step ? 1 : test)) & 0x7FFFFF; noise_LFSR[channel]=tmp; }
   //we simply zero output when other waveform is mixed with noise. On real SID LFSR continuously gets filled by zero and locks up. ($C1 waveform with pw<8 can keep it for a while...)
   wfout = (wf & 0x70) ? 0 : ((tmp & 0x100000) >> 5) + ((tmp & 0x40000) >> 4) + ((tmp & 0x4000) >> 1) + ((tmp & 0x800) << 1) + ((tmp & 0x200) << 2) + ((tmp & 0x20) << 5) + ((tmp & 0x04) << 7) + ((tmp & 0x01) << 8);
  } 
  else if (wf & PULSE_BITMASK) { //simple pulse
   pw = (vReg[2] + (vReg[3] & 0xF) * 256) * 16;  tmp = (int) accuadd >> 9;  
   if (0 < pw && pw < tmp) pw = tmp;  tmp ^= 0xFFFF;  if (pw > tmp) pw = tmp;  
   tmp = phaseaccu[channel] >> 8;
   if (wf == PULSE_BITMASK) { //simple pulse, most often used waveform, make it sound as clean as possible without oversampling
    //One of my biggest success with the SwinSID-variant was that I could clean the high-pitched and thin sounds.
    //(You might have faced with the unpleasant sound quality of high-pitched sounds without oversampling. We need so-called 'band-limited' synthesis instead.
    // There are a lot of articles about this issue on the internet. In a nutshell, the harsh edges produce harmonics that exceed the 
    // Nyquist frequency (samplerate/2) and they are folded back into hearable range, producing unvanted ringmodulation-like effect.)
    //After so many trials with dithering/filtering/oversampling/etc. it turned out I can't eliminate the fukkin aliasing in time-domain, as suggested at pages.
    //Oversampling (running the wave-generation 8 times more) was not a way at 32MHz SwinSID. It might be an option on PC but I don't prefer it in JavaScript.)
    //The only solution that worked for me in the end, what I came up with eventually: The harsh rising and falling edges of the pulse are
    //elongated making it a bit trapezoid. But not in time-domain, but altering the transfer-characteristics. This had to be done
    //in a frequency-dependent way, proportionally to pitch, to keep the deep sounds crisp. The following code does this (my favourite testcase is Robocop3 intro):
    step = (accuadd>=255)? 65535/(accuadd/256.0) : 0xFFFF; //simple pulse, most often used waveform, make it sound as clean as possible without oversampling
    if (test) wfout=0xFFFF;
    else if (tmp<pw) { lim=(0xFFFF-pw)*step; if (lim>0xFFFF) lim=0xFFFF; tmp=lim-(pw-tmp)*step; wfout=(tmp<0)?0:tmp; } //rising edge
    else { lim=pw*step; if (lim>0xFFFF) lim=0xFFFF; tmp=(0xFFFF-tmp)*step-lim; wfout=(tmp>=0)?0xFFFF:tmp; } //falling edge
   }
   else { //combined pulse
    wfout = (tmp >= pw || test) ? 0xFFFF:0; //(this would be enough for a simple but aliased-at-high-pitches pulse)
    if (wf&TRI_BITMASK) { 
     if (wf&SAW_BITMASK) { wfout = wfout? combinedWF(num,channel,PulseTriSaw_8580,tmp>>4,1,vReg[1]) : 0; } //pulse+saw+triangle (waveform nearly identical to tri+saw)
     else { tmp=phaseaccu[channel]^(ctrl&RING_BITMASK?sourceMSB[num]:0); wfout = (wfout)? combinedWF(num,channel,PulseSaw_8580,(tmp^(tmp&0x800000?0xFFFFFF:0))>>11,0,vReg[1]) : 0; } } //pulse+triangle
    else if (wf&SAW_BITMASK) wfout = wfout? combinedWF(num,channel,PulseSaw_8580,tmp>>4,1,vReg[1]) : 0; //pulse+saw
   }
  }
  else if (wf&SAW_BITMASK) { //saw
   wfout=phaseaccu[channel]>>8; //saw (this row would be enough for simple but aliased-at-high-pitch saw)
   //The anti-aliasing (cleaning) of high-pitched sawtooth wave works by the same principle as mentioned above for the pulse,
   //but the sawtooth has even harsher edge/transition, and as the falling edge gets longer, tha rising edge should became shorter, 
   //and to keep the amplitude, it should be multiplied a little bit (with reciprocal of rising-edge steepness).
   //The waveform at the output essentially becomes an asymmetric triangle, more-and-more approaching symmetric shape towards high frequencies.
   //(If you check a recording from the real SID, you can see a similar shape, the high-pitch sawtooth waves are triangle-like...)
   //But for deep sounds the sawtooth is really close to a sawtooth, as there is no aliasing there, but deep sounds should be sharp...
   if (wf&TRI_BITMASK) wfout = combinedWF(num,channel,TriSaw_8580,wfout>>4,1,vReg[1]); //saw+triangle
   else { //simple cleaned (bandlimited) saw
    steep=(accuadd/65536.0)/288.0;
    wfout += wfout*steep; if(wfout>0xFFFF) wfout=0xFFFF-(wfout-0x10000)/steep; 
   } 
  }
  else if (wf&TRI_BITMASK) { //triangle (this waveform has no harsh edges, so it doesn't suffer from strong aliasing at high pitches)
   tmp=phaseaccu[channel]^(ctrl&RING_BITMASK?sourceMSB[num]:0); wfout = (tmp^(tmp&0x800000?0xFFFFFF:0)) >> 7; 
  }
  wfout&=0xFFFF; if (wf) prevwfout[channel] = wfout; else { wfout = prevwfout[channel]; } //emulate waveform 00 floating wave-DAC (on real SID waveform00 decays after 15s..50s depending on temperature?)
  prevaccu[channel] = phaseaccu[channel]; sourceMSB[num] = MSB;            //(So the decay is not an exact value. Anyway, we just simply keep the value to avoid clicks and support SounDemon digi later...)

  //routing the channel signal to either the filter or the unfiltered master output depending on filter-switch SID-registers
  if (sReg[0x17] & FILTSW[channel]) filtin += ((int)wfout - 0x8000) * envcnt[channel] / 256;
  else if ((FILTSW[channel] != 4) || !(sReg[0x18] & OFF3_BITMASK)) 
   nonfilt += ((int)wfout - 0x8000) * envcnt[channel] / 256;
 }
 //update readable SID1-registers (some SID tunes might use 3rd channel ENV3/OSC3 value as control)
 if(num==0, memory[1]&3) { sReg[0x1B]=wfout>>8; sReg[0x1C]=envcnt[3]; } //OSC3, ENV3 (some players rely on it)    

 //FILTER: two integrator loop bi-quadratic filter, workings learned from resid code, but I kindof simplified the equations
 //The phases of lowpass and highpass outputs are inverted compared to the input, but bandpass IS in phase with the input signal.
 //The 8580 cutoff frequency control-curve is ideal (binary-weighted resistor-ladder VCRs), while the 6581 has a treshold, and below that it 
 //outputs a constant ~200Hz cutoff frequency. (6581 uses MOSFETs as VCRs to control cutoff causing nonlinearity and some 'distortion' due to resistance-modulation.
 //There's a cca. 1.53Mohm resistor in parallel with the MOSFET in 6581 which doesn't let the frequency go below 200..220Hz
 //Even if the MOSFET doesn't conduct at all. 470pF capacitors are small, so 6581 can't go below this cutoff-frequency with 1.5MOhm.)
 cutoff[num] = sReg[0x16] * 8 + (sReg[0x15] & 7);
 if (SID_model[num] == 8580) {
  cutoff[num] = ( 1 - exp((cutoff[num]+2) * cutoff_ratio_8580) ); //linear curve by resistor-ladder VCR
  resonance[num] = ( pow(2, ((4 - (sReg[0x17] >> 4)) / 8.0)) );
 } 
 else { //6581
  cutoff[num] += round(filtin*FILTER_DISTORTION_6581); //MOSFET-VCR control-voltage-modulation (resistance-modulation aka 6581 filter distortion) emulation
  rDS_VCR_FET = cutoff[num]<=VCR_FET_TRESHOLD ? 100000000.0 //below Vth treshold Vgs control-voltage FET presents an open circuit
   : cutoff_steepness_6581/(cutoff[num]-VCR_FET_TRESHOLD); // rDS ~ (-Vth*rDSon) / (Vgs-Vth)  //above Vth FET drain-source resistance is proportional to reciprocal of cutoff-control voltage
  cutoff[num] = ( 1 - exp( cap_6581_reciprocal / (VCR_SHUNT_6581*rDS_VCR_FET/(VCR_SHUNT_6581+rDS_VCR_FET)) / samplerate ) ); //curve with 1.5MOhm VCR parallel Rshunt emulation
  resonance[num] = ( (sReg[0x17] > 0x5F) ? 8.0 / (sReg[0x17] >> 4) : 1.41 );
 }  
 filtout=0;
 ftmp = filtin + prevbandpass[num] * resonance[num] + prevlowpass[num];
 if (sReg[0x18] & HIGHPASS_BITMASK) filtout -= ftmp;
 ftmp = prevbandpass[num] - ftmp * cutoff[num];
 prevbandpass[num] = ftmp;
 if (sReg[0x18] & BANDPASS_BITMASK) filtout -= ftmp;
 ftmp = prevlowpass[num] + ftmp * cutoff[num];
 prevlowpass[num] = ftmp;
 if (sReg[0x18] & LOWPASS_BITMASK) filtout += ftmp;    

 //output stage for one SID
 //when it comes to $D418 volume-register digi playback, I made an AC / DC separation for $D418 value in the SwinSID at low (20Hz or so) cutoff-frequency,
 //and sent the AC (highpass) value to a 4th 'digi' channel mixed to the master output, and set ONLY the DC (lowpass) value to the volume-control.
 //This solved 2 issues: Thanks to the lowpass filtering of the volume-control, SID tunes where digi is played together with normal SID channels,
 //won't sound distorted anymore, and the volume-clicks disappear when setting SID-volume. (This is useful for fade-in/out tunes like Hades Nebula, where clicking ruins the intro.)
 output = (nonfilt+filtout) * (sReg[0x18]&0xF) / OUTPUT_SCALEDOWN;
 if (output>=32767) output=32767; else if (output<=-32768) output=-32768; //saturation logic on overload (not needed if the callback handles it)
  return (int)output; // master output
}


//The anatomy of combined waveforms: The resid source simply uses 4kbyte 8bit samples from wavetable arrays, says these waveforms are mystic due to the analog behaviour.
//It's true, the analog things inside SID play a significant role in how the combined waveforms look like, but process variations are not so huge that cause much differences in SIDs.
//After checking these waveforms by eyes, it turned out for me that these waveform are fractal-like, recursively approachable waveforms.
//My 1st thought and trial was to store only a portion of the waveforms in table, and magnify them depending on phase-accumulator's state.
//But I wanted to understand how these waveforms are produced. I felt from the waveform-diagrams that the bits of the waveforms affect each other,
//hence the recursive look. A short C code proved by assumption, I could generate something like a pulse+saw combined waveform.
//Recursive calculations were not feasible for MCU of SwinSID, but for jsSID I could utilize what I found out and code below generates the combined waveforms into wavetables. 
//To approach the combined waveforms as much as possible, I checked out the SID schematic that can be found at some reverse-engineering sites...
//The SID's R-2R ladder WAVE DAC is driven by operation-amplifier like complementary FET output drivers, so that's not the place where I first thought the magic happens.
//These 'opamps' (for all 12 wave-bits) have single FETs as inputs, and they switch on above a certain level of input-voltage, causing 0 or 1 bit as R-2R DAC input.
//So the first keyword for the workings is TRESHOLD. These FET inputs are driven through serial switch FETs (wave-selector) that normally enables one waveform at a time.
//The phase-accumulator's output is brought to 3 kinds of circuitries for the 3 basic waveforms. The pulse simply drives
//all wave-selector inputs with a 0/1 depending on pulsewidth, the sawtooth has a XOR for triangle/ringmod generation, but what
//is common for all waveforms, they have an open-drain driver before the wave-selector, which has FETs towards GND and 'FET resistor' towards the power-supply rail.
//These outputs are clearly not designed to drive high loads, and normally they only have to drive the FETs input mentioned above.
//But when more of these output drivers are switched together by the switch-FETs in the wave-selector, they affect each other by loading each other.
//The pulse waveform, when selected, connects all of them together through a fairly strong connection, and its signal also affects the analog level (pulls below the treshold)...
//The farther a specific DAC bit driver is from the other, the less it affects its output. It turned out it's not powers of 2 but something else,
//that creates similar combined waveforms to that of real SID's... Note that combined waveforms never have values bigger than their sourcing sawtooth wave.
//The analog levels that get generated by the various bit drivers, that pull each other up/DOWN, depend on the resistances the components/wires inside the SID.
//And finally, what is output on the DAC depends on whether these analog levels are below or above the FET gate's treshold-level,
//That's how the combined waveform is generated. Maybe I couldn't explain well enough, but the code below is simple enough to understand the mechanism algoritmically.
//This simplified schematic exapmle might make it easier to understand sawtooth+pulse combination (must be observed with monospace fonts):
//                               _____            |-    .--------------.   /\/\--.
// Vsupply                /  .----| |---------*---|-    /    Vsupply   !    R    !      As can be seen on this schematic,
//  ------.       other   !  !   _____        !  TRES   \       \      !         /      the pulse wave-selector FETs 
//        !       saw bit *--!----| |---------'  HOLD   /       !     |-     2R  \      connect the neighbouring sawtooth
//        /       output  !  !                          !      |------|-         /      outputs with a fairly strong 
//     Rd \              |-  !WAVEFORM-SELECTOR         *--*---|-      !    R    !      connection to each other through
//        /              |-  !SWITCHING FETs            !  !    !      *---/\/\--*      their own wave-selector FETs.
//        ! saw-bit          !    _____                |-  !   ---     !         !      So the adjacent sawtooth outputs
//        *------------------!-----| |-----------*-----|-  !          |-         /      pull each other lower (or maybe a bit upper but not exceeding sawtooth line)
//        ! (weak drive,so   !  saw switch       ! TRES-!  `----------|-     2R  \      depending on their low/high state and
//       |- can be shifted   !                   ! HOLD !              !         /      distance from each other, causing
//  -----|- down (& up?)     !    _____          !      !              !     R   !      the resulting analog level that
//        ! by neighbours)   *-----| |-----------'     ---            ---   /\/\-*      will either turn the output on or not.
//   GND ---                 !  pulse switch                                     !      (Depending on their relation to treshold.)
//
//(As triangle waveform connects adjacent bits by default, the above explained effect becomes even stronger, that's why combined waveforms with thriangle are at 0 level most of the time.)

//in case you don't like these calculated combined waveforms it's easy to substitute the generated tables by pre-sampled 'exact' versions












unsigned int combinedWF(unsigned char num, unsigned char channel, unsigned int* wfarray, int index, unsigned char differ6581, byte freqh) {
  static float addf;
  addf = 0.6 + 0.4 / freqh;
  if(differ6581 && SID_model[num] == 6581) index &= 0x7FF;
  prevwavdata[channel] = wfarray[index] * addf + prevwavdata[channel] * (1.0 - addf);
  return prevwavdata[channel];
}

// void createCombinedWF(unsigned int* wfarray, float bitmul, float bitstrength, float treshold) {
//   int i, j, k;
//   for (i = 0; i < 4096; i++) {
//     wfarray[i] = 0;
//     for (j = 0; j < 12; j++) {
//       float bitlevel = 0;
//       for (k = 0; k < 12; k++) {
//         bitlevel += (bitmul / pow(bitstrength, fabs(k - j))) * (((i>>k)&1)-0.5);
//       }
//       wfarray[i] += (bitlevel >= treshold) ? pow(2, j) : 0;
//     }
//     wfarray[i] *= 12;
//   }
// }














//
// libcsid exported api
//

const char *libcsid_getauthor() {
    return (char *)&SIDauthor;
}

const char *libcsid_getinfo() {
    return (char *)&SIDinfo;
}

const char *libcsid_gettitle() {
    return (char *)&SIDtitle;
}

void libcsid_init(int _samplerate, int _sidmodel) {
  memory = (byte *)malloc(MAX_DATA_LEN);

  samplerate = _samplerate;
  sampleratio = round(C64_PAL_CPUCLK / samplerate);
  requested_SID_model = _sidmodel;
}

int libcsid_load(unsigned char *_buffer, int _bufferlen, int _subtune) {
  int readata, strend, subtune_amount, preferred_SID_model[3] = {8580.0, 8580.0, 8580.0};
  unsigned int i, datalen, offs, loadaddr;

  subtune = _subtune;

  unsigned char *filedata = _buffer;
  datalen = _bufferlen;

  offs = filedata[7];
  loadaddr = filedata[8] + filedata[9] ? filedata[8] * 256 + filedata[9] : filedata[offs] + filedata[offs + 1] * 256;
  printf("\nOffset: $%4.4X, Loadaddress: $%4.4X \nTimermodes:", offs, loadaddr);

  for (i = 0; i < 32; i++) {
    timermode[31 - i] = (filedata[0x12 + (i >> 3)] & (byte)pow(2, 7 - i % 8)) ? 1 : 0;
    printf(" %1d",timermode[31 - i]);
  }

  for (i = 0; i < MAX_DATA_LEN; i++) {
    memory[i] = 0;
  }

  for (i = offs + 2; i < datalen; i++) {
    if (loadaddr + i - (offs + 2) < MAX_DATA_LEN) {
      memory[loadaddr + i - (offs + 2)] = filedata[i];
    }
  }

  strend = 1;
  for (i = 0; i < 32; i++) {
    if (strend != 0) {
      strend = SIDtitle[i] = filedata[0x16 + i];
    } else {
      strend = SIDtitle[i] = 0;
    }
  }

  strend = 1;
  for (i = 0; i < 32; i++) {
    if (strend != 0) {
      strend = SIDauthor[i] = filedata[0x36 + i];
    } else {
      strend = SIDauthor[i] = 0;
    }
  }

  strend = 1;
  for (i = 0; i < 32; i++) {
    if (strend != 0) {
      strend = SIDinfo[i] = filedata[0x56 + i];
    } else {
      strend = SIDinfo[i] = 0;
    }
  }

  initaddr=filedata[0xA]+filedata[0xB]? filedata[0xA]*256+filedata[0xB] : loadaddr; playaddr=playaddf=filedata[0xC]*256+filedata[0xD]; printf("\nInit:$%4.4X,Play:$%4.4X, ",initaddr,playaddr);
  subtune_amount=filedata[0xF];
  preferred_SID_model[0] = (filedata[0x77]&0x30)>=0x20? 8580 : 6581;

  printf("Subtunes:%d , preferred SID-model:%d", subtune_amount, preferred_SID_model[0]);

  preferred_SID_model[1] = (filedata[0x77]&0xC0)>=0x80 ? 8580 : 6581;
  preferred_SID_model[2] = (filedata[0x76]&3)>=3 ? 8580 : 6581;
  SID_address[1] = filedata[0x7A]>=0x42 && (filedata[0x7A]<0x80 || filedata[0x7A]>=0xE0) ? 0xD000+filedata[0x7A]*16 : 0;
  SID_address[2] = filedata[0x7B]>=0x42 && (filedata[0x7B]<0x80 || filedata[0x7B]>=0xE0) ? 0xD000+filedata[0x7B]*16 : 0;

  SIDamount=1+(SID_address[1]>0)+(SID_address[2]>0); if(SIDamount>=2) printf("(SID1), %d(SID2:%4.4X)",preferred_SID_model[1],SID_address[1]); 
  if(SIDamount==3) printf(", %d(SID3:%4.4X)",preferred_SID_model[2],SID_address[2]);
  if (requested_SID_model!=-1) printf(" (requested:%d)",requested_SID_model); printf("\n");

  for (i=0;i<SIDamount;i++) {
    if (requested_SID_model==8580 || requested_SID_model==6581) SID_model[i] = requested_SID_model;
    else SID_model[i] = preferred_SID_model[i];
  }

  OUTPUT_SCALEDOWN = SID_CHANNEL_AMOUNT * 16 + 26;
  if (SIDamount == 2) {
    OUTPUT_SCALEDOWN /= 0.6;
  } else if (SIDamount >= 3) {
    OUTPUT_SCALEDOWN /= 0.4;
  }

  cSID_init(samplerate);
  init(subtune);

  return 0;
}

void libcsid_render(unsigned short *_output, int _numsamples) {
  play(0, (Uint8 *)_output, _numsamples * 2);
}
