/* Dump disassembler callbacks, shared by the pending decoder helpers. */
#include <stddef.h>
#include "disassembler.h"
#include "archive.h"
#include <recon.h>
#pragma intrinsic(memset)

FatalRoutine disassembler_fatal;
WarningRoutine disassembler_warning;
jmp_buf disassembler_jump;

/* Original [0004ae00,0004ae13), cdecl; no internal code data or callees. */
void initialize_disassembler(FatalRoutine fatal,WarningRoutine warning)
{
    disassembler_fatal=fatal;
    disassembler_warning=warning;
}

void print_dump_symbol_name(FILE *,const char *,PIMAGE_SYMBOL);

/* Original [0004ae20,0004b192), cdecl. */
void disassemble_section(IMAGE_SECTION_HEADER *section,unsigned short index,
    PIMAGE_SYMBOL symbols,unsigned long count,unsigned char image,
    DumpStringTable *strings,int file,FILE *stream)
{
    PIMAGE_SYMBOL *ordered,symbol;
    unsigned long symbol_index,symbol_count,next_symbol,offset,end,next_offset;
    unsigned long symbol_value,symbol_base;
    unsigned char *data,*instruction,*next_instruction;
    DisassembledInstruction *state;
    long saved_position,position;
    unsigned long format=0x1f;

    if (symbols && count) {
        ordered=malloc(count*sizeof(*ordered));
        if (!ordered) disassembler_fatal(NULL,5,"");
        symbol_count=0;
        for (symbol_index=0;symbol_index<count;++symbol_index) {
            symbol=&symbols[symbol_index];
            if (symbol->SectionNumber==index &&
                (symbol->StorageClass==IMAGE_SYM_CLASS_EXTERNAL ||
                 (symbol->StorageClass==IMAGE_SYM_CLASS_STATIC &&
                  (!symbol->NumberOfAuxSymbols || (symbol->Type&0x30)==0x20))))
                ordered[symbol_count++]=symbol;
            symbol_index+=symbol->NumberOfAuxSymbols;
        }
        ordered=realloc(ordered,symbol_count*sizeof(*ordered));
        qsort(ordered,symbol_count,sizeof(*ordered),compare_disassembly_symbols);
    } else {
        ordered=NULL;
        symbol_count=0;
    }
    next_symbol=0;
    if (!setjmp(disassembler_jump)) state=create_disassembler();
    else disassembler_fatal(program_name,0x2d,"");
    saved_position=tell_file(file);
    position=section->PointerToRawData+archive_data_offset;
    if (seek_file(file,position,SEEK_SET)==-1)
        disassembler_fatal(program_name,9,position);
    data=malloc(section->SizeOfRawData);
    if (!data) disassembler_fatal(NULL,5,"");
    read_file(file,data,section->SizeOfRawData);
    end=section->VirtualAddress+section->SizeOfRawData;
    offset=section->VirtualAddress;
    instruction=data;
    while (offset<end) {
        while (next_symbol<symbol_count) {
            symbol_base=image?section->VirtualAddress:0;
            symbol_value=ordered[next_symbol]->Value;
            if (offset-section->VirtualAddress<symbol_value-symbol_base) break;
            symbol_value-=image?section->VirtualAddress:0;
            print_dump_symbol_name(stream,"%s",ordered[next_symbol]);
            if (symbol_value!=offset-section->VirtualAddress)
                fprintf(stream," + %x",offset-section->VirtualAddress-symbol_value);
            fprintf(stream,":\n");
            ++next_symbol;
        }
        if (!setjmp(disassembler_jump)) {
            next_offset=decode_instruction(instruction,offset,0,state);
            next_instruction=instruction+state->length;
            fprintf(stream,"%s\n",format_instruction(format,state));
        } else {
            next_offset=offset+1;
            next_instruction=instruction+1;
            fprintf(stream,"    %08lx: [illegal instruction]\n",offset);
        }
        offset=next_offset;
        instruction=next_instruction;
    }
    free(data);
    if (ordered) free(ordered);
    if (seek_file(file,saved_position,SEEK_SET)==-1)
        disassembler_fatal(program_name,9,saved_position);
}

