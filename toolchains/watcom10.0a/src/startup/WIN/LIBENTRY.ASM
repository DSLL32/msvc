;****************************************************************************
;***                                                                      ***
;*** LIBENTRY.ASM - entry point for Windows DLL                           ***
;***                                                                      ***
;***	Date		By		Reason				  ***
;***	====		==		======				  ***
;***	17-mar-91	C.G.Eisler	initial implementation            ***
;***	02-apr-91	C.G.Eisler	tweaking of selector pop/pushes   ***
;***	23-may-91	C.G.Eisler	new heap initialization           ***
;***    04-jun-91       C.G.Eisler      freeing of old DLLEntries         ***
;***	16-jul-91	D.J.Pronovost	modified for 16 bit DLLs	  ***
;***	04-mar-93	F.W.Crigger	__DLLstart_ added and code to	  ***
;***					initialize ___FPE_handler	  ***
;***	19-oct-93	Greg Bentz	initialize __win_alloc_flags and  ***
;***					__win_realloc_flags GMEM_SHARE	  ***
;***                                                                      ***
;****************************************************************************
;
; startup code for WATCOM C DLLs under Microsoft Windows
;
;	This must be assembled using one of the following commands:
; 		wasm libentry.asm -bt=WINDOWS -ms -0r_
; 		wasm libentry.asm -bt=WINDOWS -mm -0r_
; 		wasm libentry.asm -bt=WINDOWS -mc -0r_
; 		wasm libentry.asm -bt=WINDOWS -ml -0r_
;

DGROUP group _NULL,_DATA,CONST,STRINGS,DATA,XIB,XI,XIE,YIB,YI,YIE,_BSS
 
public  pLocalHeap
public  pAtomTable
public  pStackTop
public  pStackMin
public  pStackBot

pLocalHeap      equ     0006H
pAtomTable      equ     0008H
pStackTop       equ     000AH
pStackMin       equ     000CH
pStackBot       equ     000EH

; cloned from windows.inc
GMEM_SHARE      equ   	2000h

	.dosseg

FAR_DATA segment byte public 'FAR_DATA' 
FAR_DATA ends

_BSS    segment word public 'BSS' 
_BSS    ends

DATA    segment word public 'DATA' 
DATA    ends

CONST	segment word public 'DATA'
CONST	ends

STRINGS segment word public 'DATA'
STRINGS ends

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

_NULL   segment para public 'BEGDATA' 
__nullarea label word
	   dw   0,0
	   dw   5
	   dw   0               ; pLocalHeap
	   dw   0               ; pAtomTable
__STACKLOW dw   0               ; pStackTop: lowest address in stack
__STACKTOP dw   0               ; pStackMin:
	   dw   0               ; pStackBot: highest address in stack
	public  __nullarea
_NULL   ends

_DATA segment word public 'DATA' 
;*
;*** externals we need
;*
assume es:nothing
assume ss:nothing
assume ds:dgroup
assume cs:_TEXT

	extrn	__AHSHIFT		    : word
	extrn	"C",___win_alloc_flags	    : dword
	extrn	"C",___win_realloc_flags    : dword

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
	   db 0                 ; slack byte
	   
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

_DATA ends

;*
;*** the windows extender code lies here
;*
_TEXT segment word public 'CODE' 

	extrn	LIBMAIN	    : far	; startup code
	extrn	LOCALINIT   : far	; Windows heap init routine
	extrn	__FInitRtns : far	; initializer
	extrn	__FFiniRtns : far	; finalizer

public          _large_code_
_large_code_    equ 0

;****************************************************************************
;***                                                                      ***
;*** LibEntry - 16-bit library entry point                                ***
;***                                                                      ***
;****************************************************************************
LibEntry proc far
	public  LibEntry
__DLLstart_:
	public  __DLLstart_

        mov     ax,ds		 ; prologue
        nop
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ds,ax

	push    di               ; handle of the module instance
	push    ds               ; library data segment
	push    cx               ; heap size
	push    es               ; command line segment
	push    si               ; command line offset
	jcxz    callc            ; skip heap init
	xor     ax,ax
	push    ds
	push    ax
	push    cx
	call    LOCALINIT
	or      ax,ax            ; did it do it ok ?
	jz      _error           ; quit if it failed

callc:
	or	word ptr ___win_alloc_flags, GMEM_SHARE
	or	word ptr ___win_realloc_flags, GMEM_SHARE
	mov	ax,offset __AHSHIFT ; get huge shift value
	mov	__HShift,al	; ...
	cmp	al,12		; real mode?
	je	notprot		; yes, so leave osmode alone
	mov	al,1
	mov	__osmode,al	; protected mode!
notprot:
	mov	ax,offset __null_FPE_rtn; initialize floating-point exception
	mov	___FPE_handler,ax	; ... handler address
	mov	___FPE_handler+2,cs	; ...
	mov	ax,0ffh		; run all initializers
	call	__FInitRtns	; call initializer routines
	call    LIBMAIN		; invoke the 'C' routine (result in AX)
	jmp	short exit	; LibMain is responsible for stack clean up

_error:

	pop     si               ; clean up stack on a LocalInit error
	pop     es               
	pop     cx               
	pop     ds
	pop     di
	jmp     short exit
	
__exit_:
	public	"C",__exit_

	push	ax		; save return code
	mov	ax,00h		; run all finalizers
	mov	dx,0ffh		; run all finalizers
	call	__FFiniRtns	; call finalizer routines
	pop	ax		; restore return code
	mov     ah,04cH         ; DOS call to exit with return code
	int     021h            ; back to DOS

exit:
        lea     sp,-2H[bp]
        pop     ds
        pop     bp
        dec     bp
        ret
LibEntry    endp

__null_FPE_rtn proc far
	ret				; return
__null_FPE_rtn endp

public	__GETDS
__GETDS proc	near
	ret				; return
__GETDS endp

_TEXT	ends
        end     LibEntry
