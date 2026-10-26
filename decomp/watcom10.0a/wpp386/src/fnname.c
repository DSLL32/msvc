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
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "symbol.h"
#include "pending.h"
#include "utility.h"
#include <recon.h>

typedef struct operator_name {
    char *code;
    char *name;
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
        op->name = NameCreateNoLen(op->name);
    for (name = special_names; name < special_names + 12; ++name)
        *name = NameCreateNoLen(*name);
    VbufInit(&mangled_name);
}

/* Actual1994 CF391b1..391b9: retrieve the interned operator name. */
char *CppOperatorName(unsigned oper) { return operator_names[oper].name; }

char *CppConversionName(void) { return operator_names[57].name; }

char *CppSpecialName(unsigned index) { return special_names[index]; }

char *CppConstructorName(void) { return operator_names[62].name; }
char *CppDestructorName(void) { return operator_names[99].name; }

void VStrNull(VBUF *buffer);
void VStrConcStr(VBUF *buffer,const char *string);
static int next_replicate;
static struct { char *ptr; unsigned len; } replicate[10];

void replicateInit(void) { next_replicate=0; }
void appendStr(char *text)
{
    if (text) VStrConcStr(&mangled_name,text);
}
void setPrefix(char *last)
{
    replicateInit();
    VStrNull(&mangled_name);
    appendStr("$");
    appendStr("W");
    appendStr(last);
}
char *setMangling(char *prefix)
{
    char *save;
    setPrefix(prefix);
    save=strsave(mangled_name.buf);
    VStrNull(&mangled_name);
    return save;
}
void VStrConcChr(VBUF *buffer,char chr);
void appendChar(char chr) { VStrConcChr(&mangled_name,chr); }
int replicateSearch(char *name)
{
    int i;
    unsigned length=strlen(name);
    for(i=0;i<next_replicate;++i) {
        if (length==replicate[i].len && strcmp(name,replicate[i].ptr)==0)
            return i;
    }
    if(i<10) {
        replicate[i].len=length;
        ++next_replicate;
        replicate[i].ptr=name;
    }
    return -1;
}
int CppLookupName(char *name,unsigned char *oper)
{
    unsigned i;
    for(i=0;i<121;++i) {
        if(name==operator_names[i].name) { *oper=(unsigned char)i; return 1; }
    }
    return 0;
}
void appendSymName(SYMBOL sym)
{
    char *name=sym->name->name;
    unsigned char oper;
    int index;
    if (CppLookupName(name,&oper)) {
        appendStr("$");
        appendStr(operator_names[oper].code);
    } else {
        index=replicateSearch(name);
        if(index>=0) appendChar((char)(index+'0'));
        else { appendStr(name); appendStr("$"); }
    }
}
TYPE TypeModExtract(TYPE type,unsigned short *flags,void **baser,unsigned char mask);
TYPE typeLookAhead(TYPE type,unsigned short *flags,void **baser)
{
    return TypeModExtract(type,flags,baser,9);
}
void appendBasedMod(unsigned short flags,void *base)
{ (void)flags; (void)base; STUB_TRAP(); }
void appendModifier(unsigned short flags,void *base)
{
    if(flags&0x700) appendBasedMod(flags,base);
    if(flags&8) appendChar('f');
    if(flags&16) appendChar('g');
    if(flags&32) appendChar('h');
    if(flags&4) appendChar('n');
    if(flags&2) appendChar('y');
    if(flags&1) appendChar('x');
}
void appendInt(unsigned value);
TYPE ScopeClass(SCOPE scope);
SYMBOL ScopeFunction(SCOPE scope);
SYMBOL ScopeOrderedStart(SCOPE scope);
SYMBOL ScopeOrderedNext(SYMBOL stop,SYMBOL curr);
unsigned ScopeIndex(SCOPE scope);
void appendTemplateParm(SYMBOL sym)
{ (void)sym; STUB_TRAP(); }
void appendTypedSymName(SYMBOL sym);
void appendClassName(TYPE type);
/* The old driver emits only the first block index, unlike the later
 * consecutive-block skip. The block number occupies the scope owner word. */
void appendScopeMangling(SCOPE scope)
{
    TYPE type;
    SYMBOL sym,stop;
    char buffer[12];
    int block_mangled=0;
    for(;scope;scope=scope->enclosing) {
        type=ScopeClass(scope);
        if(type) {
            appendChar(':');
            appendClassName(type);
            continue;
        }
        sym=ScopeFunction(scope);
        if(sym) {
            appendChar(':');
            appendStr("?");
            appendTypedSymName(sym);
            continue;
        }
        if(ScopeType(scope,3)) {
            if(!block_mangled) {
                buffer[0]='.';
                ultoa(ScopeIndex(scope),buffer+1,31);
                appendChar(':');
                appendStr(buffer);
                appendStr("$");
                block_mangled=1;
            }
            continue;
        }
        if(ScopeType(scope,6)) {
            stop=ScopeOrderedStart(scope);
            appendChar(':');
            appendChar(':');
            sym=NULL;
            while((sym=ScopeOrderedNext(stop,sym))!=NULL)
                appendTemplateParm(sym);
        }
    }
}
void appendReplName(char *name)
{
    int index=replicateSearch(name);
    if(index>=0) appendChar((char)(index+'0'));
    else { appendStr(name); appendStr("$"); }
}
void ClassMakeUniqueName(TYPE type,char *name);
/* The 1994 body performs replication here, before the later className
 * extraction. Keep its direct calls and the real class metadata name. */
