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

/* Original [0004b8e0,0004b8fa). */
unsigned char *decode_repne_prefix(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->prefix=DIS_PREFIX_REPNZ;
    return decode_opcode(data,state);
}

/* Original [0004ba50,0004ba70). */
unsigned char *update_al_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_AL];
    else state->registers_read|=register_masks[DIS_REG_AL];
    return data;
}

/* Original [0004bdd0,0004bdf3). */
unsigned char *decode_fs_prefix(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->segment=DIS_REG_FS;
    state->registers_read|=register_masks[DIS_REG_ES];
    return decode_opcode(data,state);
}

/* Original [0004ded0,0004df1d). */
unsigned char *decode_ecx_register(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ECX:DIS_REG_CX;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004de40,0004de79). */
unsigned char *decode_eax_destination(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EAX:DIS_REG_AX;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    state->registers_written|=register_masks[reg];
    return data;
}

/* Original [0004b700,0004b72a). */
unsigned char *decode_shift_group(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,&disassembly_shift_group[(*data&0x38)>>3],state);
}

/* Original [0004d7d0,0004d80b). */
unsigned char *decode_register_and_byte_immediate(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *immediate=&state->operands[2];
    decode_modrm_register(data,operand,state);
    immediate->kind=3;
    immediate->address.value_bytes=1;
    *(unsigned char *)&immediate->address.value=*data;
    return data+1;
}

enum X87RegisterMnemonic {
    X87_FFREE=58, X87_FXCH=116, X87_FST=94, X87_FSTP=95,
    X87_FNENI=57, X87_FNDISI=50, X87_FNCLEX=42, X87_FNINIT=60, X87_FSETPM=90,
    X87_FADDP=38, X87_FMULP=75, X87_FCOMPP=45, X87_FSUBP=105,
    X87_FSUBRP=108, X87_FDIVP=52, X87_FDIVRP=55,
    X87_FADD=37, X87_FMUL=74, X87_FCOM=43, X87_FCOMP=44,
    X87_FSUB=104, X87_FSUBR=107, X87_FDIV=51, X87_FDIVR=54,
    X87_FNSTSW=103, X87_FUCOM=111, X87_FUCOMP=112, X87_FLD=61, X87_FNOP=77,
    X87_FCHS=41, X87_FABS=36, X87_FTST=110, X87_FXAM=115,
    X87_FLD1=63, X87_FLDL2T=67, X87_FLDL2E=66, X87_FLDPI=65,
    X87_FLDLG2=68, X87_FLDLN2=69, X87_FLDZ=64,
    X87_F2XM1=35, X87_FYL2X=118, X87_FPTAN=81, X87_FPATAN=78,
    X87_FXTRACT=117, X87_FDECSTP=49, X87_FINCSTP=59,
    X87_FPREM=79, X87_FYL2XP1=119, X87_FSQRT=93, X87_FSINCOS=92,
    X87_FRNDINT=82, X87_FSCALE=89, X87_FSIN=91, X87_FCOS=48
};

