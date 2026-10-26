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


/* Adapted from initial6b9cb443 bld/plusplus/c/hashtab.c.
 * Actual1994 CF3c821..3c85d has a compact13-byte hash control and two
 * fixed table classes,32 and964 bytes. The later power-of-two carveTable
 * loop and reporting calls are absent. Fields await reached consumers. */
#include <string.h>
#include "compiler.h"
#include "carve.h"
#include "scope.h"
#include "pending.h"
#include <recon.h>
static carve_t carveHASHTAB, carveSMALL_TABLE, carveLARGE_TABLE;
void hashInit(INITFINI *defn)
{
    (void)defn;
    carveHASHTAB = CarveCreate(13, 32);
    carveSMALL_TABLE = CarveCreate(32, 32);
    carveLARGE_TABLE = CarveCreate(964, 4);
}

static SYMBOL_NAME listSentinel;
void HashPostInit(SCOPE scope)
{
    if (scope==0) listSentinel=AllocSymbolName(0,0);
    else ScopeSetContaining(listSentinel,scope);
}
static unsigned smallHashLimit=192; /* original initialized b31f0 */
void HashDestroy(HASHTAB hash)
{
    switch(hash->table_class) {
    case 1: CarveFree(carveSMALL_TABLE,hash->bits); break;
    case 2: CarveFree(carveLARGE_TABLE,hash->u.table); break;
    }
    CarveFree(carveHASHTAB,hash);
}
void HashInsert(HASHTAB hash,SYMBOL_NAME sym,char *name)
{
    unsigned key;
    ++hash->names;
    key=((unsigned char *)name)[-1];
    if (hash->table_class==1) {
        hash->bits[key>>5]|=1U<<(key&31);
        sym->next=hash->u.list;
        hash->u.list=sym;
        if (hash->names>smallHashLimit) HashMakeMax(hash);
    } else {
        sym->next=hash->u.table[key];
        hash->u.table[key]=sym;
    }
}
/* Actual1994 CF3c920..3c987 uses the interned name's stored hash byte
 * and pointer identity. The sentinel makes the list search bounded; found
 * nodes are genuinely moved to the bucket/list head. No string comparison. */
SYMBOL_NAME HashLookup(HASHTAB hash,char *name)
{
    unsigned key;
    SYMBOL_NAME *head,*previous,sym;
    key=((unsigned char *)name)[-1];
    if (hash->table_class==1) {
        if (!(hash->bits[key>>5]&(1U<<(key&31)))) return 0;
        head=&hash->u.list;
    } else head=&hash->u.table[key];
    listSentinel->name=name;
    previous=head;
    for (sym=*head; sym->name!=name; sym=sym->next) previous=&sym->next;
    if (sym==listSentinel) return 0;
    *previous=sym->next;
    sym->next=*head;
    *head=sym;
    return sym;
}
/* Actual1994 CF3c987..3c9fc grows the existing compact control in place;
 * this predates the later destroy-and-HashCreate implementation. */
void HashMakeMax(HASHTAB hash)
{
    struct hash_tab large;
    SYMBOL_NAME sym, next;
    SYMBOL_NAME *bucket;
    large.u.table=CarveAlloc(carveLARGE_TABLE);
    for (bucket=large.u.table; bucket<large.u.table+241; ++bucket)
        *bucket=listSentinel;
    large.table_class=2;
    large.names=0;
    for (sym=hash->u.list; sym!=listSentinel; sym=next) {
        next=sym->next;
        HashInsert(&large,sym,sym->name);
    }
    CarveFree(carveSMALL_TABLE,hash->bits);
    hash->table_class=2;
    hash->names=large.names;
    hash->u.table=large.u.table;
    /* The original leaves the now-unused small presence-map pointer intact. */
}

/* Actual1994 CF3c89a..3c8dd. Small tables use a32-byte presence map
 * and linked symbol-name list, before later linear hashing. */
HASHTAB HashCreate(void)
{
    HASHTAB hash;
    hash=CarveAlloc(carveHASHTAB);
    hash->u.list=listSentinel;
    hash->bits=CarveAlloc(carveSMALL_TABLE);
    hash->names=0;
    hash->table_class=1;
    memset(hash->bits,0,32);
    return hash;
}

int HashEmpty(HASHTAB hash)
{
    return hash->names==0;
}

void HashWalk(HASHTAB hash,void (*walker)(SYMBOL_NAME))
{
    SYMBOL_NAME sym,next;
    unsigned i;
    switch(hash->table_class) {
    case 1:
        for(sym=hash->u.list;sym!=listSentinel;sym=next) {
            next=sym->next;
            walker(sym);
        }
        break;
    case 2:
        for(i=0;i<241;++i) {
            for(sym=hash->u.table[i];sym!=listSentinel;sym=next) {
                next=sym->next;
                walker(sym);
            }
        }
        break;
    }
}

void HashWalkData(HASHTAB hash,void (*walker)(SYMBOL_NAME,void *),void *data)
{
    SYMBOL_NAME sym,next;
    unsigned i;
    switch(hash->table_class) {
    case 1:
        for(sym=hash->u.list;sym!=listSentinel;sym=next) {
            next=sym->next;
            walker(sym,data);
        }
        break;
    case 2:
        for(i=0;i<241;++i) {
            for(sym=hash->u.table[i];sym!=listSentinel;sym=next) {
                next=sym->next;
                walker(sym,data);
            }
        }
        break;
    }
}