/* Original [0004e8f0,0004e932), including its full 224-byte state. */
DisassembledInstruction *create_disassembler(void)
{
    DisassembledInstruction *state;
    state=malloc(sizeof(*state));
    if (!state) longjmp(disassembler_jump,1);
    memset(state,0,sizeof(*state));
    state->code_mode=2;
    state->data_mode=2;
    return state;
}

/* Original [0004e940,0004e972). The third argument is unused. */
unsigned long decode_instruction(unsigned char *data,unsigned long address,int flags,
    DisassembledInstruction *state)
{
    if (!data) longjmp(disassembler_jump,1);
    decode_instruction_body(address,data,state);
    return state->next_address;
}

/* Original [0004e0e0,0004e13d). Preserve the caller's mode settings. */
unsigned char *decode_instruction_body(unsigned long address,unsigned char *data,
    DisassembledInstruction *state)
{
    unsigned char *next;
    memset(state,0,offsetof(DisassembledInstruction,code_mode));
    memset(&state->default32,0,sizeof(*state)-offsetof(DisassembledInstruction,default32));
    state->default32=state->code_mode==2;
    state->address32=state->default32;
    state->operand32=state->default32;
    state->bytes=data;
    state->address=address;
    next=decode_opcode(data,state);
    state->length=next-data;
    state->next_address=address+state->length;
    return next;
}

/* Original [0004b5c0,0004b5e3). */
unsigned char *decode_opcode(unsigned char *data,DisassembledInstruction *state)
{
    return apply_opcode(data+1,&opcode_table[*data],state);
}

/* Original [0004b550,0004b5bd). */
unsigned char *apply_opcode(unsigned char *data,const OpcodeEntry *opcode,
    DisassembledInstruction *state)
{
    state->category=instruction_categories[opcode->operation];
    state->operation=opcode->operation;
    state->flags_read|=opcode->flags_read;
    state->flags_written|=opcode->flags_written;
    state->flags_undefined|=opcode->flags_undefined;
    state->mnemonic=opcode->mnemonic;
    if (opcode->first) data=opcode->first(data,1,state);
    if (opcode->second) data=opcode->second(data,2,state);
    return data;
}

/* Register aliases include their overlapping byte/word effects. */
const unsigned long register_masks[32]={
    0x0UL,0x1UL,0x2UL,0x4UL,0x8UL,0x10UL,0x20UL,0x40UL,0x80UL,0x111UL,0x222UL,0x444UL,0x888UL,0x1000UL,0x2000UL,0x4000UL,0x8000UL,0x10111UL,0x20222UL,0x40444UL,0x80888UL,0x101000UL,0x202000UL,0x404000UL,0x808000UL,0x1000000UL,0x2000000UL,0x4000000UL,0x8000000UL,0x10000000UL,0x20000000UL,0x0UL
};

/* Original [0004b900,0004b92a). */
unsigned char *update_stack_registers(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ESP:DIS_REG_SP;
    state->registers_read|=register_masks[reg];
    state->registers_written|=register_masks[reg];
    return data;
}

/* Original [0004dff0,0004e03d). */
unsigned char *decode_ebp_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EBP:DIS_REG_BP;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

const unsigned long disassembly_word_registers[8]={9,10,11,12,13,14,15,16};
const unsigned long disassembly_dword_registers[8]={17,18,19,20,21,22,23,24};

/* Original [0004d750,0004d7c9). */
unsigned char *decode_modrm_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    unsigned long reg,index=(*modrm&0x38)>>3;
    DisassemblyOperand *result=&state->operands[operand-1];
    if (state->operand32) reg=disassembly_dword_registers[index];
    else reg=disassembly_word_registers[index];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004c680,0004c6c6). */