/* Original code [0004d0e0,0004d1c7), internal switch data [4d1c8,4d1e0). */
unsigned char *decode_x87_dd(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long selector=(*data&0x38)>>3;
    state->modrm=data;
    if ((*data&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_dd_memory[selector],state);
    switch (selector) {
    case 0: state->mnemonic=X87_FFREE; break;
    case 1: state->mnemonic=X87_FXCH; break;
    case 2: state->mnemonic=X87_FST; break;
    case 3: state->mnemonic=X87_FSTP; break;
    case 4: state->mnemonic=X87_FUCOM; break;
    case 5: state->mnemonic=X87_FUCOMP; break;
    default: state->mnemonic=0;
    }
    if (!state->mnemonic) return decode_op_4b650(data,operand,state);
    state->operands[0].kind=2;
    /* The original uses display index64+rm, one beyond ST(0). */
    state->operands[0].address.base=64+(*data&7);
    return data+1;
}

/* Original [0004d470,0004d4dd). */
unsigned char *decode_x87_memory(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    unsigned char *next;
    result->kind=1;
    if (state->address32)
        next=decode_address32(state->modrm,NULL,&result->address);
    else next=decode_address16(state->modrm,NULL,&result->address);
    if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
    if (result->address.base) state->registers_read|=register_masks[result->address.base];
    if (result->address.index) state->registers_read|=register_masks[result->address.index];
    return next;
}

/* Original [0004d500,0004d510). */
unsigned char *size_x87_qword(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->operands[0].size=5;
    return data;
}

const unsigned long disassembly_x87_d9_register_mnemonics[4][8]={
    {X87_FCHS,X87_FABS,0,0,X87_FTST,X87_FXAM,0,0},
    {X87_FLD1,X87_FLDL2T,X87_FLDL2E,X87_FLDPI,X87_FLDLG2,X87_FLDLN2,X87_FLDZ,0},
    {X87_F2XM1,X87_FYL2X,X87_FPTAN,X87_FPATAN,X87_FXTRACT,0,X87_FDECSTP,X87_FINCSTP},
    {X87_FPREM,X87_FYL2XP1,X87_FSQRT,X87_FSINCOS,X87_FRNDINT,X87_FSCALE,X87_FSIN,X87_FCOS}
};

/* Original code [0004cc90,0004cdc7), switch data [4cdc8,4cde8). */
unsigned char *decode_x87_d9(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long selector=(*data&0x38)>>3,rm=*data&7;
    state->modrm=data;
    if ((*data&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_d9_memory[selector],state);
    switch (selector) {
    case 0:
    case 1:
    case 3:
        state->mnemonic=selector==0?X87_FLD:selector==1?X87_FXCH:X87_FSTP;
        state->operands[0].kind=2;
        state->operands[0].address.base=64+rm;
        break;
    case 2:
        state->mnemonic=rm?0:X87_FNOP;
        break;
    default:
        state->mnemonic=disassembly_x87_d9_register_mnemonics[selector-4][rm];
    }
    if (!state->mnemonic) return decode_op_4b650(data,operand,state);
    return data+1;
}

/* Original [0004d4f0,0004d500). */
unsigned char *size_x87_dword(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->operands[0].size=3;
    return data;
}

/* Original code [0004cac0,0004cc6b), switch data [4cc6c,4cc8c). */
unsigned char *decode_x87_d8(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long selector=(*data&0x38)>>3,reg=64+(*data&7);
    state->modrm=data;
    if ((*data&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_d8_memory[selector],state);
    switch (selector) {
    case 0: state->mnemonic=X87_FADD; break;
    case 1: state->mnemonic=X87_FMUL; break;
    case 2: state->mnemonic=X87_FCOM; break;
    case 3: state->mnemonic=X87_FCOMP; break;
    case 4: state->mnemonic=X87_FSUB; break;
    case 5: state->mnemonic=X87_FSUBR; break;
    case 6: state->mnemonic=X87_FDIV; break;
    case 7: state->mnemonic=X87_FDIVR; break;
    }
    state->operands[0].kind=2;
    if (selector==2 || selector==3) state->operands[0].address.base=reg;
    else {
        state->operands[0].address.base=63;
        state->operands[1].kind=2;
        state->operands[1].address.base=reg;
    }
    return data+1;
}

/* Original code [0004cf10,0004d0bb), switch data [4d0bc,4d0dc). */
unsigned char *decode_x87_dc(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long selector=(*data&0x38)>>3,reg=64+(*data&7);
    state->modrm=data;
    if ((*data&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_dc_memory[selector],state);
    switch (selector) {
    case 0: state->mnemonic=X87_FADD; break;
    case 1: state->mnemonic=X87_FMUL; break;
    case 2: state->mnemonic=X87_FCOM; break;
    case 3: state->mnemonic=X87_FCOMP; break;
    case 4: state->mnemonic=X87_FSUB; break;
    case 5: state->mnemonic=X87_FSUBR; break;
    case 6: state->mnemonic=X87_FDIV; break;
    case 7: state->mnemonic=X87_FDIVR; break;
    }
    state->operands[0].kind=2;
    state->operands[0].address.base=reg;
    if (selector!=2 && selector!=3) {
        state->operands[1].kind=2;
        state->operands[1].address.base=63;
    }
    return data+1;
}

const unsigned long disassembly_x87_db_register_mnemonics[8]={
    X87_FNENI,X87_FNDISI,X87_FNCLEX,X87_FNINIT,X87_FSETPM,0,0,0
};

/* Original [0004ce80,0004cf05). */
unsigned char *decode_x87_db(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long selector=(*data&0x38)>>3;
    state->modrm=data;
    if ((*data&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_db_memory[selector],state);
    state->mnemonic=selector==4?disassembly_x87_db_register_mnemonics[*data&7]:0;
    if (!state->mnemonic) return decode_op_4b650(data,operand,state);
    return data+1;
}

/* Original code [0004d1e0,0004d304), switch data [4d304,4d324). */
unsigned char *decode_x87_de(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long selector=(*data&0x38)>>3,rm=*data&7;
    state->modrm=data;
    if ((*data&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_de_memory[selector],state);
    switch (selector) {
    case 0: state->mnemonic=X87_FADDP; break;
    case 1: state->mnemonic=X87_FMULP; break;
    case 2: state->mnemonic=X87_FCOMP; break;
    case 3: state->mnemonic=rm==1?X87_FCOMPP:0; break;
    case 4: state->mnemonic=X87_FSUBP; break;
    case 5: state->mnemonic=X87_FSUBRP; break;
    case 6: state->mnemonic=X87_FDIVP; break;
    case 7: state->mnemonic=X87_FDIVRP; break;
    }
    if (selector!=3) {
        state->operands[0].kind=2;
        state->operands[0].address.base=64+rm;
        if (selector!=2) {
            state->operands[1].kind=2;
            state->operands[1].address.base=63;
        }
    }
    if (!state->mnemonic) return decode_op_4b650(data,operand,state);
    return data+1;
}

/* Original [0004d330,0004d46f). */
unsigned char *decode_x87_df(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long selector=(*data&0x38)>>3,rm=*data&7;
    state->modrm=data;
    if ((*data&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_df_memory[selector],state);
    switch (selector) {
    case 0: state->mnemonic=X87_FFREE; break;
    case 1: state->mnemonic=X87_FXCH; break;
    case 2: state->mnemonic=X87_FST; break;
    case 3: state->mnemonic=X87_FSTP; break;
    case 4:
        state->mnemonic=rm?0:X87_FNSTSW;
        if (!rm) {
            state->operands[0].kind=2;
            state->operands[0].address.base=DIS_REG_AX;
            state->registers_written|=register_masks[DIS_REG_AX];
        }
        break;
    default:
        state->mnemonic=0;
    }
    if (selector<4) {
        state->operands[0].kind=2;
        state->operands[0].address.base=64+rm;
    }
    if (!state->mnemonic) return decode_op_4b650(data,operand,state);
    return data+1;
}

/* Original [0004dbb0,0004dbec). Explicit AL operand and its overlap mask. */
unsigned char *decode_op_4dbb0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_AL;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_AL];
    else state->registers_read|=register_masks[DIS_REG_AL];
    return data;
}

/* Original [0004dc30,0004dc6c). Explicit CL operand. */
unsigned char *decode_op_4dc30(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_CL;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_CL];
    else state->registers_read|=register_masks[DIS_REG_CL];
    return data;
}

/* Original [0004dc70,0004dcac). Explicit DL operand. */
unsigned char *decode_op_4dc70(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_DL;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_DL];
    else state->registers_read|=register_masks[DIS_REG_DL];
    return data;
}

/* Original [0004dbf0,0004dc2c). Explicit BL operand. */
unsigned char *decode_op_4dbf0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_BL;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_BL];
    else state->registers_read|=register_masks[DIS_REG_BL];
    return data;
}

/* Original [0004dcb0,0004dcec). Explicit AH operand. */
unsigned char *decode_op_4dcb0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_AH;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_AH];
    else state->registers_read|=register_masks[DIS_REG_AH];
    return data;
}

