Analysis of an exception
```
     _xt_user_exc , _UserExceptionVector , This are normally called

(gdb) layout next
(gdb) layout next
(gdb) b _xt_user_exc


    Analysis
    0x40080340 <_UserExceptionVector>       wsr.excsave1   a0
    0x40080343 <_UserExceptionVector+3>     call0  0x40081c60 <_xt_user_exc> 

(gdb) p/x $a0   $2 = 0x8011aeb3
(gdb) p/x $a1  $3 = 0x3ffbf460
(gdb) info symbol 0x4011aeb3
(gdb) n
    0x40081c60 <_xt_user_exc>       rsr.exccause   a0            
    0x40081c63 <_xt_user_exc+3>     bnei   a0, 4, 0x40081c69 <_xt_user_exc+9>    // 4 is Level1Interrupt        
    0x40081c66 <_xt_user_exc+6>     j      0x40081ebc <_xt_lowint1>           
    0x40081c69 <_xt_user_exc+9>     bgeui  a0, 32, 0x40081c5c <_xt_to_coproc_exc> 
    0x40081c6c <_xt_user_exc+12>    beqi   a0, 5, 0x40081c54 <_xt_to_alloca_exc> 
    0x40081c6f <_xt_user_exc+15>    beqi   a0, 1, 0x40081c58 <_xt_to_syscall_exc>            
    0x40081c72 <_xt_user_exc+18>    mov.n  a0, a1
    0x40081c74 <_xt_user_exc+20>    addmi  a1, a1, 0xffffff00    // -0x100                      
  > 0x40081c77 <_xt_user_exc+23>    addi   a1, a1, 64                                                0x40081c7a <_xt_user_exc+26>    s32i.n a0, a1, 16 
    0x40081c7c <_xt_user_exc+28>    s32e   a0, a1, -12                                             0x40081c7f <_xt_user_exc+31>    rsr.ps a0                    
  0x40081c82 <_xt_user_exc+34>    s32i.n a0, a1, 8                                      
    0x40081c84 <_xt_user_exc+36>    rsr.epc1       a0                       
    0x40081c87 <_xt_user_exc+39>    s32i.n a0, a1, 4                      
    0x40081c89 <_xt_user_exc+41>    s32e   a0, a1, -16                       
    0x40081c8c <_xt_user_exc+44>    s32i.n a12, a1, 60     
    0x40081c8e <_xt_user_exc+46>    s32i   a13, a1, 64                       
    0x40081c91 <_xt_user_exc+49>    call0  0x40085c40 <_xt_context_save>                       
    0x40081c94 <_xt_user_exc+52>    rsr.exccause   a0                      
    0x40081c97 <_xt_user_exc+55>    s32i   a0, a1, 80                
    0x40081c9a <_xt_user_exc+58>    rsr.excvaddr   a0                      
    0x40081c9d <_xt_user_exc+61>    s32i   a0, a1, 84                      
    0x40081ca0 <_xt_user_exc+64>    rsr.excsave1   a0                      
    0x40081ca3 <_xt_user_exc+67>    s32i.n a0, a1, 12                         
    0x40081ca5 <_xt_user_exc+69>    l32r   a0, 0x4008059c                       
    0x40081ca8 <_xt_user_exc+72>    wsr.ps a0
    0x40081cab <_xt_user_exc+75>    rsr.epc1       a0                     
    0x40081cae <_xt_user_exc+78>    l32r   a5, 0x400805a0                      
    0x40081cb1 <_xt_user_exc+81>    rsync
    0x40081cb4 <_xt_user_exc+84>    or     a0, a0, a5 
    0x40081cb7 <_xt_user_exc+87>    addx2  a0, a5, a0
    0x40081cba <_xt_user_exc+90>    rsr.exccause   a2
    0x40081cbd <_xt_user_exc+93>    rsr.exccause   a2 
    0x40081cc0 <_xt_user_exc+96>    l32r   a3, 0x400805a4                  
    0x40081cc3 <_xt_user_exc+99>    addx4  a4, a2, a3                                             0x40081cc6 <_xt_user_exc+102>   l32i.n a4, a4, 0   
    0x40081cc8 <_xt_user_exc+104>   mov.n  a6, a1
    0x40081cca <_xt_user_exc+106>   callx4 a4          // xt_unhandled_exception
    0x40081ccd <_xt_user_exc+109>   call0  0x40085ca8 <_xt_context_restore>

void xt_unhandled_exception(XtExcFrame *frame)  {

    (gdb) p frame
    $21 = (XtExcFrame *) 0x3ffb9e60
    (gdb) p/x $a2
    $22 = 0x3ffb9e60
    (gdb) p/x *frame

$24 = {exit = 0x80101eb6, pc = 0x400ff937, ps = 0x60530, a0 = 0x800d0eb1, a1 = 0x3ffb9f20, a2 = 0x0, a3 = 0x5,
  a4 = 0x60023, a5 = 0x3ffaf04c, a6 = 0x1, a7 = 0x0, a8 = 0xff, a9 = 0x3ffb9f00, a10 = 0x13, a11 = 0x3ffc14b0,
  a12 = 0x4000, a13 = 0x0, a14 = 0x11, a15 = 0x0, sar = 0x0, exccause = 0x1d, excvaddr = 0x0, lbeg = 0x4000c2e0,
  lend = 0x4000c2f6, lcount = 0x0, tmp0 = 0x4000, tmp1 = 0x0, tmp2 = 0x40081c94}
(gdb) where
#0  xt_unhandled_exception (frame=0x3ffb9e60) at /home/olas/esp/esp-idf/components/esp32/./panic.c:266
#1  0x40081ccd in _xt_user_exc ()
#2  0x400ff937 in app_main () at /home/olas/esp/qemu_esp32/examples/28_gdb_blackmagic/main/./main.c:213
  212              int *ptr=0;                   
  213             *ptr=0xff; 
    0x400ff932 <app_main+186>       movi   a8, 255                                        
    0x400ff935 <app_main+189>       movi.n a2, 0    
    0x400ff937 <app_main+191>       s32i.n a8, a2, 0   


}


Guru Meditation Error of type StoreProhibited occurred on core  0HOST RER TBD
. Exception was unhandled.
Register dump:
PC      : 0x400ff937  PS      : 0x00060530  A0      : 0x800d0eb1  A1      : 0x3ffb9f20  
A2      : 0x00000000  A3      : 0x00000005  A4      : 0x00060023  A5      : 0x3ffaf04c  
A6      : 0x00000001  A7      : 0x00000000  A8      : 0x000000ff  A9      : 0x3ffb9f00  
A10     : 0x00000013  A11     : 0x3ffc14b0  A12     : 0x00004000  A13     : 0x00000000  
A14     : 0x00000011  A15     : 0x00000000  SAR     : 0x00000000  EXCCAUSE: 0x0000001d  
EXCVADDR: 0x00000000  LBEG    : 0x4000c2e0  LEND    : 0x4000c2f6  LCOUNT  : 0x00000000  

Backtrace: 0x400ff937:0x3ffb9f20 0x400d0eae:0x3ffb9f50

CPU halted.


 p/x $sp
 $7 = 0x3ffbf460

 >  0x40081ebc <_xt_lowint1>        mov.n  a0, a1                                                     0x40081ebe <_xt_lowint1+2>      addmi  a1, a1, 0xffffff00
    0x40081ec1 <_xt_lowint1+5>      addi   a1, a1, 64     
    0x40081ec4 <_xt_lowint1+8>      s32i.n a0, a1, 16
    0x40081ec6 <_xt_lowint1+10>     rsr.ps a0
    0x40081ec9 <_xt_lowint1+13>     s32i.n a0, a1, 8                                       0x40081ecb <_xt_lowint1+15>     rsr.epc1       a0 

    0x40085a8c <_frxt_int_enter>    s32i   a12, a1, 60
    0x40085a8f <_frxt_int_enter+3>  s32i   a13, a1, 64 
    (gdb) p/x $a0

(gdb) p/x $a1
$15 = 0x40081edd
(gdb) p/x $a1
$16 = 0x3ffbf3a0
(gdb) p/x $sp
$17 = 0x3ffbf3a0

(gdb) p/x $a12
$18 = 0x3ffb54d0

     0x40085a92 <_frxt_int_enter+6>  or     a12, a0, a0
(gdb) ni
0x40085a95 in _frxt_int_enter ()
(gdb) p/x $a12
$22 = 0x40081edd

     0x40085a95 <_frxt_int_enter+9>  call0  0x40085c40 <_xt_context_save>                



(gdb) p/x $a1
$26 = 0x3ffc01d0
(gdb) p/x $sp
$27 = 0x3ffc01d0
(gdb) p/x $s2
$28 = Value can't be converted to integer.
(gdb) p/x $a2
(gdb) si

   0x40085b9d <_frxt_dispatch+61>  call0  0x40085ca8 <_xt_context_restore> 

(gdb) p/x $a0
$37 = 0x40085ba0
(gdb) p/x $a1
$38 = 0x3ffc01d0


    0x40085ca8 <_xt_context_restore>        mov.n  a13, a0                                 
    0x40085caa <_xt_context_restore+2>      addi   a2, a1, 112                       
    0x40085cad <_xt_context_restore+5>      call0  0x40092c74 <xthal_restore_extra_nw>    0x40085cb0 <_xt_context_restore+8>      mov.n  a0, a13                                         0x40085cb2 <_xt_context_restore+10>     l32i   a2, a1, 88                                      0x40085cb5 <_xt_context_restore+13>     l32i   a3, a1, 92                                     0x40085cb8 <_xt_context_restore+16>     wsr.lbeg       a2       


// Return with rfe
    0x40081cf0 <_xt_user_exit+12>   l32i.n a1, a1, 16                       
  > 0x40081cf2 <_xt_user_exit+14>   rsync
    0x40081cf5 <_xt_user_exit+17>   rfe                   


```

