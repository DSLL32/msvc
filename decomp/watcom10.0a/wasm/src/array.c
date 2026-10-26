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
/* Initial Open Watcom6b9cb443 bld/womp/c/array.c, adapted to the actual
 * CF1c804..1cae6 non-rover64-element implementation. License below applies. */
#include <stdlib.h>
#include <string.h>
#include "wasm.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy,div)
#endif

array_hdr *ArrCreate(size_t elm_size, void *def)
{
    array_hdr *item;
    if (elm_size == 0) InternalError("g:\\bld\\womp\\c\\array.c",65);
    item = MemAlloc(sizeof(*item) + elm_size);
    item->block = NULL;item->elm_size=elm_size;item->num_elms=0;item->def=def;
    return item;
}
void ArrDestroy(array_hdr *arr)
{
    array_blk *current,*next;
    if (arr == NULL) InternalError("g:\\bld\\womp\\c\\array.c",83);
    for (current=arr->block;current != NULL;current=next) {
        next=current->next;MemFree(current);
    }
    MemFree(arr);
}
array_blk *newBlock(array_hdr *arr)
{
    array_blk *block;
    char *element,*stop;
    size_t size;
    void *def;
    if (arr == NULL) InternalError("g:\\bld\\womp\\c\\array.c",101);
    block=MemAlloc(sizeof(*block) + arr->elm_size*64);
    block->next=NULL;def=arr->def;
    if (def != NULL) {
        element=block->data;size=arr->elm_size;stop=element+size*64;
        while (element<stop) {memcpy(element,def,size);element+=size;}
    }
    return block;
}
void *ArrNewElm(array_hdr *arr,size_t number)
{
    array_blk *block;
    size_t local;
    if (arr == NULL) InternalError("g:\\bld\\womp\\c\\array.c",122);
    if (number >= arr->num_elms) arr->num_elms=number+1;
    local=number;block=arr->block;
    if (block == NULL) block=arr->block=newBlock(arr);
    if (block == NULL) InternalError("g:\\bld\\womp\\c\\array.c",144);
    while (local >= 64) {
        local-=64;if (block->next == NULL) block->next=newBlock(arr);
        block=block->next;
    }
    if (block == NULL) InternalError("g:\\bld\\womp\\c\\array.c",152);
    return block->data+local*arr->elm_size;
}
void *ArrAccess(array_hdr *arr,size_t number)
{
    array_blk *block;
    size_t local;
    if (arr == NULL) InternalError("g:\\bld\\womp\\c\\array.c",166);
    if (number >= arr->num_elms) InternalError("g:\\bld\\womp\\c\\array.c",167);
    local=number;block=arr->block;
    if (block == NULL) InternalError("g:\\bld\\womp\\c\\array.c",180);
    while (local >= 64) {
        local-=64;block=block->next;
        if (block == NULL) InternalError("g:\\bld\\womp\\c\\array.c",184);
    }
    if (block == NULL) InternalError("g:\\bld\\womp\\c\\array.c",186);
    return block->data+local*arr->elm_size;
}
int ArrWalk(array_hdr *arr,void *parameter,int (*callback)(void *,void *))
{
    array_blk *block;
    char *element,*stop;
    size_t size;
    div_t result;
    if (arr == NULL) InternalError("g:\\bld\\womp\\c\\array.c",204);
    block=arr->block;if (block == NULL) return 0;
    size=arr->elm_size;result=div(arr->num_elms,64);
    while (result.quot > 0) {
        if (block == NULL) InternalError("g:\\bld\\womp\\c\\array.c",213);
        element=block->data;block=block->next;stop=element+size*64;
        while (element<stop) {
            if (callback(element,parameter) == -1) return -1;
            element+=size;
        }
        --result.quot;
    }
    if (result.rem > 0) {
        if (block == NULL) InternalError("g:\\bld\\womp\\c\\array.c",226);
        element=block->data;stop=element+size*result.rem;
        while (element<stop) {
            if (callback(element,parameter) == -1) return -1;
            element+=size;
        }
    }
    return 0;
}
