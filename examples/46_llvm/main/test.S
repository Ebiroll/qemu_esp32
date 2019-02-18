	.text
	.file	"add.c"
	.global	add
	.p2align	2
	.type	add,@function
add:
	entry	sp, 56
	mov.n	a8, a3
	mov.n	a9, a2
	s32i.n	a2, sp, 16
	s32i.n	a3, sp, 12
	l32i.n	a3, sp, 16
	l32i.n	a2, sp, 12
	add.n	a2, a3, a2
	s32i.n	a8, sp, 8
	s32i.n	a9, sp, 4
	retw.n
Lfunc_end0:
	.size	add, Lfunc_end0-add