unsigned char *decode_modrm_operand_effects(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *next=decode_modrm_operand(data,operand,state);
    DisassemblyOperand *result=&state->operands[operand-1];
    if (result->kind==2) {
        if (operand==1) state->registers_written|=register_masks[result->address.base];
        else state->registers_read|=register_masks[result->address.base];
    }
    return next;
}

/* Original [0004c5a0,0004c677). */
unsigned char *decode_modrm_operand(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    const unsigned long *registers;
    unsigned char *next;
    state->modrm=data;
    if ((*data&0xc0)==0xc0) result->kind=2;
    else {
        result->kind=1;
        result->size=state->operand32?3:2;
    }
    registers=state->operand32?disassembly_dword_registers:disassembly_word_registers;
    if (state->address32) next=decode_address32(data,registers,&result->address);
    else next=decode_address16(data,registers,&result->address);
    if (result->kind==1) {
        if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
        if (result->address.base) state->registers_read|=register_masks[result->address.base];
        if (result->address.index) state->registers_read|=register_masks[result->address.index];
    }
    return next;
}

/* Original [0004b410,0004b546). */
unsigned char *decode_address32(unsigned char *data,const unsigned long *registers,
    DecodedAddress *address)
{
    unsigned long mod,rm;
    unsigned char *next;
    memset(address,0,sizeof(*address));
    mod=*data>>6;
    rm=*data&7;
    next=data+1;
    if (mod==3) {
        address->base=registers[rm];
        return next;
    }
    if (rm==4) return decode_sib_address(next,mod,address);
    if (!mod && rm==5) {
        address->value=*(long *)next;
        address->value_bytes=4;
        address->segment=DIS_REG_DS;
    } else {
        address->base=disassembly_dword_registers[rm];
        address->segment=address->base==DIS_REG_EBP?DIS_REG_SS:DIS_REG_DS;
        if (mod==1) {
            address->value=*next;
            address->value_bytes=1;
        } else if (mod==2) {
            address->value=*(long *)next;
            address->value_bytes=4;
        }
    }
    return next+address->value_bytes;
}

/* Original [0004de80,0004decd). */
unsigned char *decode_ebx_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EBX:DIS_REG_BX;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004e040,0004e08d). */
unsigned char *decode_esi_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ESI:DIS_REG_SI;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004e090,0004e0dd). */
unsigned char *decode_edi_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EDI:DIS_REG_DI;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004c300,0004c3cc). */
unsigned char *decode_near_relative(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    long displacement;
    unsigned long target;
    result->kind=4;
    if (state->operand32) {
        result->address.value_bytes=4;
        displacement=*(long *)data;
        result->address.value=displacement;
    } else {
        result->address.value_bytes=2;
        displacement=*(short *)data;
        *(short *)&result->address.value=(short)displacement;
    }
    data+=result->address.value_bytes;
    target=state->address+(data-state->bytes);
    if (!state->default32) target&=0xffff;
    target+=displacement;
    if (!state->operand32) target&=0xffff;
    state->target_address=target;
    if (!state->default32) {
        if (target>0xffff) longjmp(disassembler_jump,1);
        state->target_address+=state->address&0xffff0000;
    }
    if (operand==1) state->registers_written|=register_masks[DIS_REG_CS];
    else state->registers_read|=register_masks[DIS_REG_CS];
    return data;
}

/* Original [0004b930,0004b95a). */
unsigned char *update_frame_registers(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EBP:DIS_REG_BP;
    state->registers_read|=register_masks[reg];
    state->registers_written|=register_masks[reg];
    return data;
}

/* Original [0004b1a0,0004b1c7). */
int compare_disassembly_symbols(const void *left,const void *right)
{
    unsigned long a=(*(PIMAGE_SYMBOL const *)left)->Value;
    unsigned long b=(*(PIMAGE_SYMBOL const *)right)->Value;
    if (a<b) return -1;
    return a>b;
}

