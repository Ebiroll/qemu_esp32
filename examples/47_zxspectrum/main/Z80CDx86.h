/*** Z80Em: Portable Z80 emulator *******************************************/
/***                                                                      ***/
/***                              Z80CDx86.h                              ***/
/***                                                                      ***/
/*** This file contains various macros used by the emulation engine,      ***/
/*** optimised for GCC/x86                                                ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#define _INLINE         extern __inline__

#define M_POP(Rg)           \
        R.Rg.D=M_RDSTACK(R.SP.D)+(M_RDSTACK((R.SP.D+1)&65535)<<8); \
        R.SP.W.l+=2
#define M_PUSH(Rg)          \
        R.SP.W.l-=2;        \
        M_WRSTACK(R.SP.D,R.Rg.D); \
        M_WRSTACK((R.SP.D+1)&65535,R.Rg.D>>8)
#define M_CALL              \
{                           \
 int q;                     \
 q=M_RDMEM_OPCODE_WORD();   \
 M_PUSH(PC);                \
 R.PC.D=q;                  \
 Z80_ICount-=7;             \
}
#define M_JP                \
        R.PC.D=M_RDOP_ARG(R.PC.D)+((M_RDOP_ARG((R.PC.D+1)&65535))<<8)
#define M_JR                \
        R.PC.W.l+=((offset)M_RDOP_ARG(R.PC.D))+1; Z80_ICount-=5
#define M_RET           M_POP(PC); Z80_ICount-=6
#define M_RST(Addr)     M_PUSH(PC); R.PC.D=Addr
#define M_SET(Bit,Reg)  Reg|=1<<Bit
#define M_RES(Bit,Reg)  Reg&=~(1<<Bit)
#define M_BIT(Bit,Reg)      \
        R.AF.B.l=(R.AF.B.l&C_FLAG)|H_FLAG| \
        ((Reg&(1<<Bit))? ((Bit==7)?S_FLAG:0):Z_FLAG)
#define M_IN(Reg)           \
        Reg=DoIn(R.BC.B.l,R.BC.B.h); \
        R.AF.B.l=(R.AF.B.l&C_FLAG)|ZSPTable[Reg]

#define M_ADDW(Reg1,Reg2)    \
{                            \
 asm (                       \
 " addb %%al,%%cl       \n"  \
 " adcb %%ah,%%ch       \n"  \
 " lahf                 \n"  \
 " andb $0x11,%%ah      \n"  \
 " andb $0xC4,%1	\n"  \
 " orb %%ah,%1		\n"  \
 :"=c" (R.Reg1.D),	     \
  "=g" (R.AF.B.l)	     \
 :"0" (R.Reg1.D),            \
  "1" (R.AF.B.l),	     \
  "a" (R.Reg2.D)             \
 );                          \
}

_INLINE void _M_ADCW (dword Reg)
{
 asm (
 " shrb $1,%%al         \n"
 " adcb %%dl,%%cl       \n"
 " adcb %%dh,%%ch       \n"
 " lahf                 \n"
 " setob %%al		\n"
 " andb $0x91,%%ah	\n"
 " shlb $2,%%al		\n"
 " orb %%ah,%%al	\n"
 " orl %%ecx,%%ecx      \n"
 " lahf                 \n"
 " andb $0x40,%%ah      \n"
 " orb %%ah,%%al        \n"
 :"=c" (R.HL.D),
  "=a" (R.AF.B.l)
 :"0" (R.HL.D),
  "d" (Reg),
  "a" (R.AF.B.l)
 );
}
#define M_ADCW(Reg)     _M_ADCW(R.Reg.D)

_INLINE void _M_SBCW (dword Reg)
{
 asm (
 " shrb $1,%%al         \n"
 " sbbb %%dl,%%cl       \n"
 " sbbb %%dh,%%ch       \n"
 " lahf                 \n"
 " setob %%al		\n"
 " andb $0x91,%%ah	\n"
 " shlb $2,%%al		\n"
 " orb %%ah,%%al	\n"
 " orl %%ecx,%%ecx      \n"
 " lahf                 \n"
 " andb $0x40,%%ah      \n"
 " orb $2,%%al          \n"
 " orb %%ah,%%al        \n"
 :"=c" (R.HL.D),
  "=a" (R.AF.B.l)
 :"0" (R.HL.D),
  "d" (Reg),
  "a" (R.AF.B.l)
 );
}
#define M_SBCW(Reg)     _M_SBCW(R.Reg.D)

/*
_INLINE void _M_RLCA (void)
{
 asm (
 " xchgb %%ah,%%al      \n"
 " sahf                 \n"
 " rolb $1,%%al         \n"
 " lahf                 \n"
 " xchgb %%ah,%%al      \n"
 " andb $0xC5,%%al      \n"
 :"=a" (R.AF.D)
 :"a" (R.AF.D)
 );
}
#define M_RLCA  _M_RLCA()

_INLINE void _M_RRCA (void)
{
 asm (
 " xchgb %%ah,%%al      \n"
 " sahf                 \n"
 " rorb $1,%%al         \n"
 " lahf                 \n"
 " xchgb %%ah,%%al      \n"
 " andb $0xC5,%%al      \n"
 :"=a" (R.AF.D)
 :"a" (R.AF.D)
 );
}
#define M_RRCA  _M_RRCA()

_INLINE void _M_RLA (void)
{
 asm (
 " xchgb %%ah,%%al      \n"
 " sahf                 \n"
 " rclb $1,%%al         \n"
 " lahf                 \n"
 " xchgb %%ah,%%al      \n"
 " andb $0xC5,%%al      \n"
 :"=a" (R.AF.D)
 :"a" (R.AF.D)
 );
}
#define M_RLA   _M_RLA()

_INLINE void _M_RRA (void)
{
 asm (
 " xchgb %%ah,%%al      \n"
 " sahf                 \n"
 " rcrb $1,%%al         \n"
 " lahf                 \n"
 " xchgb %%ah,%%al      \n"
 " andb $0xC5,%%al      \n"
 :"=a" (R.AF.D)
 :"a" (R.AF.D)
 );
}
#define M_RRA   _M_RRA()

#define M_RLC(Reg)           \
 asm (                       \
 " rolb $1,%0           \n"  \
 " setcb %%al           \n"  \
 " orb $0x00,%0         \n"  \
 " lahf                 \n"  \
 " orb %%ah,%%al        \n"  \
 " andb $0xC5,%%al      \n"  \
 :"=g" (Reg),                \
  "=a" (R.AF.B.l)            \
 :"0" (Reg)                  \
 );

#define M_RRC(Reg)           \
 asm (                       \
 " rorb $1,%0           \n"  \
 " setcb %%al           \n"  \
 " orb $0x00,%0         \n"  \
 " lahf                 \n"  \
 " orb %%ah,%%al        \n"  \
 " andb $0xC5,%%al      \n"  \
 :"=g" (Reg),                \
  "=a" (R.AF.B.l)            \
 :"0" (Reg)                  \
 );

#define M_RL(Reg)            \
 asm (                       \
 " shrb $1,%%al         \n"  \
 " rclb $1,%0           \n"  \
 " setcb %%al           \n"  \
 " orb $0x00,%0         \n"  \
 " lahf                 \n"  \
 " orb %%ah,%%al        \n"  \
 " andb $0xC5,%%al      \n"  \
 :"=g" (Reg),                \
  "=a" (R.AF.B.l)            \
 :"0" (Reg),                 \
  "a" (R.AF.B.l)             \
 );

#define M_RR(Reg)            \
 asm (                       \
 " shrb $1,%%al         \n"  \
 " rcrb $1,%0           \n"  \
 " setcb %%al           \n"  \
 " orb $0x00,%0         \n"  \
 " lahf                 \n"  \
 " orb %%ah,%%al        \n"  \
 " andb $0xC5,%%al      \n"  \
 :"=g" (Reg),                \
  "=a" (R.AF.B.l)            \
 :"0" (Reg),                 \
  "a" (R.AF.B.l)             \
 );

#define M_SLL(Reg)           \
 asm (                       \
 " shlb $1,%0           \n"  \
 " lahf                 \n"  \
 " andb $0x85,%%ah      \n"  \
 " movb %%ah,%1         \n"  \
 " orb $0x01,%0         \n"  \
 :"=g" (Reg),                \
  "=g" (R.AF.B.l)            \
 :"0" (Reg)                  \
 :"eax"                      \
 );

#define M_SLA(Reg)           \
 asm (                       \
 " shlb $1,%0           \n"  \
 " lahf                 \n"  \
 " andb $0xC5,%%ah      \n"  \
 " movb %%ah,%1         \n"  \
 :"=g" (Reg),                \
  "=g" (R.AF.B.l)            \
 :"0" (Reg)                  \
 :"eax"                      \
 );

#define M_SRL(Reg)           \
 asm (                       \
 " shrb $1,%0           \n"  \
 " lahf                 \n"  \
 " andb $0xC5,%%ah      \n"  \
 " movb %%ah,%1         \n"  \
 :"=g" (Reg),                \
  "=g" (R.AF.B.l)            \
 :"0" (Reg)                  \
 :"eax"                      \
 );

#define M_SRA(Reg)           \
 asm (                       \
 " sarb $1,%0           \n"  \
 " lahf                 \n"  \
 " andb $0xC5,%%ah      \n"  \
 " movb %%ah,%1         \n"  \
 :"=g" (Reg),                \
  "=g" (R.AF.B.l)            \
 :"0" (Reg)                  \
 :"eax"                      \
 );
*/
_INLINE void M_AND(byte Reg)
{
 asm (
 " andb %2,%0           \n"
 " lahf                 \n"
 " andb $0xC4,%%ah      \n"
 " orb $0x10,%%ah       \n"
 " movb %%ah,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"r" (Reg)
 :"eax"
 );
}

