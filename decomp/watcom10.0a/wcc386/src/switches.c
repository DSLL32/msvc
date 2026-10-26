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


#include <ctype.h>
#include <string.h>
#include "options.h"
#include "memory.h"
#include <recon.h>

struct peg_options PegOptions;
unsigned char CpuModel,FpuMode,FpuLevel;
unsigned ProEpiDataSize,UnrollCount;

/* This version predates the later table of option-handler function pointers.
 * Keep the actual in-function dispatch and the old helper call identities. */
char *ProcessOption(char *option)
{
    char *p,*name;
    int first,second,ch,value;
    first=tolower((unsigned char)*option);
    p=option+1;
    second=tolower((unsigned char)*p);
    switch (first) {
    case '3': case '4': case '5':
        CpuModel=first-'/';
        if (second=='r') { CompFlags.register_conventions=1; ++p; }
        else if (second=='s') { CompFlags.register_conventions=0; ++p; }
        break;
    case '7':
        FpuMode=3;
        p=ProcessFPUStack(p);
        break;
    case 'd':
        if (*p>='0' && *p<='9') {
            value=*p++-'0';
            switch (value) {
            case 0: break;
            case 1:
                GenSwitches |= 0x40000000U; /* NUMBERS */
                if (*p=='+') {
                    CompFlags.debug_info_some=1;
                    GenSwitches |= 0x03000000U; /* DBG_TYPES | DBG_LOCALS */
                    ++p;
                }
                break;
            case 3:
                CompFlags.dump_types_with_names=1;
                /* fall through */
            case 2:
                GenSwitches |= 0xc3000000U;
                CompFlags.inline_functions=0;
                if (*p=='~') { CompFlags.no_debug_type_names=1; ++p; }
                break;
            default: p=BadCmdLine(0x431,p-3); break;
            }
        } else if (*p=='+') {
            CompFlags.extended_defines=1;
            ++p;
        } else if (second=='b' && OptionDelimiter((unsigned char)p[1])) {
            CompFlags.emit_browser_info=1;
            ++p;
        } else p=DefineCmdLineMacro(p);
        break;
    case 'e':
        switch (second) {
        case 'z': TargetSwitches |= 0x2U; ++p; break; /* EZ_OMF */
        case 'i': CompFlags.make_enums_an_int=1; ++p; break;
        case 'n': CompFlags.emit_names=1; ++p; break;
        case 'p':
            CompFlags.ep_switch_used=1;
            ++p;
            if (EqualChar((unsigned char)*p)) ++p;
            ProEpiDataSize=0;
            while (*p>='0' && *p<='9') ProEpiDataSize=ProEpiDataSize*10+*p++-'0';
            break;
        case 'e': CompFlags.ee_switch_used=1; ++p; break;
        case 'c': CompFlags.ec_switch_used=1; ++p; break;
        default:
            value=0;
            while (*p>='0' && *p<='9') value=value*10+*p++-'0';
            ErrLimit=value;
            break;
        }
        break;
    case 'g':
        if (EqualChar((unsigned char)*p)) ++p;
        GenCodeGroup=(char *)CMemAlloc(fndlen(p)+1);
        p=copy_eq(GenCodeGroup,p);
        break;
    case 'h':
        ++p;
        switch (second) {
        case 'c': GenSwitches |= 0x08000000U; break; /* DBG_CV */
        case 'd': GenSwitches |= 0x10000000U; break; /* DBG_DF */
        case 'w': break;
        default: p=BadCmdLine(0x431,p-3); break;
        }
        break;
    case 'm': p=ProcessMemoryModel(p); break;
    case 'n': p=ProcessSegmentName(p); break;
    case 'o': p=ProcessOptimization(p); break;
    case 'p':
        CompFlags.cpp_output_requested=1;
        for (;;) {
            ch=tolower((unsigned char)*p);
            if (ch=='l') { CompFlags.cpp_line_wanted=1; ++p; }
            else if (ch=='c') {
                CompFlags.keep_comments=1;
                CompFlags.comments_wanted=1;
                ++p;
            } else if (ch=='w') {
                ++p;
                if (EqualChar((unsigned char)*p)) ++p;
                value=0;
                while (*p>='0' && *p<='9') value=value*10+*p++-'0';
                if (value!=0 && value<20) value=20;
                if (value>10000) value=10000;
                SetCppWidth(value);
            } else if (EqualChar((unsigned char)*p)) {
                PreProcChar=(unsigned char)p[1];
                p+=2;
            } else break;
        }
        break;
    case 'r':
        if (second=='i') { CompFlags.returns_promoted=1; ++p; }
        else CompFlags.save_restore_segregs=1;
        break;
    case 's':
        if (second=='g') { CompFlags.sg_switch_used=1; ++p; }
        else if (second=='t') { CompFlags.st_switch_used=1; ++p; }
        else Toggles &= ~1U;
        break;
    case 'b':
        ++p;
        switch (second) {
        case 'm': CompFlags.bm_switch_used=1; break;
        case 'd': CompFlags.bd_switch_used=1; break;
        case 'w': CompFlags.bw_switch_used=1; break;
        case 't':
            if (EqualChar((unsigned char)*p)) ++p;
            name=p;
            while (isalnum((unsigned char)*p) || *p=='_') ++p;
            SetTargName(name,p-name);
            break;
        default: p=BadCmdLine(0x431,p-3); break;
        }
        break;
    case 'f': p=ProcessFileOption(p); break;
    case 'i': p=AddIncludePath(p); break;
    case 'j': SetSignedChar(); CompFlags.signed_char=1; break;
    case 'u': p=AddUndefName(p); break;
    case 'v': CompFlags.dump_prototypes=CompFlags.generate_prototypes=1; break;
    case 'w':
        if (*p>='0' && *p<='9') {
            value=0;
            while (*p>='0' && *p<='9') value=value*10+*p++-'0';
            WngLevel=value;
        } else {
            ch=tolower((unsigned char)*p++);
            if (ch=='e') CompFlags.warnings_cause_bad_exit=1;
            else if (ch=='o') CompFlags.using_overlays=1;
            else if (ch=='x') WngLevel=4;
        }
        break;
    case 'x':
        if (p[0]=='g' && p[1]=='v') {
            GenSwitches |= 0x1000U; /* INDEXED_GLOBALS */
            p+=2;
        } else p=BadCmdLine(0x431,p-2);
        break;
    case 'z':
        if (second>='0' && second<='9') {
            UnrollCount=second-'0';
            ++p;
            break;
        }
        ++p;
        switch (second) {
        case 'a':
            CompFlags.extensions_enabled=0;
            CompFlags.unique_functions=1;
            TargetSwitches &= ~1U; /* I_MATH_INLINE */
            break;
        case 'e': CompFlags.extensions_enabled=1; break;
        case 'c':
            CompFlags.strings_in_code_segment=1;
            CompFlags.zc_switch_used=1;
            TargetSwitches |= 0x10000U; /* CONST_IN_CODE */
            break;
        case 'd':
            ch=tolower((unsigned char)*p++);
            if (ch=='f') { PegOptions.ds_used=1; PegOptions.ds_on=0; }
            else if (ch=='p') { PegOptions.ds_used=1; PegOptions.ds_on=1; }
            else if (ch=='l') TargetSwitches |= 0x40000U;
            else p=BadCmdLine(0x431,p-4);
            break;
        case 'f':
            PegOptions.fs_used=1;
            ch=tolower((unsigned char)*p++);
            if (ch=='f') PegOptions.fs_on=0;
            else if (ch=='p') PegOptions.fs_on=1;
            else p=BadCmdLine(0x431,p-4);
            break;
        case 'g':
            ch=tolower((unsigned char)*p);
            if (ch=='f' || ch=='p') {
                PegOptions.gs_used=1;
                PegOptions.gs_on=(ch=='p');
                ++p;
            } else CompFlags.generate_prototypes=1;
            break;
        case 'i': CompFlags.extra_stats_wanted=1; break;
        case 'k':
            CompFlags.use_unicode=0;
            if (*p=='u' || *p=='U') {
                CompFlags.use_unicode=1;
                p=LoadUnicodeTable(p+1);
            } else if (*p>='0' && *p<='2') {
                SetDBChar(*p-'0');
                if (*p=='0' && tolower((unsigned char)p[1])=='u') {
                    CompFlags.jis_to_unicode=1;
                    ++p;
                }
                ++p;
            } else SetDBChar(0);
            break;
        case 'l':
            if (tolower((unsigned char)*p)=='d') { CompFlags.emit_dependencies=0; ++p; }
            else CompFlags.emit_library_with_main=0;
            break;
        case 'm':
            CompFlags.multiple_code_segments=1;
            CompFlags.zm_switch_used=1;
            break;
        case 'p':
            if (*p>='0' && *p<='9') {
                PackAmount=*p++-'0';
                if (PackAmount!=1 && PackAmount!=2 && PackAmount!=4 && PackAmount!=8) PackAmount=1;
            }
            break;
        case 'q': CompFlags.quiet_mode=1; break;
        case 's': CompFlags.check_syntax=1; break;
        case 't': p=SetDataThreshold(p); break;
        case 'u': CompFlags.zu_switch_used=1; TargetSwitches |= 0x200U; break;
        case 'w': SetTargName("windows",7); break;
        case 'x': Save8087Regs=1; break;
        default: p=BadCmdLine(0x431,p-3); break;
        }
        break;
    default: p=BadCmdLine(0x431,p-2); break;
    }
    return p;
}

