#ifndef LINK_DISASSEMBLER_H
#define LINK_DISASSEMBLER_H
#include <stddef.h>
#include <setjmp.h>
#include "dump.h"
#include <recon.h>

enum DisassemblyPrefix { DIS_PREFIX_REPNZ=213 };

enum DisassemblyRegister {
    DIS_REG_AL=1, DIS_REG_CX=10, DIS_REG_ECX=18, DIS_REG_AX=9, DIS_REG_EAX=17, DIS_REG_SP=13, DIS_REG_BP=14, DIS_REG_BX=12, DIS_REG_SI=15, DIS_REG_DI=16,
    DIS_REG_ESP=21, DIS_REG_EBP=22, DIS_REG_EBX=20, DIS_REG_ESI=23, DIS_REG_EDI=24,
    DIS_REG_ES=25, DIS_REG_CS=26, DIS_REG_SS=27, DIS_REG_DS=28, DIS_REG_FS=29
};

/* Decoder scratch address and formatted operands use distinct layouts. */
typedef struct DecodedAddress {
    unsigned long segment,base,index,scale,value_bytes;
    long value;
    unsigned long selector;
} DecodedAddress;
typedef struct DisassemblyOperand {
    unsigned long kind,size;
    DecodedAddress address;
} DisassemblyOperand;
typedef struct DisassembledInstruction {
    unsigned char category,operation;
    unsigned long length,address,next_address;
    unsigned long target_address,registers_read,registers_written;
    unsigned long flags_read,flags_written,flags_undefined;
    unsigned long code_mode,data_mode;
    unsigned long default32,address32,operand32;
    unsigned long prefix,segment,mnemonic;
    unsigned char *modrm,*bytes;
    DecodedAddress effective_address;
    long immediate;
    unsigned short selector;
    DisassemblyOperand operands[3];
} DisassembledInstruction;
typedef char DisassemblerOperandIs36[sizeof(DisassemblyOperand)==36?1:-1];
typedef char DisassemblerSizeIs224[sizeof(DisassembledInstruction)==224?1:-1];
typedef unsigned char *(*OperandDecoder)(unsigned char *,int,DisassembledInstruction *);
typedef struct OpcodeEntry {
    unsigned short mnemonic,flags_written,flags_read,flags_undefined;
    OperandDecoder first,second;
    unsigned char operation;
} OpcodeEntry;
typedef char OpcodeEntryIs20[sizeof(OpcodeEntry)==20?1:-1];
extern const unsigned long register_masks[32];
extern const OpcodeEntry disassembly_signed_byte_group[8];
extern const OpcodeEntry disassembly_shift_group[8];
extern const long disassembly_extended_pages[16];
extern const OpcodeEntry disassembly_extended_opcodes[112];
unsigned char *decode_modrm_operand(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_address16(unsigned char *,const unsigned long *,DecodedAddress *);
unsigned char *decode_address32(unsigned char *,const unsigned long *,DecodedAddress *);
unsigned char *decode_sib_address(unsigned char *,unsigned long,DecodedAddress *);
extern const OpcodeEntry opcode_table[256];
extern const unsigned char instruction_categories[22];
unsigned char *apply_opcode(unsigned char *,const OpcodeEntry *,DisassembledInstruction *);
unsigned char *decode_extended_opcode(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b650(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b670(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_signed_byte_group(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b6d0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_shift_group(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b730(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b760(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b790(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b7c0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_operand_size_prefix(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_address_size_prefix(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_repe_prefix(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_repne_prefix(unsigned char *,int,DisassembledInstruction *);
unsigned char *update_stack_registers(unsigned char *,int,DisassembledInstruction *);
unsigned char *update_frame_registers(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b960(unsigned char *,int,DisassembledInstruction *);
unsigned char *update_counter_registers(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b9c0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4b9f0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4ba10(unsigned char *,int,DisassembledInstruction *);
unsigned char *update_al_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4ba70(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4ba90(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bac0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bae0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bb10(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bb50(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bb90(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bbd0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bc10(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bc50(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bc90(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bcd0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bd10(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bd40(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bd70(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_ds_prefix(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_fs_prefix(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4be00(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_string_byte_destination(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4be90(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_string_byte_source(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bf70(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4bfe0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c060(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_byte_immediate(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c0b0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_immediate(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c140(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c170(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c1a0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_short_relative(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_near_relative(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c3d0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_absolute_memory(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c4c0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_modrm_operand_effects(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_modrm_byte_operand(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c830(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c910(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c940(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4c970(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_memory_operand(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4ca60(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_x87_d8(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_x87_d9(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4cdf0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_x87_db(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_x87_dc(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_x87_dd(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_x87_de(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_x87_df(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4d5e0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4d640(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4d690(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_modrm_byte_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_modrm_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4d8a0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4d900(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4d990(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4da30(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4da70(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dab0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4daf0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dbb0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dbf0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dc30(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dc70(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dcb0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dcf0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dd30(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dd70(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_eax_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4de00(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_eax_destination(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_ebx_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_ecx_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4df20(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4df50(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4dfa0(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_ebp_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_esi_register(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_edi_register(unsigned char *,int,DisassembledInstruction *);
extern jmp_buf disassembler_jump;
DisassembledInstruction *create_disassembler(void);
unsigned long decode_instruction(unsigned char *,unsigned long,int,DisassembledInstruction *);
unsigned char *decode_opcode(unsigned char *,DisassembledInstruction *);
unsigned char *decode_instruction_body(unsigned long,unsigned char *,DisassembledInstruction *);
char *format_instruction(unsigned char,DisassembledInstruction *);
char *format_instruction_address(unsigned long);
unsigned long format_instruction_bytes(DisassembledInstruction *,unsigned long,char *);
unsigned long select_disassembly_mnemonic(DisassembledInstruction *);
char *format_disassembly_operand(DisassembledInstruction *,const DisassemblyOperand *,char *);
char *format_instruction_text(DisassembledInstruction *,unsigned long,char *);
int compare_disassembly_symbols(const void *,const void *);

unsigned char *decode_op_4b7f0(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4b820(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4b850(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4c530(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4c6d0(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4c710(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d520(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d560(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d5a0(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_register_and_byte_immediate(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d810(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d850(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d8d0(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d930(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d960(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4d9d0(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4da00(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4db30(unsigned char *,int,DisassembledInstruction *);

unsigned char *decode_op_4db70(unsigned char *,int,DisassembledInstruction *);


extern const OpcodeEntry disassembly_x87_dd_memory[8];
unsigned char *decode_x87_memory(unsigned char *,int,DisassembledInstruction *);
unsigned char *decode_op_4d4e0(unsigned char *,int,DisassembledInstruction *);
unsigned char *size_x87_qword(unsigned char *,int,DisassembledInstruction *);


extern const OpcodeEntry disassembly_x87_d9_memory[8];
unsigned char *size_x87_dword(unsigned char *,int,DisassembledInstruction *);


extern const OpcodeEntry disassembly_x87_d8_memory[8];


extern const OpcodeEntry disassembly_x87_dc_memory[8];


extern const OpcodeEntry disassembly_x87_db_memory[8];
unsigned char *decode_op_4d510(unsigned char *,int,DisassembledInstruction *);


extern const OpcodeEntry disassembly_x87_de_memory[8];


extern const OpcodeEntry disassembly_x87_df_memory[8];

#endif