/* Original [0004b310,0004b40b). */
unsigned char *decode_sib_address(unsigned char *data,unsigned long mod,
    DecodedAddress *address)
{
    unsigned long base=*data&7;
    unsigned char *displacement=data+1;
    switch (mod) {
    case 0:
        if (base==5) {
            address->value=*(long *)displacement;
            address->value_bytes=4;
        } else address->base=disassembly_dword_registers[base];
        break;
    case 1:
        address->base=disassembly_dword_registers[base];
        *(unsigned char *)&address->value=*displacement;
        address->value_bytes=1;
        break;
    case 2:
        address->base=disassembly_dword_registers[base];
        address->value=*(long *)displacement;
        address->value_bytes=4;
        break;
    default:
        longjmp(disassembler_jump,1);
    }
    address->scale=*data>>6;
    address->index=disassembly_dword_registers[(*data&0x38)>>3];
    if (address->index==DIS_REG_ESP) {
        address->index=0;
        if (*data&0xc0) longjmp(disassembler_jump,1);
    }
    /* The original conditional SS store is immediately overwritten by DS. */
    address->segment=DIS_REG_DS;
    return displacement+address->value_bytes;
}

/* Original [0004ca10,0004ca52). */
unsigned char *decode_memory_operand(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    data=decode_modrm_operand(data,operand,state);
    if (state->operands[operand-1].kind!=1) longjmp(disassembler_jump,1);
    return data;
}

const unsigned long disassembly_byte_registers[8]={1,2,3,4,5,6,7,8};

/* Original [0004d6f0,0004d74b). */
unsigned char *decode_modrm_byte_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    unsigned long reg=disassembly_byte_registers[(*modrm&0x38)>>3];
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004c750,0004c82a). */
unsigned char *decode_modrm_byte_operand(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    unsigned char *next;
    state->modrm=data;
    if ((*data&0xc0)==0xc0) result->kind=2;
    else {
        result->kind=1;
        result->size=1;
    }
    if (state->address32)
        next=decode_address32(data,disassembly_byte_registers,&result->address);
    else next=decode_address16(data,disassembly_byte_registers,&result->address);
    if (result->kind==2) {
        if (operand==1) state->registers_written|=register_masks[result->address.base];
        else state->registers_read|=register_masks[result->address.base];
    } else {
        if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
        if (result->address.base) state->registers_read|=register_masks[result->address.base];
        if (result->address.index) state->registers_read|=register_masks[result->address.index];
    }
    return next;
}

/* Original [0004b880,0004b89e). */
unsigned char *decode_operand_size_prefix(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->operand32=!state->operand32;
    return decode_opcode(data,state);
}

/* Original [0004bda0,0004bdc3). */
unsigned char *decode_ds_prefix(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->segment=DIS_REG_DS;
    /* Segment-prefix bodies use the ES effect mask in the original. */
    state->registers_read|=register_masks[DIS_REG_ES];
    return decode_opcode(data,state);
}

/* Original [0004ddb0,0004ddfd). */
unsigned char *decode_eax_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EAX:DIS_REG_AX;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004c440,0004c4b7). */
unsigned char *decode_absolute_memory(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=1;
    result->address.segment=DIS_REG_DS;
    if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
    result->size=state->operand32?3:2;
    if (state->address32) {
        result->address.value_bytes=4;
        result->address.value=*(long *)data;
    } else {
        result->address.value_bytes=2;
        *(short *)&result->address.value=*(short *)data;
    }
    return data+result->address.value_bytes;
}

/* Original [0004b8a0,0004b8be). */
unsigned char *decode_address_size_prefix(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->address32=!state->address32;
    return decode_opcode(data,state);
}

const unsigned long disassembly_address16_registers[8][2]={
    {DIS_REG_BX,DIS_REG_SI},{DIS_REG_BX,DIS_REG_DI},
    {DIS_REG_BP,DIS_REG_SI},{DIS_REG_BP,DIS_REG_DI},
    {0,DIS_REG_SI},{0,DIS_REG_DI},{DIS_REG_BP,0},{DIS_REG_BX,0}
};