/* Original [0004dd30,0004dd6c). Explicit CH operand. */
unsigned char *decode_op_4dd30(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_CH;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_CH];
    else state->registers_read|=register_masks[DIS_REG_CH];
    return data;
}

/* Original [0004dd70,0004ddac). Explicit DH operand. */
unsigned char *decode_op_4dd70(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_DH;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_DH];
    else state->registers_read|=register_masks[DIS_REG_DH];
    return data;
}

/* Original [0004dcf0,0004dd2c). Explicit BH operand. */
unsigned char *decode_op_4dcf0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_BH;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_BH];
    else state->registers_read|=register_masks[DIS_REG_BH];
    return data;
}

/* Original [0004d640,0004d687). The ModR/M byte register is written
 * regardless of its display operand position. */
unsigned char *decode_op_4d640(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    unsigned long reg=disassembly_byte_registers[(*modrm&0x38)>>3];
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    state->registers_written|=register_masks[reg];
    return data;
}

extern const OpcodeEntry disassembly_byte_arithmetic_group[8];

/* Original [0004b670,0004b69a). Opcode80's byte arithmetic group. */
unsigned char *decode_op_4b670(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,
        &disassembly_byte_arithmetic_group[(*data&0x38)>>3],state);
}

extern const OpcodeEntry disassembly_byte_shift_group[8];