Source

```

_xt_context_save:

    s32i    a2,  sp, XT_STK_A2
    s32i    a3,  sp, XT_STK_A3
    s32i    a4,  sp, XT_STK_A4
    s32i    a5,  sp, XT_STK_A5
    s32i    a6,  sp, XT_STK_A6
    s32i    a7,  sp, XT_STK_A7
    s32i    a8,  sp, XT_STK_A8
    s32i    a9,  sp, XT_STK_A9
    s32i    a10, sp, XT_STK_A10
    s32i    a11, sp, XT_STK_A11








 0x40085cda <_xt_context_restore+50>     l32i.n a10, a1, 52                                        0x40085cdc <_xt_context_restore+52>     l32i.n a11, a1, 56                                        > 0x40085cde <_xt_context_restore+54>   l32i.n a12, a1, 60                                      0x40085ce0 <_xt_context_restore+56>     l32i   a13, a1, 64                                        0x40085ce3 <_xt_context_restore+59>     l32i   a14, a1, 68                                        0x40085ce6 <_xt_context_restore+62>     l32i   a15, a1, 72                                        0x40085ce9 <_xt_context_restore+65>     ret.n                       

```


```
_xt_lowint1:
    mov     a0, sp                          /* sp == a1 */
    addi    sp, sp, -XT_STK_FRMSZ           /* allocate interrupt stack frame */
    s32i    a0, sp, XT_STK_A1               /* save pre-interrupt SP */
    rsr     a0, PS                          /* save interruptee's PS */
    s32i    a0, sp, XT_STK_PS
    rsr     a0, EPC_1                       /* save interruptee's PC */
    s32i    a0, sp, XT_STK_PC
    rsr     a0, EXCSAVE_1                   /* save interruptee's a0 */
    s32i    a0, sp, XT_STK_A0
    movi    a0, _xt_user_exit               /* save exit point for dispatch */
    s32i    a0, sp, XT_STK_EXIT

    /* Save rest of interrupt context and enter RTOS. */
    call0   XT_RTOS_INT_ENTER               /* common RTOS interrupt entry */

```


