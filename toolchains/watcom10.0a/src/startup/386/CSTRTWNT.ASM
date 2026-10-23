;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;%  Copyright (C) 1988-1994 by WATCOM International Corp. All	%
;%  rights reserved. No part of this software may be reproduced %
;%  in any form or by any means - graphic, electronic or	%
;%  mechanical, including photocopying, recording, taping	%
;%  or information storage and retrieval systems - except	%
;%  with the written permission of WATCOM International Corp.	%
;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
;
; startup code for WATCOM C/C++32 character mode under Microsoft Windows NT
;
;	This must be assembled using one of the following commands:
;		wasm cstrtwnt -bt=NT -ms -3r_
;		wasm cstrtwnt -bt=NT -ms -3s_
;
        name    cstrtwnt
.386p
        assume  nothing

        extrn	__NTMain	: near
	extrn	___begtext	: near

_TEXT   segment use32 word public 'CODE'

        public  _cstart_

        assume  cs:_TEXT

_cstart_ proc near
        jmp     __NTMain
	dd	___begtext	; reference module with segment definitions
;
; copyright message
;
        db      "WATCOM C/C++32 Run-Time system. "
        db      "(c) Copyright by WATCOM International Corp. 1988-1994."
        db      " All rights reserved."
_cstart_ endp

_TEXT   ends

        end     _cstart_