_INLINE void M_OR(byte Reg)
{
 asm (
 " orb %2,%0            \n"
 " lahf                 \n"
 " andb $0xC4,%%ah      \n"
 " movb %%ah,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"r" (Reg)
 :"eax"
 );
}

_INLINE void M_XOR(byte Reg)
{
 asm (
 " xorb %2,%0           \n"
 " lahf                 \n"
 " andb $0xC4,%%ah      \n"
 " movb %%ah,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"r" (Reg)
 :"eax"
 );
}

#define M_INC(Reg)           \
 asm (                       \
 " shrb $1,%%al         \n"  \
 " incb %0              \n"  \
 " lahf                 \n"  \
 " setob %%al           \n"  \
 " shlb $2,%%al         \n"  \
 " andb $0xD1,%%ah      \n"  \
 " orb %%ah,%%al        \n"  \
 :"=g" (Reg),                \
  "=a" (R.AF.B.l)            \
 :"0" (Reg),                 \
  "a" (R.AF.B.l)             \
 );

#define M_DEC(Reg)           \
 asm (                       \
 " shrb $1,%%al         \n"  \
 " decb %0              \n"  \
 " lahf                 \n"  \
 " setob %%al           \n"  \
 " shlb $2,%%al         \n"  \
 " andb $0xD1,%%ah      \n"  \
 " orb %%ah,%%al        \n"  \
 " orb $2,%%al          \n"  \
 :"=g" (Reg),                \
  "=a" (R.AF.B.l)            \
 :"0" (Reg),                 \
  "a" (R.AF.B.l)             \
 );

