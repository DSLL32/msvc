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


#ifndef WCC_GENERATE_CODE_H
#define WCC_GENERATE_CODE_H
#include <recon.h>
/* The actual old dispatcher accepts G_NO through G_FXCH, values0..100. */
enum gentype {
    G_NO,
    G_RC,
    G_AC,
    G_MC,
    G_RR2,
    G_RM2,
    G_MR2,
    G_WORDR1,
    G_R1,
    G_M1,
    G_R2,
    G_M2,
    G_1SHIFT,
    G_R1SHIFT,
    G_NSHIFT,
    G_RNSHIFT,
    G_CLSHIFT,
    G_RCLSHIFT,
    G_RR1,
    G_RM1,
    G_MR1,
    G_LEA,
    G_LDSES,
    G_MS1,
    G_SM1,
    G_RS,
    G_SR,
    G_MOVMC,
    G_MADDR,
    G_TEST,
    G_SEGR1,
    G_MOVAM,
    G_MOVMA,
    G_MOVRC,
    G_RADDR,
    G_C1,
    G_RRFBIN,
    G_RNFBIN,
    G_RRFBINP,
    G_RNFBINP,
    G_RRFBIND,
    G_RNFBIND,
    G_MRFBIN,
    G_MNFBIN,
    G_MFLD,
    G_RFLD,
    G_SLACK_1,
    G_MFST,
    G_RFST,
    G_SLACK_2,
    G_FCHS,
    G_FRNDINT,
    G_FLDZ,
    G_FLD1,
    G_FINIT,
    G_FCOMPP,
    G_MCOMP,
    G_RCOMP,
    G_MCOM,
    G_RCOM,
    G_MFSTNP,
    G_RFSTNP,
    G_FTST,
    G_REPOP,
    G_LOADADDR,
    G_4SHIFT,
    G_4RSHIFT,
    G_CXSHIFT,
    G_RCXSHIFT,
    G_4NEG,
    G_4RNEG,
    G_FNEG,
    G_RFNEG,
    G_ENTER,
    G_CALL,
    G_RCALL,
    G_ICALL,
    G_CCALL,
    G_RJMP,
    G_MJMP,
    G_CJMP,
    G_SIGNEX,
    G_186MUL,
    G_186RMUL,
    G_FWAIT,
    G_FCHOP,
    G_SEG_SEG,
    G_DEBUG,
    G_MOVSX,
    G_MOVZX,
    G_0FRR2,
    G_0FRM2,
    G_PUSHFS,
    G_POW2DIV,
    G_POW2DIV_286,
    G_DIV2,
    G_FMATH,
    G_IFUNC,
    G_FSINCOS,
    G_FP_SLACK_7,
    G_FXCH
};
#endif
