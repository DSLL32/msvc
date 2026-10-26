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

#ifndef WPP_CPRAGMA_H
#define WPP_CPRAGMA_H
#include <stddef.h>
#include <recon.h>
typedef struct aux_info AUX_INFO;
/* The 1994 register sets are32bits (actual comparisons at a9ca..a9e9).
 * Packed call_class follows flags at37, and the terminal zero-width int
 * rounds each original object to44bytes. The later source has wider sets. */
#pragma pack(1)
struct aux_info {
    AUX_INFO *next;
    unsigned use, index;
    void *code;
    char *objname;
    unsigned *parms;
    unsigned returns, streturn, save;
    unsigned char flags;
    unsigned _class;
    int : 0;
};
typedef char AuxInfoSize[(sizeof(AUX_INFO)==44)?1:-1];
typedef char AuxInfoClass[(offsetof(AUX_INFO,_class)==37)?1:-1];
typedef struct aux_entry {
    struct aux_entry *next;
    AUX_INFO *info;
    char name[1];
} AUX_ENTRY;
#pragma pack()
extern AUX_INFO *CurrInfo;
int setAuxInfo(unsigned index, int create_new);
unsigned lookupMagicKeyword(char *name);
AUX_ENTRY *AuxLookup(char *name);
void CreateAux(char *name);
void *PragmaLookup(char *name, unsigned index);
#endif