_INLINE void M_ADD (byte Reg)
{
 asm (
 " addb %2,%0           \n"
 " lahf                 \n"
 " setob %%al           \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 " movb %%al,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"r" (Reg),
  "0" (R.AF.B.h)
 :"eax"
 );
}

_INLINE void M_ADC (byte Reg)
{
 asm (
 " shrb $1,%%al         \n"
 " adcb %2,%0           \n"
 " lahf                 \n"
 " setob %%al           \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 :"=g" (R.AF.B.h),
  "=a" (R.AF.B.l)
 :"r" (Reg),
  "a" (R.AF.B.l),
  "0" (R.AF.B.h)
 );
}

_INLINE void M_SUB (byte Reg)
{
 asm (
 " subb %2,%0           \n"
 " lahf                 \n"
 " setob %%al           \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 " orb $2,%%al          \n"
 :"=g" (R.AF.B.h),
  "=a" (R.AF.B.l)
 :"r" (Reg),
  "0" (R.AF.B.h)
 );
}

_INLINE void M_SBC (byte Reg)
{
 asm (
 " shrb $1,%%al         \n"
 " sbbb %2,%0           \n"
 " lahf                 \n"
 " setob %%al           \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 " orb $2,%%al          \n"
 :"=g" (R.AF.B.h),
  "=a" (R.AF.B.l)
 :"r" (Reg),
  "a" (R.AF.B.l),
  "0" (R.AF.B.h)
 );
}