unsigned Inline_Threshold;
char *ProcessOptimization(char *p)
{
    int ch;
    for (;;) {
        ch=tolower((unsigned char)*p);
        switch (ch) {
        case 'a': GenSwitches |= 0x04000000U; break; /* RELAX_ALIAS */
        case 'c': TargetSwitches |= 0x8000U; break; /* NO_CALL_RET_TRANSFORM */
        case 'd': GenSwitches |= 0x80000000U; break; /* NO_OPTIMIZATION */
        case 'e':
            ++p;
            Inline_Threshold=0;
            if (EqualChar((unsigned char)*p)) ++p;
            while (isdigit((unsigned char)*p)) Inline_Threshold=Inline_Threshold*10+*p++-'0';
            --p;
            if (Inline_Threshold==0) Inline_Threshold=20;
            Toggles |= 8U; /* TOGGLE_INLINE */
            break;
        case 'f':
            TargetSwitches |= 0x20000U; /* NEED_STACK_FRAME */
            if (p[1]=='+') { DefaultInfo.class |= 0x01000000U; ++p; }
            break;
        case 'i': CompFlags.inline_functions=1; break;
        case 'l':
            GenSwitches |= 0x00400000U; /* LOOP_OPTIMIZATION */
            if (p[1]=='+') { GenSwitches |= 0x00800000U; ++p; }
            break;
        case 'm': TargetSwitches |= 1U; break; /* I_MATH_INLINE */
        case 'n': GenSwitches |= 0x00080000U; break; /* FP_UNSTABLE_OPTIMIZATION */
        case 'o': GenSwitches &= ~0x00100000U; break; /* MEMORY_LOW_FAILS */
        case 'p': CompFlags.op_switch_used=1; break;
        case 'r': GenSwitches |= 0x00200000U; break; /* INS_SCHEDULING */
        case 's': GenSwitches &= ~0x80000000U; OptSize=100; break;
        case 't': GenSwitches &= ~0x80000000U; OptSize=0; break;
        case 'u': CompFlags.unique_functions=1; break;
        case 'x':
            Toggles &= ~1U;
            GenSwitches &= ~0x80000000U;
            GenSwitches |= 0x00600000U;
            CompFlags.inline_functions=1;
            TargetSwitches |= 1U;
            break;
        case 'z': GenSwitches |= 0x00040000U; break; /* NULL_DEREF_OK */
        default: return BadCmdLine(0x432,p);
        }
        ++p;
        if (*p=='\0' || *p=='-' || *p==' ' || *p=='\t' || *p==SwitchChar) return p;
    }
}

