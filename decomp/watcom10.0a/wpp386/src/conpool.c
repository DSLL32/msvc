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


/* Adapted from initial6b9cb443 bld/plusplus/c/conpool.c.
 * Actual1994 CF2ec6f..2ec90 has only the floating constant pool:
 *20-byte records,32 per block; no later int64 pool or64-record block. */
#include "compiler.h"
#include "carve.h"
#include "ptree.h"
#include "pending.h"
#include <recon.h>
typedef struct pool_con POOL_CON;
/* The old pool key is the canonical type plus the owned binary64 value.
 * Its used/free flag follows the value at byte16. */
struct pool_con {
    POOL_CON *next;
    TYPE type;
    double value;
    unsigned char free;
    unsigned char unused[3];
};
typedef char PoolConSize[(sizeof(POOL_CON)==20)?1:-1];
TYPE TypedefModifierRemove(TYPE type);
void *RingLookup(void *head,int (*compare)(void *,const void *),const void *key);
void *RingCarveAlloc(carve_t carve,void **head);
static POOL_CON *pool_float;
static carve_t carvePOOL_CON;
void conPoolsInit(INITFINI *defn)
{
    (void)defn;
    pool_float = 0;
    carvePOOL_CON = CarveCreate(20, 32);
}

int compareFloat(void *entry,const void *key)
{
    POOL_CON *pool=entry;
    PTREE expr=(PTREE)key;
    return pool->value==expr->u.floating_constant && pool->type==expr->type;
}
void initFloat(POOL_CON *pool,PTREE expr)
{
    pool->value=expr->u.floating_constant;
}
POOL_CON *addPoolCon(PTREE expr,POOL_CON **head,
                     int (*compare)(void *,const void *),
                     void (*initialize)(POOL_CON *,PTREE))
{
    TYPE type=TypedefModifierRemove(expr->type);
    POOL_CON *pool=RingLookup(*head,compare,expr);
    if(!pool) {
        pool=RingCarveAlloc(carvePOOL_CON,(void **)head);
        initialize(pool,expr);
        pool->free&=~1;
        pool->type=type;
    }
    return pool;
}
void *ConPoolFloatAdd(PTREE expr)
{
    return addPoolCon(expr,&pool_float,compareFloat,initFloat);
}
