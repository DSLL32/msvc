;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;%  Copyright (C) 1988-1994 by WATCOM International Corp. All	%
;%  rights reserved. No part of this software may be reproduced %
;%  in any form or by any means - graphic, electronic or	%
;%  mechanical, including photocopying, recording, taping	%
;%  or information storage and retrieval systems - except	%
;%  with the written permission of WATCOM International Corp.	%
;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;
; startup code for WATCOM C/C++16 under Microsoft Windows
;
;	This must be assembled using one of the following commands:
;		wasm cstrtw16 -bt=WINDOWS -ms -0r_
;		wasm cstrtw16 -bt=WINDOWS -mm -0r_
;		wasm cstrtw16 -bt=WINDOWS -mc -0r_
;		wasm cstrtw16 -bt=WINDOWS -ml -0r_
;
include mdef.inc

public	pLocalHeap
public	pAtomTable
public	pStackTop
public	pStackMin
public	pStackBot

MAX_FILE_NAME	equ	144
pLocalHeap	equ	0006H
pAtomTable	equ	0008H
pStackTop	equ	000AH
pStackMin	equ	000CH
pStackBot	equ	000EH

	name	wstart

	assume	nothing

	extrn	INITTASK	: far
	extrn	GETMODULEFILENAME : far
	extrn	INITAPP 	: far
	extrn	WAITEVENT	: far
	extrn	__AHSHIFT	: word

	extrn	"C",exit_	: far
	extrn	__FInitRtns	: far
	extrn	__FFiniRtns	: far
	
if _MODEL and _BIG_CODE
	extrn	WINMAIN 	: far
else
	extrn	WINMAIN 	: near
endif

	extrn	_edata		: byte		; end of DATA (start of BSS)
	extrn	_end		: byte		; end of BSS (start of STACK)

	extrn	__DOSseg__	: byte

 DGROUP group _NULL,AFX_NULL,_DATA,CONST,STRINGS,DATA,BCSD,XIB,XI,XIE,YIB,YI,YIE,_BSS,STACK

ife _MODEL and _BIG_CODE

if _MODEL NE _TINY
; this guarantees that no function pointer will equal NULL
; (WLINK will keep segment 'BEGTEXT' in front)
; This segment must be at least 4 bytes in size to avoid confusing the
; signal function.
; need a symbol defined here to prevent the dead code elimination from
; eliminating the segment.
; (the int 3h is useful for quickly revealing jumps to NULL code pointers)

BEGTEXT  segment word public 'CODE'
	assume	cs:BEGTEXT
forever	label	near
	int	3h
	jmp	short forever
___begtext label byte
	nop
	nop
	nop
	nop
	public ___begtext
	assume	cs:nothing
BEGTEXT  ends

endif
endif

_TEXT	segment word public 'CODE'

FAR_DATA segment byte public 'FAR_DATA'
FAR_DATA ends

	assume	ds:DGROUP

_NULL	segment para public 'BEGDATA'
__nullarea label word
	   dw	0,0
	   dw	5
	   dw	0		; pLocalHeap
	   dw	0		; pAtomTable
__STACKLOW dw	0		; pStackTop: lowest address in stack
__STACKTOP dw	0		; pStackMin:
	   dw	0		; pStackBot: highest address in stack
	public	__nullarea
_NULL	ends

AFX_NULL    segment word public 'BEGDATA'
AFX_NULL    ends

_DATA	segment word public 'DATA'
__aaltstkovr dw -1		; alternate stack overflow routine address
__curbrk   dw 0 		; top of usable memory
__psp	   dw 0 		; segment addr of program segment prefix
__osmajor  db 0 		; major DOS version number
__osminor  db 0 		; minor DOS version number
__osmode   db 0 		; 0 => DOS real mode
__HShift   db 0 		; Huge Shift value
__cbyte    dw 0 		; used by getch, getche
__child    dw 0 		; non-zero => a spawned process is running
__no87	   dw 0 		; always try to use the 8087
__get_ovl_stack dw 0,0		; get overlay stack pointer
__restore_ovl_stack dw 0,0	; restore overlay stack pointer
 __FPE_handler label dword
___FPE_handler dw 0,0		; FPE handler
__LpCmdLine dw 0,0		; lpCmdLine (for _argc, _argv processing)
__LpPgmName dw 0,0		; lpPgmName (for _argc, _argv processing)
filename	db MAX_FILE_NAME dup(0)

	public	"C",__curbrk
	public	"C",__psp
	public	"C",__osmajor
	public	"C",__osminor
	public	__osmode
	public	"C",__STACKLOW
	public	"C",__STACKTOP
	public	"C",__cbyte
	public	"C",__child
	public	__no87
	public	__HShift
	public	__get_ovl_stack
	public	__restore_ovl_stack
	public	 __FPE_handler
	public	___FPE_handler
	public	"C",__LpCmdLine
	public	"C",__LpPgmName