char *ProcessFileOption(char *option)
{
    char *p,*name;
    unsigned length;
    int kind,sub;
    kind=tolower((unsigned char)*option);
    p=option+1;
    sub=tolower((unsigned char)*p);
    if (kind=='h' && sub=='q') {
        CompFlags.no_pch_warnings=1;
        ++p;
    }
    if (EqualChar((unsigned char)*p)) ++p;
    switch (kind) {
    case 'h': case 'i': case 'o':
        length=FileOptionLength(p);
        name=(char *)CMemAlloc(length+1);
        memcpy(name,p,length);
        name[length]='\0';
        p+=length;
        if (kind=='i') ForceInclude=name;
        else if (kind=='h') {
            PCH_FileName=name;
            if (length==0) {
                CMemFree(name);
                PCH_FileName="wcc386.pch";
            }
        } else {
            CompFlags.cpp_output_to_file=1;
            ObjectFileName=name;
        }
        break;
    case 'p':
        sub=tolower((unsigned char)option[1]);
        p=option+2;
        switch (sub) {
        case '2': FpuLevel=1; break;
        case '3': FpuLevel=2; break;
        case '5': FpuLevel=3; break;
        case 'r': Stack87=4; break;
        case 'i':
            if (p[0]=='8' && p[1]=='7') { FpuMode=3; p+=2; }
            else FpuMode=2;
            return ProcessFPUStack(p);
        case 'c': FpuMode=1; break;
        default: p=BadCmdLine(0x431,option-2); break;
        }
        break;
    default: p=BadCmdLine(0x431,option-2); break;
    }
    return p;
}

unsigned FileOptionLength(char *text)
{
    unsigned length=0;
    while (*text!='\0' && *text!=' ' && *text!='\t' && (unsigned char)*text!=SwitchChar) {
        ++text;
        ++length;
    }
    return length;
}
