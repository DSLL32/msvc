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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/sdk/rc/wres/c/wrfindrs.c, commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; CF20548..205a4.
 * The12-byte window has Res/Type/Lang pointer order; Watcom uses hidden ESI. */
#include "wasm.h"
#include <recon.h>

WResDirWindow WResFindResource(const WResID *type, const WResID *name,
                              WResDir dir, WResLangType *lang)
{
    WResDirWindow window;
    window.CurrType = __FindType(type, dir);
    if (window.CurrType != NULL) {
        window.CurrRes = __FindRes(name, window.CurrType);
        if (window.CurrRes != NULL) {
            if (lang == NULL) window.CurrLang = window.CurrRes->Head;
            else window.CurrLang = __FindLang(lang, window.CurrRes);
        } else {
            window.CurrLang = NULL;
            window.CurrType = NULL;
        }
    } else {
        window.CurrRes = NULL;
        window.CurrLang = NULL;
    }
    return window;
}

/* CF204c0..204e4. */
WResTypeNode *__FindType(const WResID *type, WResDir dir)
{
    WResTypeNode *curr;
    for (curr = dir->Head; curr != NULL; curr = curr->Next) {
        if (WResIDCmp(type, &curr->Info.TypeName)) break;
    }
    return curr;
}

/* CF204e4..20508. */
WResResNode *__FindRes(const WResID *name, WResTypeNode *type)
{
    WResResNode *curr;
    for (curr = type->Head; curr != NULL; curr = curr->Next) {
        if (WResIDCmp(name, &curr->Info.ResName)) break;
    }
    return curr;
}

/* CF20508..20545. */
WResLangNode *__FindLang(WResLangType *lang, WResResNode *res)
{
    WResLangNode *curr;
    WResLangType deflang;
    if (lang == NULL) {
        lang = &deflang;
        lang->lang = 0;
        lang->sublang = 0;
    }
    for (curr = res->Head; curr != NULL; curr = curr->Next) {
        if (curr->Info.lang.lang == lang->lang &&
            curr->Info.lang.sublang == lang->sublang) break;
    }
    return curr;
}

/* CF205b0..205c6;12-byte aggregate input is callee-cleaned on Watcom. */
WResLangInfo *WResGetLangInfo(WResDirWindow window)
{
    if (window.CurrLang == NULL) return NULL;
    return &window.CurrLang->Info;
}