_DATA	ends

CONST	segment word public 'DATA'
CONST	ends

STRINGS segment word public 'DATA'
STRINGS ends

DATA	segment word public 'DATA'
DATA	ends

BCSD	segment word public 'DATA'
BCSD	ends

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

_BSS	      segment word public 'BSS'
_BSS	      ends

STACK	segment para stack 'STACK'
STACK	ends

	assume	nothing
	public	_wstart_
	public	_cstart_

	assume	cs:_TEXT
	assume	ds:DGROUP

 _wstart_ proc near
 _cstart_:
	jmp	around

;
; copyright message
;
        db      "WATCOM C/C++16 Run-Time system. "
        db      "(c) Copyright by WATCOM International Corp. 1988-1994."
	db	' All rights reserved.'

ife _MODEL and _BIG_CODE
if _MODEL NE _TINY
	dw	___begtext		; make sure dead code elimination
endif					; doesn't kill BEGTEXT segment
endif

around: call	INITTASK		; initialize
	or	ax,ax			; if not OK
	je	_error			; then error
	mov	__psp,es		; save ES
	push	di			; push parms for WINMAIN (hInstance)
	push	si			; ... (hPrevInstance)
	push	es			; ... (lpCmdLine)
	push	bx			; ... (...)
	push	dx			; ... (cmdShow)
	
if _MODEL and _BIG_DATA
	mov	__LpCmdLine+0,bx	; stash lpCmdLine pointer
	mov	__LpCmdLine+2,es	; ...
else
;
;	copy command line into bottom of stack
;
	push	bx
	mov	si,offset DGROUP:_end	; end of _BSS segment (start of STACK)
	mov	__LpCmdLine+0,si	; stash lpCmdLine pointer
	mov	__LpCmdLine+2,ds	; ...
_loop:	mov	al,es:0[bx]
	mov	[si],al
	inc	si
	inc	bx
	cmp	al,0
	jne	_loop
	mov	__STACKLOW,si
	pop	bx
endif

	sub	ax,ax			; wait for events
	push	ax			; ...
	call	WAITEVENT		; ...
	push	di
	push	di			; initialize application
	call	INITAPP 		; ...

	cld				; set forward direction
	push	ds			; set es=ds
	pop	es			; ...
	mov	cx,offset DGROUP:_end	; end of _BSS segment (start of STACK)
	mov	di,offset DGROUP:_edata ; start of _BSS segment
	sub	cx,di			; calc # of bytes in _BSS segment
	mov	al,0			; zero the _BSS segment
	rep	stosb			; . . .
	mov	ax,offset __AHSHIFT	; get huge shift value
	mov	__HShift,al		; ...
	cmp	al,12			; real mode?
	je	notprot			; yes, so leave osmode alone
	mov	al,1
	mov	__osmode,al		; protected mode!
notprot:
	mov	ax,offset __null_FPE_rtn; initialize floating-point exception
	mov	___FPE_handler,ax	; ... handler address
	mov	ax,seg __null_FPE_rtn	; initialize floating-point exception
	mov	___FPE_handler+2,ax	; ...

	mov	ah,30h			; get DOS version number
	int	21h			; ...
	mov	__osmajor,al		; ...
	mov	__osminor,ah		; ...
	; hinst is already on the stack
	push	ds
	mov	di, offset filename
	push	di
	mov	ax, MAX_FILE_NAME
	push	ax
	call	GETMODULEFILENAME
	mov     ax,offset filename
	mov	__LpPgmName+0,di	; stash lpPgmName
	mov	__LpPgmName+2,ds	; ...
	
	mov	ax,0ffh			; run all initializers
	call	__FInitRtns		; call initializer routines

	call	WINMAIN 		; invoke user's program
	call	exit_			; exit
_wstart_ endp

__exit_  proc near
	public	"C",__exit_
_error:
	push	ax			; save return code
	mov	ax,00h			; run all finalizers
	mov	dx,0ffh			; run all finalizers
	call	__FFiniRtns		; call finalizer routines
	pop	ax			; restore return code
	mov	ah,04cH 		; DOS call to exit with return code
	int	021h			; back to DOS
__exit_ endp

public	__GETDS
__GETDS proc	near
	ret				; return
__GETDS endp

__null_FPE_rtn proc far
	ret				; return
__null_FPE_rtn endp


_TEXT	ends

	end	_cstart_