/* Original [0004b6d0,0004b6fa). Byte shift/rotate dispatch. */
unsigned char *decode_op_4b6d0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,
        &disassembly_byte_shift_group[(*data&0x38)>>3],state);
}

/* Original [0004c140,0004c16b). Shift count of one consumes no bytes. */
unsigned char *decode_op_4c140(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=3;
    result->address.value_bytes=1;
    *(unsigned char *)&result->address.value=1;
    return data;
}

/* Original [0004d810,0004d84a). SHLD/SHRD have a third CL operand. */
unsigned char *decode_op_4d810(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    decode_modrm_register(data,operand,state);
    state->operands[2].kind=2;
    state->operands[2].address.base=DIS_REG_CL;
    state->registers_read|=register_masks[DIS_REG_CL];
    return data;
}

extern const OpcodeEntry disassembly_byte_unary_group[8];

/* Original [0004b730,0004b75a). F6's byte unary group. */
unsigned char *decode_op_4b730(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,
        &disassembly_byte_unary_group[(*data&0x38)>>3],state);
}

extern const OpcodeEntry disassembly_byte_increment_group[8];

/* Original [0004b790,0004b7ba). FE byte increment/decrement group. */
unsigned char *decode_op_4b790(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,
        &disassembly_byte_increment_group[(*data&0x38)>>3],state);
}

/* Original [0004bb10,0004bb48). EAX register effects, without a display operand. */
unsigned char *decode_op_4bb10(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EAX:DIS_REG_AX;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bb90,0004bbc8). ECX register effects, without a display operand. */
unsigned char *decode_op_4bb90(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ECX:DIS_REG_CX;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004df50,0004df9d). Explicit DX/EDX operand. */
unsigned char *decode_op_4df50(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EDX:DIS_REG_DX;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bbd0,0004bc08). EDX register effects, without a display operand. */
unsigned char *decode_op_4bbd0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EDX:DIS_REG_DX;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bb50,0004bb88). EBX register effects, without a display operand. */
unsigned char *decode_op_4bb50(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EBX:DIS_REG_BX;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004dfa0,0004dfed). Explicit SP/ESP operand. */
unsigned char *decode_op_4dfa0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ESP:DIS_REG_SP;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bc10,0004bc48). ESP register effects, without a display operand. */
unsigned char *decode_op_4bc10(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ESP:DIS_REG_SP;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bc50,0004bc88). EBP register effects, without a display operand. */
unsigned char *decode_op_4bc50(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EBP:DIS_REG_BP;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bc90,0004bcc8). ESI register effects, without a display operand. */
unsigned char *decode_op_4bc90(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_ESI:DIS_REG_SI;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bcd0,0004bd08). EDI register effects, without a display operand. */
unsigned char *decode_op_4bcd0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EDI:DIS_REG_DI;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bae0,0004bb0b). PUSHA/POPA touch the full mode-sized set. */
unsigned char *decode_op_4bae0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long mask=state->operand32?0xffffffUL:0xffffUL;
    if (operand==1) state->registers_written|=mask;
    else state->registers_read|=mask;
    return data;
}

/* Original [0004ba90,0004bab9). Preserve both repeated AH-write/AL-read effects. */
unsigned char *decode_op_4ba90(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->registers_written|=register_masks[DIS_REG_AH];
    state->registers_read|=register_masks[DIS_REG_AL];
    state->registers_written|=register_masks[DIS_REG_AH];
    state->registers_read|=register_masks[DIS_REG_AL];
    return data;
}

/* Original [0004c060,0004c07e). AAM/AAD require the decoder's decimal radix. */
unsigned char *decode_op_4c060(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (*data!=10) longjmp(disassembler_jump,1);
    return data+1;
}

/* Original [0004ba10,0004ba48). CBW/CWDE's narrower source effect. */
unsigned char *decode_op_4ba10(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_AX:DIS_REG_AL;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004b960,0004b98a). CWD/CDQ read and write the accumulator. */
unsigned char *decode_op_4b960(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EAX:DIS_REG_AX;
    state->registers_read|=register_masks[reg];
    state->registers_written|=register_masks[reg];
    return data;
}

