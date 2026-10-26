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

#ifndef ASMPARSE_H
#define ASMPARSE_H
#include "asminit.h"
#include <recon.h>
#define ERROR -1
#define STRING_EXPANDED 2
#define NOT_ERROR 1
#define EMPTY -2
#define TRUE 1
#define FALSE 0
#define W_BIT 1
#define NOT_W_BIT 0xfe
#define BIT_012 7
#define NOT_BIT_012 0xf8
#define BIT_67 0xc0
#define MOD_11 0xc0
#define NOT_EMPTY 2
#define SCALE_FACTOR_1 0
#define SCALE_FACTOR_2 0x40
#define SCALE_FACTOR_4 0x80
#define SCALE_FACTOR_8 0xc0
#define OPND1 0
#define OPND2 1
#define BYTE_2 2
#define BYTE_4 4
#define P_CPU_MASK 0x70
#define P_FPU_MASK 7
#define P_NO87 0
#define P_87 1
#define F_16 1
#define F_0F 2
#define F_32 3
#define LOCK 1
#define REPxx 2
#define FWAIT 3
#define R_in_OP 3
#define no_RM 1
#define no_WDS 2
#define ADRSIZ 0x67
#define OPSIZ 0x66
#define OP_NOP 0x90
#define OP_WAIT 0x9b
#define EXTENDED_OPCODE 0x0f
#define NOT_BIT_67 0x3f
#define NOT_BIT_345 0xc7
#define P_386 0x30
#define P_PM 8
#define P_286p 0x28
#define P_486 0x40
#define P_586 0x50
#define FIX_SEG 0
#define FIX_RELOFF8 1
#define FIX_RELOFF16 2
#define FIX_RELOFF32 3
#define FIX_OFF16 4
#define FIX_OFF32 5
#define FIX_PTR16 6
#define FIX_PTR32 7
#define SCRAP_INSTRUCTION 3
#define INDIRECT_JUMP 4
#define IS_BRANCH(t) (((t)>=T_JA && (t)<=T_JZ) || (t)==T_CALL || (t)==T_CALLF)
#define IS_ANY_BRANCH(t) (IS_BRANCH(t) || ((t)>=T_LOOP && (t)<=T_LOOPZ))
#define MEM_TYPE(t,k) ((t)==T_##k || (t)==T_S##k)
/* Packed25-byte assembler fixup, proved by AddFixup allocation/stores
 * and BackPatch consumers; distinct from the WOMP fixup record. */
#pragma pack(1)
struct asmfixup {
    struct asmfixup *next;
    char *name;
    unsigned long offset, fix_loc;
    unsigned char fix_type, external;
    signed char frame;
    unsigned short frame_datum;
    dir_node *def_seg;
};
#pragma pack()
typedef char asmfixup_offset_check[offsetof(struct asmfixup,offset) == 8 ? 1 : -1];
typedef char asmfixup_size_check[sizeof(struct asmfixup) == 25 ? 1 : -1];
typedef char asmfixup_segment_check[offsetof(struct asmfixup,def_seg) == 21 ? 1 : -1];
extern struct asmfixup *InsFixups[3];
extern unsigned char More_Array_Element, Last_Element_Size, Opnd_Count, CheckSeg;
extern char ConstantOnly;
extern signed char Frame;
extern unsigned char Frame_Datum;
int proc_check(void);
void MakeCPUConstant(long token);
void MakeConstant(long token);
int comp_opt(unsigned short token);
int WritePrologue(void);
int dup_array(asm_sym *sym, char start, char size);
int array_element(asm_sym *sym, char start, char size);
void little_endian(char *text, char size);
void output_float(char index, char size, char negative);
void double2tbyte(double value, char *bytes);
int ChangeCurrentLocation(unsigned char relative, long amount);
void AsmDataByte(unsigned char value);
int store_fixup(int index);
struct fixup *CreateFixupRec(int index);
int Ret(int index, int count);
int check_override(int *index);
void check_assume(int default_reg);
int FixOverride(int index);
extern signed char Label_Idx;
extern unsigned char AssumeError;
asm_sym *AsmLookup(char *name);
dir_node *GetSeg(asm_sym *sym);
void find_frame(asm_sym *sym);
struct asmfixup *AddFixup(asm_sym *sym, int type);
int data_init(int name, int index);
int GetStructSize(int index);
int InitializeStructure(asm_sym *sym, int index);
int AddFieldToStruct(int index);
int BackPatch(asm_sym *sym);
void AsmByte(unsigned char value);
int operator(int token, unsigned char direct);
int directive(int index, long token);
int mem(int index);
int mem2code(char scale, int index, int base);
int Reg386(int token);
int comp_mem(int first, int second);
void seg_override(int reg);
unsigned GetPrefixAssume(asm_sym *sym, unsigned def);
unsigned GetAssume(asm_sym *sym, unsigned def);
dir_node *get_grp(asm_sym *sym);
unsigned search_assume(dir_node *segment, unsigned def);
unsigned GetDirIdx(char *name, int table);
void add_frame(void);
int jmp(int index);
void jumpExtend(void);
#define IS_JMP(t) ((t) >= T_JA && (t) <= T_JZ)
int idata(long value);
int MakeLabel(char *name, int type);
int AsmChangeName(char *old_name, char *new_name);
int reg(int index);
unsigned char get_sr_rm_byte(unsigned char prefix);
int check_size(void);
int OperandSize(unsigned long operand);
int InRange(unsigned long value, unsigned bytes);
int match_phase_1(void);
int match_phase_2(int *index);
int match_phase_3(int *index, unsigned long determinant);
int output_data(unsigned long determinant, int operand);
void mark_fixupp(unsigned long determinant, int operand);
int asmmatch_output(int index);
void AsmCodeByte(unsigned char value);
void AddLinnumData(void);
int AddFloatingPointEmulationFixup(const struct asm_ins *ins, unsigned char second);
#endif