```
_xt_panic:
    /* Allocate exception frame and save minimal context. */
    mov     a0, sp
    addi    sp, sp, -XT_STK_FRMSZ
    s32i    a0, sp, XT_STK_A1
    #if XCHAL_HAVE_WINDOWED
    s32e    a0, sp, -12                     /* for debug backtrace */
    #endif
    rsr     a0, PS                          /* save interruptee's PS */
    s32i    a0, sp, XT_STK_PS
    rsr     a0, EPC_1                       /* save interruptee's PC */
    s32i    a0, sp, XT_STK_PC
    #if XCHAL_HAVE_WINDOWED
    s32e    a0, sp, -16                     /* for debug backtrace */
    #endif
    s32i    a12, sp, XT_STK_A12             /* _xt_context_save requires A12- */
    s32i    a13, sp, XT_STK_A13             /* A13 to have already been saved */
    call0   _xt_context_save

    /* Save exc cause and vaddr into exception frame */
    rsr     a0, EXCCAUSE
    s32i    a0, sp, XT_STK_EXCCAUSE
    rsr     a0, EXCVADDR
    s32i    a0, sp, XT_STK_EXCVADDR

    /* _xt_context_save seems to save the current a0, but we need the interuptees a0. Fix this. */
    rsr     a0, EXCSAVE_1                   /* save interruptee's a0 */

    s32i    a0, sp, XT_STK_A0

    /* Set up PS for C, disable all interrupts except NMI and debug, and clear EXCM. */
    movi    a0, PS_INTLEVEL(5) | PS_UM | PS_WOE
    wsr     a0, PS

    //Call panic handler
    mov     a6,sp
    call4 panicHandler
```