/* Original [0004ba70,0004ba90). LAHF/SAHF's implicit DI-mask identity. */
unsigned char *decode_op_4ba70(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_DI];
    else state->registers_read|=register_masks[DIS_REG_DI];
    return data;
}

/* Original [0004b9f0,0004ba09). XLAT reads and writes AL. */
unsigned char *decode_op_4b9f0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->registers_read|=register_masks[DIS_REG_AL];
    state->registers_written|=register_masks[DIS_REG_AL];
    return data;
}

/* Original [0004bf70,0004bfd4). XLAT's implicit DS:BX/EBX byte source. */
unsigned char *decode_op_4bf70(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->address32?DIS_REG_EBX:DIS_REG_BX;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=1;
    result->address.segment=DIS_REG_DS;
    if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
    result->size=1;
    state->registers_read|=register_masks[reg];
    result->address.base=reg;
    return data;
}

/* Original [0004da30,0004da6c). Explicit ES segment-register operand. */
unsigned char *decode_op_4da30(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_ES;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_ES];
    else state->registers_read|=register_masks[DIS_REG_ES];
    return data;
}

/* Original [0004da70,0004daac). Explicit CS segment-register operand. */
unsigned char *decode_op_4da70(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_CS;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_CS];
    else state->registers_read|=register_masks[DIS_REG_CS];
    return data;
}

/* Original [0004dab0,0004daec). Explicit SS segment-register operand. */
unsigned char *decode_op_4dab0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_SS;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_SS];
    else state->registers_read|=register_masks[DIS_REG_SS];
    return data;
}

/* Original [0004daf0,0004db2c). Explicit DS segment-register operand. */
unsigned char *decode_op_4daf0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_DS;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_DS];
    else state->registers_read|=register_masks[DIS_REG_DS];
    return data;
}

/* Original [0004db30,0004db6c). Explicit FS segment-register operand. */
unsigned char *decode_op_4db30(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_FS;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_FS];
    else state->registers_read|=register_masks[DIS_REG_FS];
    return data;
}

/* Original [0004db70,0004dbac). Explicit GS segment-register operand. */
unsigned char *decode_op_4db70(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_GS;
    if (operand==1) state->registers_written|=register_masks[DIS_REG_GS];
    else state->registers_read|=register_masks[DIS_REG_GS];
    return data;
}

/* Original [0004c830,0004c90a). Fixed16-bit ModR/M operand. */
unsigned char *decode_op_4c830(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    unsigned char *next;
    state->modrm=data;
    if ((*data&0xc0)==0xc0) result->kind=2;
    else {
        result->kind=1;
        result->size=2;
    }
    if (state->address32)
        next=decode_address32(data,disassembly_word_registers,&result->address);
    else next=decode_address16(data,disassembly_word_registers,&result->address);
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

/* ModR/M segment selector identities, original00081d40. */
const unsigned long disassembly_segment_registers[8]={
    DIS_REG_ES,DIS_REG_CS,DIS_REG_SS,DIS_REG_DS,DIS_REG_FS,DIS_REG_GS,0,0
};

/* Original [0004d5e0,0004d63b). Explicit segment-register operand. */
unsigned char *decode_op_4d5e0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    unsigned long reg=disassembly_segment_registers[(*modrm&0x38)>>3];
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004bd10,0004bd33). ES override; original prefixes all use ES's mask. */
unsigned char *decode_op_4bd10(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->segment=DIS_REG_ES;
    state->registers_read|=register_masks[DIS_REG_ES];
    return decode_opcode(data,state);
}

/* Original [0004bd40,0004bd63). CS override; original prefixes all use ES's mask. */
unsigned char *decode_op_4bd40(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->segment=DIS_REG_CS;
    state->registers_read|=register_masks[DIS_REG_ES];
    return decode_opcode(data,state);
}

/* Original [0004bd70,0004bd93). SS override; original prefixes all use ES's mask. */
unsigned char *decode_op_4bd70(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->segment=DIS_REG_SS;
    state->registers_read|=register_masks[DIS_REG_ES];
    return decode_opcode(data,state);
}

/* Original [0004be00,0004be23). GS override; original prefixes all use ES's mask. */
unsigned char *decode_op_4be00(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->segment=DIS_REG_GS;
    state->registers_read|=register_masks[DIS_REG_ES];
    return decode_opcode(data,state);
}

