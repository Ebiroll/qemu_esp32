/******************************************************************************
 * Copyright 2015 Espressif Systems
 *
 * Description: Assembly routines for the gdbstub
 *
 * License: ESPRESSIF MIT License
 *******************************************************************************/

#include "gdbstub-cfg.h"

#include <xtensa/config/specreg.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

#define DEBUG_PC			(EPC + XCHAL_DEBUGLEVEL)
#define DEBUG_EXCSAVE		(EXCSAVE + XCHAL_DEBUGLEVEL)
#define DEBUG_PS			(EPS + XCHAL_DEBUGLEVEL)

.global gdbstub_savedRegs
.global debug_exception_handler
.global gdbstub_exception_stack

// part of esp-open-rtos
// contains a0 - a4
.global debug_saved_ctx

	.text
.literal_position

	.text
	.align	4

/*
The savedRegs struct:
	0x00 uint32_t pc;
	0x04 uint32_t ps;
	0x08 uint32_t sar;
	0x0C uint32_t vpri;
	0x10 uint32_t a0;
	0x14 uint32_t a[14]; //a2..a15
	     uint32_t litbase;
	     uint32_t sr176;
	     uint32_t sr208;
	     uint32_t a1;
	     uint32_t reason;
*/

// This is the debugging exception routine; it's called by the debugging vector
debug_exception_handler:
	// a0 - a4 are in debug_saved_ctx
	// move them to gdbstub struct and save the rest
	// esp32 movi    a1, debug_saved_ctx
	//esp32 movi	a2, gdbstub_savedRegs

	// a0
	l32i	a0, a1, 0x00
	s32i    a0, a2, 0x10

	// a1
	l32i	a0, a1, 0x04
	s32i    a0, a2, 0x58

	// a2 - a4
	l32i	a0, a1, 0x08
	s32i    a0, a2, 0x14

	l32i	a0, a1, 0x0c
	s32i    a0, a2, 0x18

	l32i	a0, a1, 0x10
	s32i    a0, a2, 0x1c

	// the rest of registers is still intact
	s32i	a5, a2, 0x20
	s32i	a6, a2, 0x24
	s32i	a7, a2, 0x28
	s32i	a8, a2, 0x2c
	s32i	a9, a2, 0x30
	s32i	a10, a2, 0x34
	s32i	a11, a2, 0x38
	s32i	a12, a2, 0x3c
	s32i	a13, a2, 0x40
	s32i	a14, a2, 0x44
	s32i	a15, a2, 0x48
	rsr		a0, SAR
	s32i	a0, a2, 0x08
	// esp32 rsr		a0, LITBASE
	s32i	a0, a2, 0x4C
	rsr		a0, 176
	s32i	a0, a2, 0x50
//	rsr		a0, 208
//	s32i	a0, a2, 0x54
	rsr		a0, DEBUGCAUSE
	s32i	a0, a2, 0x5C
	rsr		a4, DEBUG_PC
	s32i	a4, a2, 0x00
	rsr		a4, DEBUG_PS
	s32i	a4, a2, 0x04

	// Move to our own stack
	movi	a1, gdbstub_exception_stack + 255 * 4

// If ICOUNT is -1, disable it by setting it to 0, otherwise we will keep triggering on the same instruction.
	rsr		a2, ICOUNT
	movi	a3, -1
	bne		a2, a3, .no_icount_reset
	movi	a3, 0
	wsr		a3, ICOUNT
.no_icount_reset:

	rsr		a2, ps
	addi	a2, a2, -PS_EXCM_MASK
	wsr		a2, ps
	rsync

// Call into the C code to do the actual handling.
	call0	gdbstub_handle_debug_exception

	rsr		a2, ps
	addi	a2, a2, PS_EXCM_MASK
	wsr		a2, ps
	rsync

	// Restore registers from the gdbstub_savedRegs struct
	movi	a2, gdbstub_savedRegs

	// PC
	l32i	a0, a2, 0x00
	wsr		a0, DEBUG_PC

	// PS
	l32i	a0, a2, 0x04
	wsr		a0, DEBUG_PS

//	l32i	a0, a2, 0x54
//	wsr		a0, 208

	l32i	a0, a2, 0x50
	wsr		a0, 176

	l32i	a0, a2, 0x4C
	// esp32 wsr		a0, LITBASE

	l32i	a0, a2, 0x08
	wsr		a0, SAR

	// a15 - a3
	l32i	a15, a2, 0x48
	l32i	a14, a2, 0x44
	l32i	a13, a2, 0x40
	l32i	a12, a2, 0x3c
	l32i	a11, a2, 0x38
	l32i	a10, a2, 0x34
	l32i	a9, a2, 0x30
	l32i	a8, a2, 0x2c
	l32i	a7, a2, 0x28
	l32i	a6, a2, 0x24
	l32i	a5, a2, 0x20
	l32i	a4, a2, 0x1c
	l32i	a3, a2, 0x18

	// a0
	l32i	a0, a2, 0x10

	// a1
	l32i	a1, a2, 0x58

	// a2
	l32i	a2, a2, 0x14

	//All done. Return to where we came from.
	rfi		XCHAL_DEBUGLEVEL

