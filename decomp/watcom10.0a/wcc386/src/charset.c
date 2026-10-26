/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "scanner.h"
#include <recon.h>

/* ASCII identifier/delimiter classes, encountered by ScanName. The released
 * table matches all257 bytes in this target, including its extended letters. */
unsigned char CharSet[]  = {                              /* ASCII */

/*  00 NUL 01 SOH 02 STX 03 ETX 04 EOT 05 ENQ 06 ACK 07 BEL */
     C_EX,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC, /* NUL to BEL */

/*  08 BS  09 HT  0A LF  0B VT  0C FF  0D CR  0E SO  0F SI  */
     C_BC,  C_WS,  C_EX,  C_WS,  C_WS,  C_WS,  C_BC,  C_BC, /* BS  to SI */

/*  10 DLE 11 DC1 12 DC2 13 DC3 14 DC4 15 NAK 16 SYN 17 ETB */
     C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC, /* DLE to ETB */

/*  18 CAN 19 EM  1A SUB 1B ESC 1C FS  1D GS  1E RS  1F US  */
     C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC, /* CAN to US */

        C_WS,           /* 20 SP */
        C_D2,           /* 21 !  */
        C_D1,           /* 22 "  */
        C_D2,           /* 23 #  */
        C_BC,           /* 24 $  */
        C_D2,           /* 25 %  */
        C_D2,           /* 26 &  */
        C_D1,           /* 27 '  */

        C_D1,           /* 28 (  */
        C_D1,           /* 29 )  */
        C_D2,           /* 2A *  */
        C_D2,           /* 2B +  */
        C_D1,           /* 2C ,  */
        C_D2,           /* 2D -  */
        C_D1,           /* 2E .  */
        C_D2,           /* 2F /  */

        C_DI,           /* 30 0  */
        C_DI,           /* 31 1  */
        C_DI,           /* 32 2  */
        C_DI,           /* 33 3  */
        C_DI,           /* 34 4  */
        C_DI,           /* 35 5  */
        C_DI,           /* 36 6  */
        C_DI,           /* 37 7  */

        C_DI,           /* 38 8  */
        C_DI,           /* 39 9  */
        C_D2,           /* 3A :  */
        C_D1,           /* 3B ;  */
        C_D2,           /* 3C <  */
        C_D2,           /* 3D =  */
        C_D2,           /* 3E >  */
        C_D1 | C_EX,    /* 3F ?  */

        C_BC,           /* 40 @  */
        C_HX | C_AL,    /* 41 A  */
        C_HX | C_AL,    /* 42 B  */
        C_HX | C_AL,    /* 43 C  */
        C_HX | C_AL,    /* 44 D  */
        C_HX | C_AL,    /* 45 E  */
        C_HX | C_AL,    /* 46 F  */
        C_AL,           /* 47 G  */

/*  48 H   49 I   4A J   4B K   4C L   4D M   4E N   4F O   */
     C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL, /* H   to O */

/*  50 P   51 Q   52 R   53 S   54 T   55 U   56 V   57 W   */
     C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL, /* P   to W */

/*  58 X   59 Y   5A Z   5B [   5C \   5D ]   5E ^   5F _   */
     C_AL,  C_AL,  C_AL,  C_D1,  C_EX,  C_D1,  C_D2,  C_AL, /* X   to _ */

        C_BC,           /* 60 `  */
        C_HX | C_AL,    /* 61 a  */
        C_HX | C_AL,    /* 62 b  */
        C_HX | C_AL,    /* 63 c  */
        C_HX | C_AL,    /* 64 d  */
        C_HX | C_AL,    /* 65 e  */
        C_HX | C_AL,    /* 66 f  */
        C_AL,           /* 67 g  */

/*  68 h   69 i   6A j   6B k   6C l   6D m   6E n   6F o   */
     C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL, /* h   to o */

/*  70 p   71 q   72 r   73 s   74 t   75 u   76 v   77 w   */
     C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL, /* p   to w */

/*  78 x   79 y   7A z   7B {   7C |   7D }   7E ~   7F DEL */
     C_AL,  C_AL,  C_AL, C_D1,  C_D2,  C_D1,  C_D1,  C_BC,  /* x   to DEL */

/*  80     81     82     83     84     85     86     87  */
    C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  /* $80 to $87 */

/*  88     89     8A     8B     8C     8D     8E     8F  */
    C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  /* $88 to $8F */

/*  90     91     92     93     94     95     96     97  */
    C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  /* $90 to $97 */

/*  98     99     9A     9B     9C     9D     9E     9F  */
    C_AL,  C_AL,  C_AL,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $98 to $9F */

/*  A0     A1     A2     A3     A4     A5     A6     A7  */
    C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  C_AL,  /* $A0 to $A7 */

/*  A8     A9     AA     AB     AC     AD     AE     AF  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $A8 to $AF */

/*  B0     B1     B2     B3     B4     B5     B6     B7  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $B0 to $B7 */

/*  B8     B9     BA     BB     BC     BD     BE     BF  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $B8 to $BF */

/*  C0     C1     C2     C3     C4     C5     C6     C7  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $C0 to $C7 */

/*  C8     C9     CA     CB     CC     CD     CE     CF  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $C8 to $CF */

/*  D0     D1     D2     D3     D4     D5     D6     D7  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $D0 to $D7 */

/*  D8     D9     DA     DB     DC     DD     DE     DF  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $D8 to $DF */

/*  E0     E1     E2     E3     E4     E5     E6     E7  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $E0 to $E7 */

/*  E8     E9     EA     EB     EC     ED     EE     EF  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $E8 to $EF */

/*  F0     F1     F2     F3     F4     F5     F6     F7  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $F0 to $F7 */

/*  F8     F9     FA     FB     FC     FD     FE     FF  */
    C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  C_BC,  /* $F8 to $FF */

    0                   // EOF_CHAR
  };