/* Original [0004c4c0,0004c52c). Fixed16-bit register-only ModR/M.rm. */
unsigned char *decode_op_4c4c0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg;
    DisassemblyOperand *result=&state->operands[operand-1];
    if ((*data&0xc0)!=0xc0) longjmp(disassembler_jump,1);
    reg=disassembly_word_registers[*data&7];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    state->modrm=data;
    return data+1;
}

/* Original [0004c970,0004c9ba). BOUND's memory pair has its own size code. */
unsigned char *decode_op_4c970(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    data=decode_modrm_operand(data,operand,state);
    if (state->operands[operand-1].kind!=1) longjmp(disassembler_jump,1);
    state->operands[operand-1].size=5;
    return data;
}

/* Original [0004d8a0,0004d8ca). LES includes the segment destination effect. */
unsigned char *decode_op_4d8a0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_ES];
    else state->registers_read|=register_masks[DIS_REG_ES];
    return decode_modrm_register(data,operand,state);
}

/* Original [0004ca60,0004cabe). Segment:offset pointer in memory. */
unsigned char *decode_op_4ca60(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    data=decode_modrm_operand(data,operand,state);
    if (state->operands[operand-1].kind!=1) longjmp(disassembler_jump,1);
    state->operands[operand-1].size=state->operand32?8:7;
    return data;
}

/* Original [0004d900,0004d92a). Segment DS effect plus general register destination. */
unsigned char *decode_op_4d900(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_DS];
    else state->registers_read|=register_masks[DIS_REG_DS];
    return decode_modrm_register(data,operand,state);
}

/* Original [0004d8d0,0004d8fa). Segment SS effect plus general register destination. */
unsigned char *decode_op_4d8d0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_SS];
    else state->registers_read|=register_masks[DIS_REG_SS];
    return decode_modrm_register(data,operand,state);
}

/* Original [0004d930,0004d95a). Segment FS effect plus general register destination. */
unsigned char *decode_op_4d930(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_FS];
    else state->registers_read|=register_masks[DIS_REG_FS];
    return decode_modrm_register(data,operand,state);
}

/* Original [0004d960,0004d98a). Segment GS effect plus general register destination. */
unsigned char *decode_op_4d960(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_GS];
    else state->registers_read|=register_masks[DIS_REG_GS];
    return decode_modrm_register(data,operand,state);
}

extern const OpcodeEntry disassembly_task_group[8];

/* Original [0004b7f0,0004b81a). 0F00 descriptor/task selectors. */
unsigned char *decode_op_4b7f0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,&disassembly_task_group[(*data&0x38)>>3],state);
}

extern const OpcodeEntry disassembly_system_group[8];

/* Original [0004b820,0004b84a). 0F01 system-table/status selectors. */
unsigned char *decode_op_4b820(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,&disassembly_system_group[(*data&0x38)>>3],state);
}

/* Original [0004c9c0,0004ca0a). Six-byte descriptor-table memory operand. */
unsigned char *decode_descriptor_memory(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    data=decode_modrm_operand(data,operand,state);
    if (state->operands[operand-1].kind!=1) longjmp(disassembler_jump,1);
    state->operands[operand-1].size=4;
    return data;
}

/* CR0, CR2, CR3; other selector slots have the original zero identity. */
const unsigned long disassembly_control_registers[8]={32,0,34,35,0,0,0,0};

/* Original [0004d520,0004d55d). Control registers carry no ordinary GPR mask. */
unsigned char *decode_op_4d520(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=disassembly_control_registers[(*modrm&0x38)>>3];
    return data;
}

/* Original [0004c530,0004c59c). Fixed32-bit register-only ModR/M.rm. */
unsigned char *decode_op_4c530(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg;
    DisassemblyOperand *result=&state->operands[operand-1];
    if ((*data&0xc0)!=0xc0) longjmp(disassembler_jump,1);
    reg=disassembly_dword_registers[*data&7];
    result->kind=2;
    result->address.base=reg;
    if (operand==1) state->registers_written|=register_masks[reg];
    else state->registers_read|=register_masks[reg];
    state->modrm=data;
    return data+1;
}

/* DR0..3 and DR6..7 identities; reserved DR4/DR5 remain zero. */
const unsigned long disassembly_debug_registers[8]={40,41,42,43,0,0,46,47};

