;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;%  Copyright (C) 1988-1993 by WATCOM International Corp. All	%
;%  rights reserved. No part of this software may be reproduced %
;%  in any form or by any means - graphic, electronic or	%
;%  mechanical, including photocopying, recording, taping	%
;%  or information storage and retrieval systems - except	%
;%  with the written permission of WATCOM International Corp.	%
;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;
; startup code for WATCOM C/C++32 under Microsoft Windows
;
;	This must be assembled using one of the following commands:
;		wasm cstrtw32 -bt=WINDOWS -mf -3r_
;		wasm cstrtw32 -bt=WINDOWS -mf -3s_
;
.387
.386p

DGROUP group CONST,_DATA,DATA,XIB,XI,XIE,YIB,YI,YIE,_BSS,STACK

extrn __DOSseg__:byte

	extrn	WINMAIN : near

cFarProc MACRO name
public "C",name
name	label FWORD
	      dd 0
	      dw 0
ENDM

FarProc MACRO name
public name
name	label FWORD
	      dd 0
	      dw 0
ENDM

	assume	nothing

	extrn	_edata		: byte		; end of DATA (start of BSS)
	extrn	_end		: byte		; end of BSS (start of STACK)

	extrn	GETMODULEFILENAME : near
	extrn	WEP		: near
	extrn	__InitRtns	: near
	extrn	__FiniRtns	: near
	extrn	"C",exit_	: near

; this guarantees that no function pointer will equal NULL
; (WLINK will keep segment 'BEGTEXT' in front)
; This segment must be at least 4 bytes in size to avoid confusing the
; signal function.

BEGTEXT segment use32 word public 'CODE'
        assume  cs:BEGTEXT
forever: jmp    short forever
___begtext label byte
        nop
        nop
        nop
        nop
	public ___begtext
        assume  cs:nothing
BEGTEXT ends

_TEXT	segment use32 word public 'CODE'

XIB	segment word public 'DATA'
XIB	ends
XI	segment word public 'DATA'
XI	ends
XIE	segment word public 'DATA'
XIE	ends

YIB	segment word public 'DATA'
YIB	ends
YI	segment word public 'DATA'
YI	ends
YIE	segment word public 'DATA'
YIE	ends

_DATA	segment use32 word public 'DATA'

MAX_FILE_NAME	equ	144
TMPSTACK	equ	__STACKLOW ; location on stack to copy command line
;
; these variables must all be kept in order
;
_LocalPtr   dw	0
public _LocalPtr
hInstance  dw  0
hPrevInstance  dw  0
lpCmdLine  dd  0
cmdShow    dw  0
__no87	   dw 0 		; non-zero => "NO87" environment var present

; magical selectors for real memory
public ___A000,___B000,___B800,___C000,___D000,___E000,___F000
___A000 dw	0
___B000 dw	0
___B800 dw	0
___C000 dw	0
___D000 dw	0
___E000 dw	0
___F000 dw	0

; data ptrs
cFarProc __CodeSelectorBaseAddr
cFarProc __DataSelectorBaseAddr 
FarProc __32BitCallBackAddr
cFarProc __DLLEntryAddr
cFarProc __WEPAddr

public	__16BitCallBackAddr
__16BitCallBackAddr dd 0

; proc lists
FarProc Invoke16BitFunctionAddr
FarProc __INT21ADDR
FarProc __WIN16THUNK1ADDR
FarProc __WIN16THUNK2ADDR
FarProc __WIN16THUNK3ADDR
FarProc __WIN16THUNK4ADDR
FarProc __WIN16THUNK5ADDR
FarProc __WIN16THUNK6ADDR
FarProc __Call16Addr

__LpCmdLine dd 0		; pointer to raw command line
__LpPgmName dd 0		; pointer to program name (for argv[0])
__STACKLOW dd 0 		; lowest address in stack
__STACKTOP dd 0 		; highest address in stack
__ASTACKSIZ dd 0		; alternate stack size
__ASTACKPTR dd 0		; alternate stack pointer
__curbrk   dd 0 		; top of usable memory
__cbyte    dd 0 		; used by getch, getche
__osmajor  db 4 		; major DOS version number
__osminor  db 0 		; minor DOS version number
__Extender db 0 		; 10 => 386 windows
__init_387_emulator db 0	; to prevent emulator from coming in with
				;	-fpi

public	"C",__pid
__pid	dw	0
public	"C",__wincmdptr
__wincmdptr LABEL FWORD
cmd_offset dd 0
cmd_seg    dw 0
filename   db MAX_FILE_NAME dup(0)
__inDLL	db	0		; 0 => ordinary EXE, non-zero => DLL

 __FPE_handler label dword
___FPE_handler dd __null_FPE_rtn ; FPE handler

	public	"C",__LpCmdLine
	public	"C",__LpPgmName
	public	"C",__STACKLOW
	public	"C",__STACKTOP
        public  __ASTACKSIZ
        public  __ASTACKPTR
	public	"C",__curbrk
	public	"C",__cbyte
	public	"C",__osmajor
	public	"C",__osminor
	public	"C",__Extender
	public	__no87
	public	 __FPE_handler
	public	___FPE_handler
	public	__init_387_emulator

_DATA	ends

STACK_SIZE	equ	2000h

STACK	segment para stack 'STACK'
	db	(STACK_SIZE) dup(?)
STACK	ends


_BSS	      segment word public 'BSS'
_BSS	      ends

CONST	segment word public 'DATA'
CONST	ends

DATA	segment word public 'DATA'
DATA	ends

	assume	cs:_TEXT
	assume	ds:_DATA
	assume	fs:_DATA
	assume	gs:_DATA
	assume	ss:_DATA