void appendClassName(TYPE type)
{
    struct class_info *info=type->u.c.info;
    int index;
    if(!info->name) ClassMakeUniqueName(type,NULL);
    index=replicateSearch(info->name);
    if(index>=0) appendChar((char)(index+'0'));
    else { appendStr(info->name); appendStr("$"); }
}
TYPE MemberPtrClass(TYPE type,TYPE *unmodified)
{ (void)type; (void)unmodified; STUB_TRAP(); return NULL; }
char *SimpleTypeName(TYPE type);
extern TYPE TypeError;
void appendTypeFlags(unsigned short flags) { appendModifier(flags,NULL); }

/* Actual 1994 two-argument driver: no later BOOL/int64/control-mask forms. */
void appendTypeContinue(TYPE type,char *name)
{
    unsigned first_dim=1, count;
    TYPE *arguments;
    unsigned short flags;
    void *base;
    char *enum_name;
    while(type) {
        switch(type->id) {
        case TYP_CHAR: appendChar('a'); type=TypeError; break;
        case TYP_SCHAR: appendChar('c'); break;
        case TYP_UCHAR: appendChar('u'); appendChar('c'); break;
        case TYP_WCHAR: appendChar('w'); break;
        case TYP_SSHORT: appendChar('s'); break;
        case TYP_USHORT: appendChar('u'); appendChar('s'); break;
        case TYP_SINT: appendChar('i'); break;
        case TYP_UINT: appendChar('u'); appendChar('i'); break;
        case TYP_SLONG: appendChar('l'); break;
        case TYP_ULONG: appendChar('u'); appendChar('l'); break;
        case TYP_FLOAT: appendChar('b'); break;
        case TYP_DOUBLE: appendChar('d'); break;
        case TYP_LONG_DOUBLE: appendChar('t'); break;
        case TYP_POINTER:
            first_dim=0;
            appendChar((type->flag&1) ? 'r':'p');
            type=typeLookAhead(type->of,&flags,&base);
            appendModifier(flags,base);
            continue;
        case TYP_TYPEDEF: break;
        case TYP_ENUM:
            appendStr("$");
            enum_name=SimpleTypeName(type);
            if(enum_name) appendReplName(enum_name);
            else { appendStr("$"); appendStr("TE"); appendStr("$"); }
            appendScopeMangling(type->u.t.scope);
            appendStr("$");
            type=TypeError;
            break;
        case TYP_CLASS:
            appendStr("$");
            appendClassName(type);
            appendScopeMangling(type->u.c.scope->enclosing);
            appendStr("$");
            break;
        case TYP_FUNCTION:
            flags=type->u.f.args->qualifier;
            if(flags) { appendChar('.'); appendTypeFlags(flags); }
            appendChar('(');
            count=type->u.f.args->num_args;
            arguments=type->u.f.args->type_list;
            for(;count;--count) appendTypeContinue(*arguments++,NULL);
            appendChar(')');
            if(name==CppConstructorName() || name==CppDestructorName()) {
                appendChar('_'); name=NULL; type=TypeError;
            }
            break;
        case TYP_ARRAY:
            appendChar('[');
            if(first_dim) first_dim=0;
            else appendInt(type->u.a.array_size);
            appendChar(']');
            break;
        case TYP_DOT_DOT_DOT: appendChar('e'); break;
        case TYP_VOID: appendChar('v'); break;
        case TYP_MODIFIER: appendModifier(type->flag,type->u.m.base); break;
        case TYP_MEMBER_POINTER:
            appendChar('m');
            appendTypeContinue(MemberPtrClass(type,NULL),NULL);
            type=typeLookAhead(type->of,&flags,&base);
            first_dim=0;
            appendModifier(flags,base);
            continue;
        }
        type=type->of;
    }
}
void appendType(TYPE type,char *name)
{
    unsigned short flags;
    void *base;
    type=typeLookAhead(type,&flags,&base);
    appendModifier(flags,base);
    appendTypeContinue(type,name);
}
void appendTypedSymName(SYMBOL sym)
{
    appendSymName(sym);
    appendType(sym->sym_type,sym->name->name);
}
void appendInt(unsigned value)
{
    char buffer[16];
    ultoa(value,buffer,10);
    appendStr(buffer);
}
void VStrPrepStr(VBUF *buffer,const char *text);
void VStrPrepChr(VBUF *buffer,char chr);
void CFatal(const char *message);
void prependStr(char *text)
{
    if(text) VStrPrepStr(&mangled_name,text);
}
void prependChar(char chr) { VStrPrepChr(&mangled_name,chr); }
void prependLen(void)
{
    char buffer[16];
    ultoa(strlen(mangled_name.buf),buffer,36);
    switch(strlen(buffer)) {
    case 1: prependStr(buffer); prependChar('0'); break;
    case 2: prependStr(buffer); break;
    default: CFatal("internal name length > 36*36"); break;
    }
}
char *retMangling(char *save)
{
    prependLen();
    prependStr(save);
    CMemFree(save);
    return NameCreateNoLen(mangled_name.buf);
}

/* The 1994 default-argument helper disappeared from the later source. Its
 * actual name uses the same typed-symbol and decimal argument-index fields. */
char *CppDefaultArgName(SYMBOL sym,unsigned index)
{
    char *save=setMangling("df");
    appendTypedSymName(sym);
    appendInt(index);
    return retMangling(save);
}

void appendDelta(long offset)
{
    char buffer[16];
    buffer[0]='o';
    ultoa(offset,buffer+1,31);
    appendStr(buffer);
}
char *tableName(SCOPE scope,long offset,char *kind)
{
    char *save=setMangling(kind);
    appendDelta(offset);
    appendScopeMangling(scope);
    return retMangling(save);
}
char *CppVFTableName(SCOPE scope,long offset)
{
    return tableName(scope,offset,"vf");
}