/* Original [0004d560,0004d59d). Debug-register selector. */
unsigned char *decode_op_4d560(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=disassembly_debug_registers[(*modrm&0x38)>>3];
    return data;
}

/* TR3..7 identities in the original table; this386 corpus uses TR6/TR7. */
const unsigned long disassembly_test_registers[8]={0,0,0,51,52,53,54,55};

/* Original [0004d5a0,0004d5dd). Test-register selector. */
unsigned char *decode_op_4d5a0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=disassembly_test_registers[(*modrm&0x38)>>3];
    return data;
}

/* Original [0004b9c0,0004b9e1). Far return/interrupt writes CS and updates SP. */
unsigned char *decode_op_4b9c0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->registers_written|=register_masks[DIS_REG_CS];
    return update_stack_registers(data,operand,state);
}

/* Original [0004df20,0004df4b). DX is read for port I/O at either position. */
unsigned char *decode_op_4df20(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=DIS_REG_DX;
    state->registers_read|=register_masks[DIS_REG_DX];
    return data;
}

/* Original [0004de00,0004de39). OUT's accumulator is always read. */
unsigned char *decode_op_4de00(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->operand32?DIS_REG_EAX:DIS_REG_AX;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    state->registers_read|=register_masks[reg];
    return data;
}

/* Original [0004be90,0004bef9). Word/dword string destination ES:DI/EDI. */
unsigned char *decode_op_4be90(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->address32?DIS_REG_EDI:DIS_REG_DI;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=1;
    result->address.segment=DIS_REG_ES;
    state->registers_read|=register_masks[DIS_REG_ES];
    result->size=state->operand32?3:2;
    state->registers_read|=register_masks[reg];
    result->address.base=reg;
    return data;
}

/* Original [0004bfe0,0004c054). Word/dword string source DS:SI/ESI. */
unsigned char *decode_op_4bfe0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned long reg=state->address32?DIS_REG_ESI:DIS_REG_SI;
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=1;
    result->address.segment=DIS_REG_DS;
    if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
    result->size=state->operand32?3:2;
    state->registers_read|=register_masks[reg];
    result->address.base=reg;
    return data;
}

extern const OpcodeEntry disassembly_indirect_group[8];

/* Original [0004b7c0,0004b7ea). FF's indirect-control-transfer group. */
unsigned char *decode_op_4b7c0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,&disassembly_indirect_group[(*data&0x38)>>3],state);
}

/* Original [0004bac0,0004bae0). Far jump's implicit CS effect. */
unsigned char *decode_op_4bac0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_CS];
    else state->registers_read|=register_masks[DIS_REG_CS];
    return data;
}

/* Original [0004c0b0,0004c0e0). Fixed16-bit immediate; upper bytes untouched. */
unsigned char *decode_op_4c0b0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=3;
    result->address.value_bytes=2;
    *(unsigned short *)&result->address.value=*(unsigned short *)data;
    return data+2;
}

/* Original [0004d4e0,0004d4f0). The size callback writes operand1,
 * although apply_opcode invokes it at callback position2. */
unsigned char *decode_op_4d4e0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->operands[0].size=2;
    return data;
}

/* Original [0004d510,0004d520). Extended precision/BCD memory size. */
unsigned char *decode_op_4d510(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    state->operands[0].size=6;
    return data;
}

extern const OpcodeEntry disassembly_x87_da_memory[8];

enum { X87_FUCOMPP=113 };

/* Original [0004cdf0,0004ce7f). DA integer arithmetic or FUCOMPP. */
unsigned char *decode_op_4cdf0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char modrm=*data;
    state->modrm=data;
    if ((modrm&0xc0)!=0xc0)
        return apply_opcode(data,&disassembly_x87_da_memory[(modrm&0x38)>>3],state);
    state->mnemonic=((modrm&0x38)==0x28 && (modrm&7)==1)?X87_FUCOMPP:0;
    if (!state->mnemonic) return decode_op_4b650(data,operand,state);
    return data+1;
}

extern const OpcodeEntry disassembly_unary_group[8];

/* Original [0004b760,0004b78a). F7's word/dword unary group. */
unsigned char *decode_op_4b760(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,&disassembly_unary_group[(*data&0x38)>>3],state);
}

