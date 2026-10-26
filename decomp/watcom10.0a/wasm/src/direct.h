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

/* Adapted from bld/wasm/c/direct.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9, CFfa3c..fce6.
 * The shipped compiler inlines dir_add; retain those operations in this body.
 * Allocation layouts and switch IDs are the 1994 layouts, not newer additions. */
#ifndef WASM_DIRECT_H
#define WASM_DIRECT_H
#include "asminit.h"
#include <recon.h>
enum {
    TOK_TINY=16,TOK_SMALL,TOK_COMPACT,TOK_MEDIUM,TOK_LARGE,TOK_HUGE,
    TOK_FLAT,TOK_NEARSTACK,TOK_FARSTACK,
    TOK_PROC_FAR=47,TOK_PROC_NEAR,
    TOK_PROC_BASIC=49,TOK_PROC_FORTRAN,TOK_PROC_PASCAL,TOK_PROC_C,
    TOK_PROC_WATCOM_C,TOK_PROC_STDCALL,TOK_PROC_SYSCALL,
    TOK_PROC_PRIVATE=56,TOK_PROC_PUBLIC,TOK_PROC_EXPORT,TOK_PROC_USES,TOK_PROC_VARARG,
    TOK_OS_OS2=61,TOK_OS_DOS
};
enum {
    MOD_NONE,MOD_TINY,MOD_SMALL,MOD_COMPACT,MOD_FLAT,MOD_MEDIUM,MOD_LARGE,MOD_HUGE
};
enum {
    TOK_READONLY=0,TOK_BYTE,TOK_WORD,TOK_DWORD,TOK_PARA,TOK_PAGE,
    TOK_PRIVATE,TOK_PUBLIC,TOK_STACK,TOK_COMMON,TOK_MEMORY,
    TOK_USE16,TOK_USE32,TOK_IGNORE,TOK_AT
};
enum { TOK_EXT_NEAR=25,TOK_EXT_FAR,TOK_EXT_PROC,TOK_EXT_BYTE,TOK_EXT_SBYTE,
       TOK_EXT_WORD,TOK_EXT_SWORD,TOK_EXT_DWORD,TOK_EXT_SDWORD,TOK_EXT_PWORD,
       TOK_EXT_FWORD,TOK_EXT_QWORD,TOK_EXT_TBYTE,TOK_EXT_ABS };
enum { TOK_DS=39,TOK_ES,TOK_SS,TOK_FS,TOK_GS,TOK_CS,TOK_ERROR,TOK_NOTHING };
enum { ALIGN_ABS, ALIGN_BYTE, ALIGN_WORD, ALIGN_PARA, ALIGN_PAGE,
       ALIGN_DWORD, ALIGN_4KPAGE };
#define COMB_INVALID 0
#define LNAME_NULL 0
extern seg_list *CurrSeg;
void AsmTakeOut(char *name);
void write_epilogue(void);
void close_lastseg(void);
int MakeComm(char *name, int type, unsigned char already_defd, int number, int distance);
void pop_register(struct regs_list *regist);
void push_proc(dir_node *procedure);
dir_node *pop_proc(void);
int proc_exam(int index);
int find_size(int type);
void size_override(char *buffer, int size);
int get_watcom_argument_string(char *buffer, unsigned char size, unsigned char *count);
int FindClassLnameIdx(char *name);
int GetLnameIdx(char *name);
char *Check4Mangler(int *index);
void SetMangler(asm_sym *symbol, char *type);
char *Mangle(asm_sym *symbol, char *buffer);
asm_sym *MakeExtern(char *name, int type, unsigned char already_defined);
extern qdesc *PubQueue;
int InsertClassLname(char *name);
int push_seg(dir_node *segment);
typedef struct stacknode { struct stacknode *next; void *elt; } stacknode;
void push(void **stack, void *element);
dir_node *pop_seg(void);
void *pop(void **stack);
void GetSymInfo(asm_sym *symbol);
unsigned GetCurrSeg(void);
unsigned GetCurrGrp(void);
unsigned long GetCurrAddr(void);
unsigned long GetCurrSegStart(void);
unsigned GetGrpIdx(asm_sym *sym);
void find_use32(void);
#define INIT_MEMORY 0x10
enum { SIM_CODE, SIM_STACK, SIM_DATA, SIM_DATA_UN, SIM_FARDATA, SIM_FARDATA_UN, SIM_CONST, SIM_NONE };
#define SIM_CODE_OFFSET 5
#define SIM_DATA_OFFSET 5
#define FAROFFSET(seg) ((seg) == SIM_FARDATA ? 8 : 7)
void input_group(int type);
unsigned long GetCurrSegAlign(void);
void fill_in_objfile_space(unsigned size);
typedef struct typeinfo { char *string; unsigned value, init; } typeinfo;
extern typeinfo TypeInfo[63];
void get_module_name(void);
void wipe_space(char *token);
int token_cmp(char **token, int start, int end);
unsigned checkword(char **token);
void set_text_seg_name(void);
void module_prologue(int model);
int find_bit(void);
#endif