/* Original [0004b1d0,0004b305). */
unsigned char *decode_address16(unsigned char *data,const unsigned long *registers,
    DecodedAddress *address)
{
    unsigned long mod=*data>>6,rm=*data&7;
    unsigned char *next=data+1;
    memset(address,0,sizeof(*address));
    if (mod==3) address->base=registers[rm];
    else if (!mod && rm==6) {
        *(unsigned short *)&address->value=*(unsigned short *)next;
        address->value_bytes=2;
        address->segment=DIS_REG_DS;
    } else {
        address->base=disassembly_address16_registers[rm][0];
        address->index=disassembly_address16_registers[rm][1];
        address->segment=address->base==DIS_REG_BP?DIS_REG_SS:DIS_REG_DS;
        if (mod==1) {
            *(unsigned char *)&address->value=*next;
            address->value_bytes=1;
        } else if (mod==2) {
            *(unsigned short *)&address->value=*(unsigned short *)next;
            address->value_bytes=2;
        }
    }
    return next+address->value_bytes;
}

/* Original [0004b6a0,0004b6ca). */
unsigned char *decode_signed_byte_group(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,&disassembly_signed_byte_group[(*data&0x38)>>3],state);
}

/* Original [0004c080,0004c0ac). */
unsigned char *decode_byte_immediate(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=3;
    result->address.value_bytes=1;
    *(unsigned char *)&result->address.value=*data;
    return data+1;
}

/* Original [0004b5f0,0004b645). */
unsigned char *decode_extended_opcode(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    long page=disassembly_extended_pages[*data>>4];
    if (page==-1) longjmp(disassembler_jump,1);
    return apply_opcode(data+1,
        &disassembly_extended_opcodes[page|(*data&15)],state);
}

/* Original [0004c0e0,0004c137). */
unsigned char *decode_immediate(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=3;
    if (state->operand32) {
        result->address.value_bytes=4;
        result->address.value=*(long *)data;
    } else {
        result->address.value_bytes=2;
        *(short *)&result->address.value=*(short *)data;
    }
    return data+result->address.value_bytes;
}

/* Original [0004b990,0004b9ba). */
unsigned char *update_counter_registers(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ECX:DIS_REG_CX;
    state->registers_read|=register_masks[reg];
    state->registers_written|=register_masks[reg];
    return data;
}

/* Original [0004c250,0004c2fa). */
unsigned char *decode_short_relative(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    long displacement=(signed char)*data;
    unsigned long target;
    result->kind=4;
    result->address.value_bytes=1;
    *(unsigned char *)&result->address.value=*data++;
    target=state->address+(data-state->bytes);
    if (!state->default32) target&=0xffff;
    target+=displacement;
    if (!state->operand32) target&=0xffff;
    state->target_address=target;
    if (!state->default32) {
        if (target>0xffff) longjmp(disassembler_jump,1);
        state->target_address+=state->address&0xffff0000;
    }
    if (operand==1) state->registers_written|=register_masks[DIS_REG_CS];
    else state->registers_read|=register_masks[DIS_REG_CS];
    return data;
}

/* Original [0004b8c0,0004b8da). */
unsigned char *decode_repe_prefix(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    /* The F3 prefix selects the original "repnz" display entry. */
    state->prefix=DIS_PREFIX_REPNZ;
    return decode_opcode(data,state);
}

/* Original [0004be30,0004be8c). */
unsigned char *decode_string_byte_destination(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    unsigned long reg=state->address32?DIS_REG_EDI:DIS_REG_DI;
    result->kind=1;
    result->address.segment=DIS_REG_ES;
    state->registers_read|=register_masks[DIS_REG_ES];
    result->size=1;
    result->address.base=reg;
    state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bf00,0004bf64). */
unsigned char *decode_string_byte_source(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    unsigned long reg=state->address32?DIS_REG_ESI:DIS_REG_SI;
    result->kind=1;
    result->address.segment=DIS_REG_DS;
    if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
    result->size=1;
    result->address.base=reg;
    state->registers_read|=register_masks[reg];
    return data;
}
