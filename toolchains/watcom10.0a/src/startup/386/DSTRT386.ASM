;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;%  Copyright (C) 1988-1994 by WATCOM International Corp. All	%
;%  rights reserved. No part of this software may be reproduced %
;%  in any form or by any means - graphic, electronic or	%
;%  mechanical, including photocopying, recording, taping	%
;%  or information storage and retrieval systems - except	%
;%  with the written permission of WATCOM International Corp.	%
;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;
; startup code for WATCOM C/C++32 DLL's
;
;	This must be assembled using one of the following commands:
;		wasm dstrt386 -ms -3r_
;		wasm dstrt386 -ms -3s_
;
        name    dstrt386
.386p
        assume  nothing

        extrn	__LibMain	: near
	extrn	___begtext	: near

_TEXT   segment use32 word public 'CODE'

        public  __DLLstart_

        assume  cs:_TEXT

__DLLstart_ proc near
        jmp     __LibMain
	dd	___begtext	; reference module with segment definitions
;
; copyright message
;
        db      "WATCOM C/C++32 Run-Time system. "
        db      "(c) Copyright by WATCOM International Corp. 1988-1994."
        db      " All rights reserved."
__DLLstart_ endp

_TEXT   ends

        end     __DLLstart_
