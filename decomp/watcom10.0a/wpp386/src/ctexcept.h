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


#ifndef WPP_CTEXCEPT_H
#define WPP_CTEXCEPT_H
#include "typesig.h"
#include "carve.h"
#include <recon.h>
typedef struct throw_cnv THROW_CNV;
struct throw_cnv { THROW_CNV *next; TYPE_SIG *sig; unsigned offset; };
typedef struct throw_cnv_ctl {
    THROW_CNV *hdr,*cur;
    carve_t carver;
    CGIO_LOCATION err_locn;
    TYPE src_type;
    unsigned offset;
    unsigned char flags;
    unsigned char unused[3];
} THROW_CNV_CTL;
typedef char ThrowCnvSize[(sizeof(THROW_CNV)==12)?1:-1];
typedef char ThrowCnvCtlSize[(sizeof(THROW_CNV_CTL)==36)?1:-1];
unsigned ThrowCnvInit(THROW_CNV_CTL *ctl,TYPE type);
TYPE ThrowCnvType(THROW_CNV_CTL *ctl,unsigned *offset);
void ThrowCnvFini(THROW_CNV_CTL *ctl);
#endif