#if GDBSTUB_FREERTOS
/*
FreeRTOS exception handling code. For some reason or another, we can't just hook the main exception vector: it
seems FreeRTOS uses that for something else too (interrupts). FreeRTOS has its own fatal exception handler, and we
hook that. Unfortunately, that one is called from a few different places (eg directly in the DoubleExceptionVector)
so the precise location of the original register values are somewhat of a mystery when we arrive here... 

As a 'solution', we'll just decode the most common case of the user_fatal_exception_handler being called from
the user exception handler vector:
- excsave1 - orig a0
- a1: stack frame:
	sf+16: orig a1
	sf+8: ps
	sf+4: epc
	sf+12: orig a0
	sf: magic no?
*/
	.global gdbstub_handle_user_exception
	.global gdbstub_user_exception_entry
	.align	4
gdbstub_user_exception_entry:
// Save all regs to structure
	movi	a0, gdbstub_savedRegs
	s32i	a1, a0, 0x14 //was a2
	s32i	a3, a0, 0x18
	s32i	a4, a0, 0x1c
	s32i	a5, a0, 0x20
	s32i	a6, a0, 0x24
	s32i	a7, a0, 0x28
	s32i	a8, a0, 0x2c
	s32i	a9, a0, 0x30
	s32i	a10, a0, 0x34
	s32i	a11, a0, 0x38
	s32i	a12, a0, 0x3c
	s32i	a13, a0, 0x40
	s32i	a14, a0, 0x44
	s32i	a15, a0, 0x48
	rsr		a2, SAR
	s32i	a2, a0, 0x08
	rsr		a2, LITBASE
	s32i	a2, a0, 0x4C
	rsr		a2, 176
	s32i	a2, a0, 0x50
	rsr		a2, 208
	s32i	a2, a0, 0x54
	rsr		a2, EXCCAUSE
	s32i	a2, a0, 0x5C

// Get the rest of the regs from the stack struct
	l32i	a3, a1, 12
	s32i	a3, a0, 0x10
	l32i	a3, a1, 16
	s32i	a3, a0, 0x58
	l32i	a3, a1, 8
	s32i	a3, a0, 0x04
	l32i	a3, a1, 4
	s32i	a3, a0, 0x00
	movi    a1, gdbstub_exception_stack + 255 * 4

	rsr		a2, ps
	addi	a2, a2, -PS_EXCM_MASK
	wsr		a2, ps
	rsync

	call0	gdbstub_handle_user_exception

UserExceptionExit:

/*
 Okay, from here on, it Does Not Work. There's not really any continuing from an exception in the
 FreeRTOS case; there isn't any effort put in reversing the mess the exception code made yet. Maybe this
 is still something we need to implement later, if there's any demand for it, or maybe we should modify
 FreeRTOS to allow this in the future. (Which will then kill backwards compatibility... hmmm.)
*/
	j UserExceptionExit
#endif

	.global gdbstub_save_extra_sfrs_for_exception
	.align 4
// The  Xtensa OS HAL does not save all the special function register things. This bit of assembly
// fills the gdbstub_savedRegs struct with them.
gdbstub_save_extra_sfrs_for_exception:
	movi	a2, gdbstub_savedRegs
	// esp32 rsr		a3, LITBASE
	s32i	a3, a2, 0x4C
	rsr		a3, 176
	s32i	a3, a2, 0x50
	rsr		a3, 208
	s32i	a3, a2, 0x54
	rsr		a3, EXCCAUSE
	s32i	a3, a2, 0x5C
	ret

	.global gdbstub_init_debug_entry
	.align	4

// These routines all assume only one breakpoint and watchpoint is available, which
// is the case for the ESP8266 Xtensa core.

	.global gdbstub_set_hw_breakpoint
gdbstub_set_hw_breakpoint:
	// a2 - addr, a3 - len (unused here)
	rsr		a4, IBREAKENABLE
	bbsi	a4, 0, return_w_error
	wsr		a2, IBREAKA
	movi	a2, 1
	wsr		a2, IBREAKENABLE
	isync
	movi 	a2, 1
	ret

	.global gdbstub_del_hw_breakpoint
gdbstub_del_hw_breakpoint:
	// a2 - addr
	rsr		a5, IBREAKENABLE
	bbci	a5, 0, return_w_error
	rsr		a3, IBREAKA
	bne		a3, a2, return_w_error
	movi	a2, 0
	wsr		a2, IBREAKENABLE
	isync
	movi	a2, 1
	ret

	.global gdbstub_set_hw_watchpoint
	// a2 - addr, a3 - mask, a4 - type (1=read, 2=write, 3=access)
gdbstub_set_hw_watchpoint:
	// Check if any of the masked address bits are set. If so, that is an error.
	movi	a5,0x0000003F
	xor		a5, a5, a3
	bany	a2, a5, return_w_error
	// Check if watchpoint already is set
	rsr		a5, DBREAKC
	movi	a6, 0xC0000000
	bany	a6, a5, return_w_error
	// Set watchpoint
	wsr		a2, DBREAKA
	
	// Combine type and mask
	movi	a6, 0x3F
	and		a3, a3, a6
	slli	a4, a4, 30
	or		a3, a3, a4
	wsr		a3, DBREAKC
	
//	movi	a2, 1
	mov		a2, a3
	isync
	ret

	.global gdbstub_del_hw_watchpoint
	// a2 - addr
gdbstub_del_hw_watchpoint:
	// See if the address matches
	rsr		a3, DBREAKA
	bne		a3, a2, return_w_error

	// See if the bp actually is set
	rsr		a3, DBREAKC
	movi	a2, 0xC0000000
	bnone	a3, a2, return_w_error
	// Disable bp
	movi	a2, 0
	wsr		a2, DBREAKC
	movi	a2, 1
	isync
	ret

return_w_error:
	movi	a2, 0
	ret