_INLINE void M_CP (byte Reg)
{
 asm (
 " cmpb %2,%0          \n"
 " lahf                \n"
 " setob %%al          \n"
 " shlb $2,%%al        \n"
 " andb $0xD1,%%ah     \n"
 " orb %%ah,%%al       \n"
 " orb $2,%%al         \n"
 :"=g" (R.AF.B.h),
  "=a" (R.AF.B.l)
 :"r" (Reg),
  "0" (R.AF.B.h)
 );
}

#define M_RLCA              \
 R.AF.B.h=(R.AF.B.h<<1)|((R.AF.B.h&0x80)>>7); \
 R.AF.B.l=(R.AF.B.l&0xEC)|(R.AF.B.h&C_FLAG)

#define M_RRCA              \
 R.AF.B.l=(R.AF.B.l&0xEC)|(R.AF.B.h&0x01); \
 R.AF.B.h=(R.AF.B.h>>1)|(R.AF.B.h<<7)

#define M_RLA               \
{                           \
 int i;                     \
 i=R.AF.B.l&C_FLAG;         \
 R.AF.B.l=(R.AF.B.l&0xEC)|((R.AF.B.h&0x80)>>7); \
 R.AF.B.h=(R.AF.B.h<<1)|i;  \
}

#define M_RRA               \
{                           \
 int i;                     \
 i=R.AF.B.l&C_FLAG;         \
 R.AF.B.l=(R.AF.B.l&0xEC)|(R.AF.B.h&0x01); \
 R.AF.B.h=(R.AF.B.h>>1)|(i<<7);            \
}

#define M_RLC(Reg)         \
{                          \
 int q;                    \
 q=Reg>>7;                 \
 Reg=(Reg<<1)|q;           \
 R.AF.B.l=ZSPTable[Reg]|q; \
}
#define M_RRC(Reg)         \
{                          \
 int q;                    \
 q=Reg&1;                  \
 Reg=(Reg>>1)|(q<<7);      \
 R.AF.B.l=ZSPTable[Reg]|q; \
}
#define M_RL(Reg)            \
{                            \
 int q;                      \
 q=Reg>>7;                   \
 Reg=(Reg<<1)|(R.AF.B.l&1);  \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_RR(Reg)            \
{                            \
 int q;                      \
 q=Reg&1;                    \
 Reg=(Reg>>1)|(R.AF.B.l<<7); \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SLL(Reg)           \
{                            \
 int q;                      \
 q=Reg>>7;                   \
 Reg=(Reg<<1)|1;             \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SLA(Reg)           \
{                            \
 int q;                      \
 q=Reg>>7;                   \
 Reg<<=1;                    \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SRL(Reg)           \
{                            \
 int q;                      \
 q=Reg&1;                    \
 Reg>>=1;                    \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SRA(Reg)           \
{                            \
 int q;                      \
 q=Reg&1;                    \
 Reg=(Reg>>1)|(Reg&0x80);    \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}