/* Original [0004c3d0,0004c43a). Byte moffs uses address-size, independent of data-size. */
unsigned char *decode_op_4c3d0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=1;
    result->size=1;
    result->address.segment=DIS_REG_DS;
    if (!state->segment) state->registers_read|=register_masks[DIS_REG_DS];
    if (state->address32) {
        result->address.value_bytes=4;
        result->address.value=*(long *)data;
    } else {
        result->address.value_bytes=2;
        *(unsigned short *)&result->address.value=*(unsigned short *)data;
    }
    return data+result->address.value_bytes;
}

/* Original [0004d690,0004d6f0). XCHG's ModR/M.reg is always written. */
unsigned char *decode_op_4d690(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *modrm=state->modrm?state->modrm:data;
    unsigned long index=(*modrm&0x38)>>3;
    unsigned long reg=state->operand32?disassembly_dword_registers[index]:
        disassembly_word_registers[index];
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=2;
    result->address.base=reg;
    state->registers_written|=register_masks[reg];
    return data;
}

/* Original [0004c6d0,0004c70b). BT additionally reads a register-form operand. */
unsigned char *decode_op_4c6d0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    data=decode_modrm_operand(data,operand,state);
    if (result->kind==2) state->registers_read|=register_masks[result->address.base];
    return data;
}

extern const OpcodeEntry disassembly_immediate_bit_group[8];

/* Original [0004b850,0004b87a). 0FBA immediate bit-test group. */
unsigned char *decode_op_4b850(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    return apply_opcode(data,&disassembly_immediate_bit_group[(*data&0x38)>>3],state);
}

/* Original [0004c910,0004c934). IMUL's third, mode-sized immediate. */
unsigned char *decode_op_4c910(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    data=decode_modrm_operand_effects(data,operand,state);
    return decode_immediate(data,3,state);
}

/* Original [0004c940,0004c964). IMUL's third, byte-sized immediate. */
unsigned char *decode_op_4c940(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    data=decode_modrm_operand_effects(data,operand,state);
    return decode_byte_immediate(data,3,state);
}

/* Original [0004d990,0004d9c4). ENTER updates stack/frame then reads its WORD. */
unsigned char *decode_op_4d990(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    update_stack_registers(data,operand,state);
    update_frame_registers(data,operand,state);
    return decode_op_4c0b0(data,operand,state);
}

/* Original [0004c170,0004c19b). INT3's implicit immediate consumes no bytes. */
unsigned char *decode_op_4c170(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=3;
    result->address.value_bytes=1;
    *(unsigned char *)&result->address.value=3;
    return data;
}

/* Original [0004c1a0,0004c244). Immediate far selector:offset operand. */
unsigned char *decode_op_4c1a0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    DisassemblyOperand *result=&state->operands[operand-1];
    result->kind=5;
    if (state->operand32) {
        result->address.value_bytes=4;
        result->address.value=*(long *)data;
    } else {
        result->address.value_bytes=2;
        *(unsigned short *)&result->address.value=*(unsigned short *)data;
    }
    data+=result->address.value_bytes;
    *(unsigned short *)&result->address.selector=*(unsigned short *)data;
    if (state->operand32) state->target_address=result->address.value;
    else state->target_address=(unsigned short)result->address.value+
        ((unsigned long)(unsigned short)result->address.selector<<16);
    if (operand==1) state->registers_written|=register_masks[DIS_REG_CS];
    else state->registers_read|=register_masks[DIS_REG_CS];
    return data+2;
}

/* Original [0004c710,0004c74b): XADD's second memory/register decode. */
unsigned char *decode_op_4c710(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    unsigned char *next=decode_modrm_operand(data,operand,state);
    DisassemblyOperand *result=&state->operands[operand-1];
    if (result->kind==2)
        state->registers_written|=register_masks[result->address.base];
    return next;
}

/* Original [0004d9d0,0004d9fa): byte CMPXCHG accumulator effect. */
unsigned char *decode_op_4d9d0(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    if (operand==1) state->registers_written|=register_masks[DIS_REG_AL];
    else state->registers_read|=register_masks[DIS_REG_AL];
    return decode_modrm_operand_effects(data,operand,state);
}

/* Original [0004da00,0004da29): word/dword CMPXCHG's accumulator effect. */
unsigned char *decode_op_4da00(unsigned char *data,int operand,
    DisassembledInstruction *state)
{
    decode_op_4bb10(data,operand,state);
    return decode_modrm_operand_effects(data,operand,state);
}
