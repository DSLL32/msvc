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

/* cppNamesInit, correlated to bld/plusplus/c/fnname.c at initial
 * commit6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. The121 encountered
 * operator records follow the actual1994 ppops order; the1995 rewrite moved
 * operators and changed the placeholder algorithm. Literal spellings belong
 * here beside this initializer, not a copied data/string section. */
#include "compiler.h"
#include <recon.h>

typedef struct operator_name {
    char *name;
    char *spelling;
} OPERATOR_NAME;
#define OP(code, spelling) { code, "$op" spelling "$" }
static OPERATOR_NAME operator_names[] = {
    OP("oj", "+"),
    OP("oi", "-"),
    OP("of", "*"),
    OP("om", "/"),
    OP("on", "%"),
    OP("ok", "&"),
    OP("os", "|"),
    OP("or", "^"),
    OP("oa", ">>"),
    OP("ob", "<<"),
    OP("oi", "-"),
    OP("oq", "~"),
    OP("ok", "&"),
    OP("aa", "="),
    OP("ra", "=="),
    OP("rb", "!="),
    OP("re", ">"),
    OP("rd", "<="),
    OP("rc", "<"),
    OP("rf", ">="),
    OP("ot", "&&"),
    OP("ou", "||"),
    OP("oc", "!"),
    OP("oe", "->"),
    OP("od", "[]"),
    OP("ac", "+="),
    OP("ad", "-="),
    OP("ab", "*="),
    OP("ae", "/="),
    OP("af", "%="),
    OP("ai", "&="),
    OP("aj", "|="),
    OP("ak", "^="),
    OP("ag", ">>="),
    OP("ah", "<<="),
    OP("oj", "+"),
    OP("og", "++"),
    OP("oh", "--"),
    OP("og", "++"),
    OP("oh", "--"),
    OP("of", "*"),
    OP("", ""),
    OP("oo", ","),
    OP("", ""),
    OP("", ""),
    OP("aa00", ".*"),
    OP("ol", "->*"),
    OP("op", "()"),
    OP("ab00", "."),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("ac00", "storage"),
    OP("ad00", "operator"),
    OP("ae00", "::"),
    OP("", ""),
    OP("cv", "CONV"),
    OP("dl", "delete"),
    OP("dl", "delete"),
    OP("da", "delete[]"),
    OP("dl", "delete[]"),
    OP("ct", "CTOR"),
    OP("nw", "new"),
    OP("nw", "new"),
    OP("na", "new[]"),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", "log"),
    OP("", "cos"),
    OP("", "sin"),
    OP("", "tan"),
    OP("", "sqrt"),
    OP("", "fabs"),
    OP("", "acos"),
    OP("", "asin"),
    OP("", "atan"),
    OP("", "cosh"),
    OP("", "exp"),
    OP("", "sinh"),
    OP("", "tanh"),
    OP("", "log10"),
    OP("", "pow"),
    OP("", "atan2"),
    OP("", "fmod"),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("dt", "DTOR"),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
    OP("", ""),
};
#undef OP
static char *special_names[] = {
    "this", ".return", ".copy_arg", ".mod_init", ".fn_init", ".cdtor",
    ".tdctor", ".tcctor", ".tdtor", ".tdopdel", ".tdopdelarr", ".sj_val"
};
VBUF mangled_name;

void cppNamesInit(INITFINI *entry)
{
    OPERATOR_NAME *op;
    char **name;

    (void)entry;
    for (op = operator_names; op < operator_names + 121; ++op)
        op->name = NameCreateNoLen(op->spelling);
    for (name = special_names; name < special_names + 12; ++name)
        *name = NameCreateNoLen(*name);
    VbufInit(&mangled_name);
}