__saved_DS dw 0
public _cstart_
public _wstart_
public __DLLstart_
_cstart_ proc  far
_wstart_:
__DLLstart_:
	dd	offset	hInstance ; loader starts execution 8 bytes past here
	dd	_end
	mov	_LocalPtr,gs		; save selector of extender's data
	mov	__inDLL,bl		; save DLL indicator flag

	mov	__STACKTOP,esp		; set stack top
	mov	__STACKLOW,ecx		; and stack low

	; initialize bss
	lea	ecx,DGROUP:_end 	; end of _BSS segment (start of free)
	lea	edi,DGROUP:_edata	; start of _BSS segment
	sub	ecx,edi 		; calc # of bytes in _BSS segment
	xor	eax,eax 		; zero the _BSS segment
	mov	dl,cl			; copy the lower bits of size
	shr	ecx,2			; get number of dwords
	rep	stosd			; copy them
	mov	cl,dl			; get lower bits
	and	cl,3			; get number of bytes left (modulo 4)
	rep	stosb

	; save ds
	mov	ecx,offset __saved_DS
	mov	ds:[ecx],ds

	; set up heap
	lea	edx,DGROUP:_end 	; start of free
	add	edx,3
	and	edx,not 3
	mov	__curbrk,edx

	push	es			; save es
	les	ecx,__WEPAddr		; get addr of pointer to fill in
	mov	eax,offset __WEP	; get addr of WEP routine
	mov	es:[ecx],eax		; so supervisor knows
	pop	es			; restore es

	; put command string on the stack
	push	es
	mov	ebx,TMPSTACK
	mov	__LpCmdLine,ebx		; for use by initargv
	mov	esi,lpCmdLine		; offset + selector
	mov	edx,esi 		; get the
	shr	edx,10h 		;   selector
	cmp	edx,0			; is it zero?
	jne	short okcpy		; no, do the copy
	mov	byte ptr ds:[ebx],0	; put a trailing zero
	jmp	short donecpy
okcpy:	mov	es,dx
	mov	ds:cmd_seg,es		; save for later
	movzx	esi,si			;    use by getcmd
	mov	ds:cmd_offset,esi
again:	mov	al,byte ptr es:[esi]
	mov	byte ptr ds:[ebx],al
	cmp	al,0
	je	short donecpy
	inc	esi
	inc	ebx
	jmp	short again
donecpy:pop	es

	movzx	eax,hInstance
	push	eax
	mov	edi, offset filename
	mov	__LpPgmName,edi
	push	edi
	push	MAX_FILE_NAME
	call	GETMODULEFILENAME

	mov	eax,0ffh		; run all initializers
	call	__InitRtns		; call initializer routines

;;	/* allocate alternate stack for F77 */
;;	_ASTACKPTR = (char *)alloca( _ASTACKSIZ ) + _ASTACKSIZ;
	cmp	byte ptr __inDLL,0	; if in DLL
	je	short not_dll		; then
	  mov	eax,__STACKLOW		; - put alternate stack on bottom
	  add	eax,__ASTACKSIZ		; - ...
	  mov	__ASTACKPTR,eax		; - ...
	  jmp	short not_dll2		; else
not_dll:mov	__ASTACKPTR,esp		; - save address of alternate stack
	sub	esp,__ASTACKSIZ		; - allocate alternate stack for F77
not_dll2:				; endif

	; push parms for WINMAIN
	mov	ax,hInstance
	mov	__pid,ax		; save for use by getpid()
	movzx	eax,ax
	push	eax
	mov	ax,hPrevInstance
	movzx	eax,ax
	push	eax
	mov	eax,TMPSTACK
	push	eax
	mov	ax,cmdShow
	movzx	eax,ax
	push	eax

	call	WINMAIN
	jmp	exit_			; exit
	dw	___begtext		; make sure dead code elimination
					; doesn't kill BEGTEXT segment
_cstart_ endp


__exit_  proc far
	public	"C",__exit_
ifdef __STACK__
	pop	eax			; get return code into eax
endif
	cmp	byte ptr __inDLL,0	; are we in a DLL?
	jne	short skip_fini		; skip fini rtns if we are
	push	eax			; save return value
	push	edx			; save edx
	mov	eax,00h			; run all finalizers
	mov	edx,0ffh		; run all finalizers
	call	__FiniRtns		; call finalizer routines
	pop	edx			; restore edx
	pop	eax			; restore return value
skip_fini:
	mov	esp,__STACKTOP		; reset stack pointer
	mov	ds,_LocalPtr		; restore ds
	ret
__exit_ endp

__null_FPE_rtn proc near
	ret				; return
__null_FPE_rtn endp

__WEP	proc	far
	push	eax			; push parm
	push	cs			; simulate far call
	call	WEP			; call user's WEP function
	push	edx			; save edx
	mov	eax,00h			; run all finalizers
	mov	edx,0ffh		; run all finalizers
	call	__FiniRtns		; call finalizer routines
	pop	edx			; restore edx
	mov	ds,_LocalPtr		; restore ds
	ret				; return
__WEP	endp

public __GETDS
__GETDS proc near
	mov	ds,cs:__saved_DS
	ret
__GETDS endp

public	__Int21
public	__Int21_
__Int21	proc	near
__Int21_:push	ebp			; save ebp
	mov	ds,_LocalPtr		; load extenders data segment
	call	fword ptr es:__INT21ADDR; call extender
	pop	ebp			; restore ebp
	ret				; return
__Int21	endp

_TEXT	ends

	end _cstart_
