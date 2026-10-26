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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


#include <stddef.h>
#include <string.h>
#include "macros.h"
#include "options.h"
#include "scanner.h"
#include <recon.h>

unsigned Toggles,DataThreshold,OptSize;
int ErrLimit=20,WngLevel,PreProcChar;
struct undef_names *UndefNames;
char *ObjectFileName,*ModuleName;
hw_reg_set DefaultParms[2]={HW_EAX|HW_EBX|HW_ECX|HW_EDX,0};

void InitModInfo(void)
{
    Toggles=0x0001|0x0002|0x0020; /* stack checks, unreferenced, shared strings */
    DataThreshold=32767;
    OptSize=50;
    UndefNames=NULL;
    WholeFName=NULL;
    ObjectFileName=NULL;
    ModuleName=NULL;
    ErrLimit=20;
    WngLevel=1;
    /* Unlike the later source's qword default, this target starts packed. */
    PackAmount=1;
    PreProcChar='#';
    DefaultInfo.class=0;
    DefaultInfo.code=NULL;
    DefaultInfo.parms=DefaultParms;
    DefaultInfo.returns=0;
    DefaultInfo.streturn=0;
    DefaultInfo.save=~0U;
    DefaultInfo.use=0;
    DefaultInfo.objname=NULL;
    CompFlags.signed_char=0;
    CompFlags.check_syntax=0;
    CompFlags.model_options=0x18;
    CompFlags.extensions_enabled=1;
    CompFlags.inline_functions=0;
    CompFlags.dump_prototypes=0;
    CompFlags.generate_prototypes=0;
    CompFlags.bss_segment_used=0;
    CompFlags.zu_switch_used=0;
    CompFlags.errfile_written=0;
    CompFlags.register_conventions=0;
    CompFlags.emit_library_with_main=1;
    CompFlags.undefine_all_macros=0;
    CompFlags.emit_dependencies=1;
    CompFlags.use_unicode=1;
    CompFlags.no_debug_type_names=0;
}

void PreDefine_Macro(char *str)
{
    struct undef_names *uname;
    char *p;
    unsigned length;
    if (CompFlags.undefine_all_macros) return;
    if (UndefNames!=NULL) {
        p=str;
        length=0;
        while (*p!='\0' && !EqualChar((unsigned char)*p)) { ++p; ++length; }
        for (uname=UndefNames;uname;uname=uname->next) {
            if (memcmp(uname->name,str,length)==0 && uname->name[length]=='\0') return;
        }
    }
    /* This target passes flags0, before the later CAN_BE_REDEFINED bit. */
    Def_Macro_Tokens(str,1,0);
}

char *Def_Macro_Tokens(char *str,int multiple_tokens,int flags)
{
    int i,length,one;
    MEPTR entry;
    FCB tmp_file,*old_file;

    str=copy_eq(Buffer,str);
    length=strlen(Buffer);
    if (length==0) { CErr1(0x489); return str; }
    entry=(MEPTR)CMemAlloc(offsetof(MEDEFN,macro_name)+length+1);
    strcpy(entry->macro_name,Buffer);
    entry->parm_count=0;
    i=0;
    if (!EqualChar((unsigned char)*str)) {
        /* This version stores a typed binary constant before its spelling,
         * rather than the later T_PPNUMBER text-only representation. */
        TokenBuf[0]=T_CONSTANT;
        TokenBuf[1]=5;
        one=1;
        memcpy(TokenBuf+2,&one,sizeof(one));
        TokenBuf[6]='1';
        TokenBuf[7]='\0';
        i=8;
    } else {
        old_file=SrcFile;
        SrcFile=&tmp_file;
        ReScanInit(++str);
        for (;;) {
            if (*str=='\0') break;
            ReScanToken();
            if (ReScanPos()==str) break;
            if (CurToken==T_WHITE_SPACE || CurToken==T_BAD_CHAR) break;
            TokenBuf[i++]=(char)CurToken;
            switch (CurToken) {
            case T_CONSTANT:
                i=SaveConstantToken(i);
                /* fall through */
            case T_ID:
                ++TokenLen;
                /* fall through */
            case T_STRING:
            case T_LSTRING:
                memcpy(TokenBuf+i,Buffer,TokenLen);
                i+=TokenLen;
                break;
            }
            str=ReScanPos();
            if (!multiple_tokens) break;
        }
        SrcFile=old_file;
    }
    TokenBuf[i]=T_NULL;
    MacroAdd(entry,TokenBuf,i+1,flags);
    return str;
}

char *copy_eq(char *destination,char *source)
{
    unsigned char c;
    for (;;) {
        c=(unsigned char)*source;
        if (c=='\0' || EqualChar(c) || c==' ' || c=='-' || c==SwitchChar) break;
        *destination++=c;
        ++source;
    }
    *destination='\0';
    return source;
}

char *Define_Macro(char *str)
{
    return Def_Macro_Tokens(str,CompFlags.extended_defines,0);
}

void MiscMacroDefs(void)
{
    struct undef_names *name;
    if (CompFlags.inline_functions) Define_Macro("__INLINE_FUNCTIONS__");
    if (!CompFlags.extensions_enabled) Define_Macro("NO_EXT_KEYS");
    else Define_Extensions();
    if (CompFlags.signed_char) Define_Macro("__CHAR_SIGNED__");
    PreDefine_Macro("__WATCOMC__=1000");
    while (UndefNames!=NULL) {
        name=UndefNames;
        UndefNames=name->next;
        CMemFree(name->name);
        CMemFree(name);
    }
}

void Define_Extensions(void)
{
    PreDefine_Macro("_far16=__far16");
    PreDefine_Macro("_Far16=__far16");
    PreDefine_Macro("near=__near");
    PreDefine_Macro("far=__far");
    PreDefine_Macro("huge=__huge");
    PreDefine_Macro("cdecl=__cdecl");
    PreDefine_Macro("pascal=__pascal");
    PreDefine_Macro("fortran=__fortran");
    PreDefine_Macro("interrupt=__interrupt");
    PreDefine_Macro("_near=__near");
    PreDefine_Macro("_far=__far");
    PreDefine_Macro("_huge=__huge");
    PreDefine_Macro("_cdecl=__cdecl");
    PreDefine_Macro("_pascal=__pascal");
    PreDefine_Macro("_fortran=__fortran");
    PreDefine_Macro("_interrupt=__interrupt");
    PreDefine_Macro("_export=__export");
    PreDefine_Macro("_loadds=__loadds");
    PreDefine_Macro("_saveregs=__saveregs");
    PreDefine_Macro("_syscall=__syscall");
    PreDefine_Macro("_based=__based");
    PreDefine_Macro("_self=__self");
    PreDefine_Macro("_segname=__segname");
    PreDefine_Macro("_segment=__segment");
    PreDefine_Macro("_System=__syscall");
    PreDefine_Macro("_Cdecl=__cdecl");
    PreDefine_Macro("_Pascal=__pascal");
    PreDefine_Macro("SOMLINK=__syscall");
}

/* Descriptive name for the old binary token payload helper. The later
 * released preprocessor stores numeric spellings instead of this value. */
int SaveConstantToken(int index)
{
    int value;
    const void *source;
    TokenBuf[index++]=(char)ConstType;
    if (ConstType==TYPE_CHAR || ConstType==TYPE_INT || ConstType==TYPE_UINT) {
        value=(int)Constant;
        source=&value;
    } else source=&Constant;
    memcpy(TokenBuf+index,source,4);
    return index+4;
}
