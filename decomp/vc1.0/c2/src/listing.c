/* Assembly listing output. */
#include <ctype.h>
#include <string.h>
#include "listing.h"
#include "options.h"
#include "output.h"
#include "function-input.h"
#include "records.h"
#include "instruction-encoding.h"
#include "instruction-rewrite.h"
#include "machine-layout.h"
#include <recon.h>
extern int dos_object;
extern Segment *segment_list;
ListingStorage listing_storage; /* 0007c954..0007caa4; see typed cache alias view */
int listing_source_count, listing_source_index; /* 0007e1c8,0007e1cc */
char *assembly_source_filename; /* 0007e1c0 */
#define source_line_cursor (listing_storage.state.cursor) /* 0007ca94 */
#define source_lines (listing_storage.state.lines) /* 0007ca98 */
static long source_checkpoint_position, source_checkpoint_line; /* 0007e1d0,0007e1d4 */
static long source_current_line; /* 0007e1c4 */
static FILE *listing_source_file; /* 0007e1d8 */
unsigned listing_column; /* 00080d40 */
extern char *generated_source;
/* Original28680..2875f. Collect line boundaries before printing the machine
   stream; a changed file records the preceding line as its initial boundary. */
void __fastcall prepare_source_listing(MachineInstruction *instruction)
{
    char *filename;
    long last_line;
    int i;
    source_line_cursor = source_lines = NULL;
    source_checkpoint_position = source_checkpoint_line = 0;
    for (i = 0; i < listing_source_count; ++i) listing_sources[i].lines = NULL;
    filename = assembly_source_filename;
    last_line = -2;
    current_source_location = (unsigned)-2;
    for (; instruction; instruction = instruction->next) {
        switch (instruction->opcode & 15) {
        case 0:
            if (current_source_location != (unsigned)last_line) {
                last_line = current_source_location;
                record_listing_source_line(last_line);
            }
            break;
        case 4:
            if (instruction->record.source_location) {
                current_source_location = instruction->record.source_location;
                if (last_line == -1) {
                    last_line = -2;
                    record_listing_source_line(current_source_location - 1);
                }
            }
            break;
        case 5:
            if (filename != instruction->record.filename) {
                filename = instruction->record.filename;
                open_listing_source(filename);
                generated_source = filename;
            }
            current_source_location = (unsigned)-1;
            last_line = -1;
            break;
        }
    }
}
/* Original27fc0..28114. Each file retains its collected line boundaries and
   current read line. The overflow diagnostic follows all new-row writes. */
void __fastcall open_listing_source(char *filename)
{
    int index;
    unsigned char *left, *right;
    listing_sources[listing_source_index].lines = source_lines;
    listing_sources[listing_source_index].line = source_current_line;
    for (index = 0; index < listing_source_count; ++index) {
        left = (unsigned char *)listing_sources[index].name;
        right = (unsigned char *)filename;
        while (*left == *right && *left) { ++left; ++right; }
        if (*left == *right) break;
    }
    listing_source_index = index;
    if (index == listing_source_count) {
        listing_sources[listing_source_count].name = filename;
        listing_sources[listing_source_count].file = fopen(filename, "rb");
        index = listing_source_index;
        if (!listing_sources[listing_source_count].file) {
            report_context_error(0x53, "source", listing_sources[listing_source_count].name);
            index = listing_source_index;
        }
        listing_sources[listing_source_count].lines = NULL;
        listing_sources[listing_source_count].line = 0;
        ++listing_source_count;
        source_checkpoint_position = source_checkpoint_line = 0;
        if (listing_source_count > 20) {
            compiler_internal_error("@(#)listing.c:1.76", 0x5ed);
            index = listing_source_index;
        }
    }
    listing_source_index = index;
    listing_source_file = listing_sources[index].file;
    source_line_cursor = source_lines = listing_sources[index].lines;
    source_current_line = listing_sources[index].line;
}
/* Original2a880..2a916. Keep unique signed line boundaries in order, retaining
   a cursor so the usual increasing-line stream does not restart at the head. */
void __fastcall record_listing_source_line(long line)
{
    ListingSourceLine *cursor, *entry;
    if (!source_lines || source_lines->line > line) {
        entry = arena_alloc(1, sizeof(ListingSourceLine));
        entry->line = line;
        entry->next = source_lines;
        source_line_cursor = source_lines = entry;
    } else {
        cursor = source_line_cursor;
        if (cursor->line > line) cursor = source_lines;
        while (cursor->next && cursor->next->line <= line) cursor = cursor->next;
        source_line_cursor = cursor;
        if (cursor->line != line) {
            entry = arena_alloc(1, sizeof(ListingSourceLine));
            entry->line = line;
            entry->next = source_line_cursor->next;
            source_line_cursor->next = entry;
            source_line_cursor = entry;
        }
    }
}
/* Original282d0..28335. Stack/register local symbols are listed once; other
   eligible local classes are marked handled without an assignment line. */
void __fastcall write_local_listing(SymbolHeader *symbol)
{
    BackendSymbol *entry = (BackendSymbol *)symbol;
    unsigned storage;
    if ((entry->output_flags & 4) && !(entry->output_flags & 2)) {
        storage = entry->attributes & 0x70;
        if (symbol->kind == 1 && (storage == 0x10 || storage == 0x30)) {
            printf("%s = %ld", symbol_display_name(symbol), (long)symbol->field04);
            listing_column = 0;
            printf("\n");
        }
        entry->output_flags |= 2;
    }
}
/* Original28230..2827c. Numeric local labels and named symbols share the
   listing column counter; NULL is used by implicit operand forms. */
void __fastcall write_listing_symbol(SymbolHeader *symbol)
{
    if (!symbol) return;
    if (symbol->kind == 0x13)
        listing_column += printf("%ld+", (long)symbol->field04);
    else
        listing_column += printf("%s", symbol_display_name(symbol));
}
/* Original28440..28507. Long physical source lines are emitted as repeated
   chunks, with the numbered prefix only on the first chunk. */
void __fastcall write_listing_source(unsigned line)
{
    char buffer[256];
    long current;
    int more;
    current = previous_listing_source_line(line) + 1;
    listing_column = 0;
    printf("\n");
    for (; current <= (long)line; ++current) {
        more = read_listing_source(buffer, current);
        if (!*buffer) return;
        listing_column += printf("; %-5lu: %s", (unsigned long)current, buffer);
        while (more && *buffer) {
            more = read_listing_source(buffer, current);
            listing_column += printf("%s", buffer);
        }
    }
    listing_column = 0;
    printf("\n");
}
/* Original2a920..2a957. The requested line is a recorded boundary; the
   predecessor determines how many intervening source lines to print. */
long __fastcall previous_listing_source_line(long line)
{
    ListingSourceLine *cursor;
    if (source_lines->line == line) return line - 1;
    cursor = source_line_cursor;
    if (cursor->line >= line) cursor = source_lines;
    while (cursor->next->line < line) cursor = cursor->next;
    source_line_cursor = cursor;
    return cursor->line;
}
/* Original2a960..2aa75. A checkpoint supports backwards line requests.
   A255-byte piece without newline returns1 only for the requested line;
   pieces of skipped long lines do not advance the logical line counter. */
int __fastcall read_listing_source(char *buffer, long line)
{
    unsigned length;
    char *carriage;
    if (source_checkpoint_line >= line) {
        source_checkpoint_line = 0;
        source_checkpoint_position = 0;
    }
    if (source_current_line >= line) {
        fseek(listing_source_file, source_checkpoint_position, SEEK_SET);
        source_current_line = source_checkpoint_line;
    }
    while (source_current_line != line) {
        if (!fgets(buffer, 256, listing_source_file)) {
            *buffer = 0;
            return 0;
        }
        if (!strchr(buffer, '\n')) {
            for (length = 0; buffer[length]; ++length) {}
            if (length != 255) {
                buffer[length] = '\n';
                buffer[length + 1] = 0;
                return 0;
            }
            if (line - source_current_line == 1) return 1;
        } else {
            ++source_current_line;
        }
    }
    if (!source_checkpoint_line) {
        source_checkpoint_position = ftell(listing_source_file);
        source_checkpoint_line = line;
    }
    carriage = strchr(buffer, '\r');
    if (carriage) {
        *carriage = '\n';
        carriage[1] = 0;
    }
    return 0;
}
/* Original28370..28434. A REP-family prefix and its following operation share
   one listing row, encoded consecutively into the same twenty-byte scratch. */
MachineInstruction *__fastcall write_instruction_listing(MachineInstruction *instruction, unsigned *offset)
{
    unsigned char bytes[20];
    unsigned count;
    MachineInstruction *last = instruction;
    if (machine_selection_properties[(instruction->opcode >> 8) & 255] & MC_REPEAT_PREFIX)
        last = next_machine_operation(instruction);
    if (assembly_code) {
        count = encode_machine_instruction(instruction, bytes, *offset, 0);
        if (last != instruction)
            count += encode_machine_instruction(last, bytes + count, *offset + count, 0);
        write_listing_bytes(*offset, count, bytes);
        pad_listing_column(24);
        *offset += count;
    } else {
        pad_listing_column(8);
    }
    write_instruction_text(instruction);
    write_instruction_comment(instruction);
    if (last != instruction) write_instruction_text(last);
    listing_column = 0;
    printf("\n");
    return last->next;
}
/* Register spellings consumed by35870. The segment entries deliberately
   share storage between the16/32-bit views, as in the original tables. */
static char register_es[] = "es", register_cs[] = "cs";
static char register_ss[] = "ss", register_ds[] = "ds";
static char register_fs[] = "fs", register_gs[] = "gs";
char *listing_registers32[14] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    register_es, register_cs, register_ss, register_ds, register_fs, register_gs
};
char *listing_registers16[14] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    register_es, register_cs, register_ss, register_ds, register_fs, register_gs
};
char *listing_registers8[8] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
char *listing_system_registers[24] = {
    "cr0", NULL, "cr2", "cr3", NULL, NULL, NULL, NULL,
    "dr0", "dr1", "dr2", "dr3", NULL, NULL, "dr6", "dr7",
    NULL, NULL, NULL, NULL, NULL, NULL, "tr6", "tr7"
};
/* Original35870..358a8. Callers supply register numbers in the selected
   class (8 byte,14 word/dword,24 system entries); there was no range guard.
   Zero table entries for nonexistent control/debug registers remain NULL. */
char *__fastcall machine_register_name(unsigned reg, unsigned bytes)
{
    if (!bytes) return listing_system_registers[reg];
    if (bytes == 1) return listing_registers8[reg];
    if (bytes == 4 || bytes == 6) return listing_registers32[reg];
    return listing_registers16[reg];
}

/* Original2a700..2a7bc. Memory operands carry a size and optional segment;
   a one-byte symbolic branch has the explicit SHORT qualifier. */
void __fastcall write_operand_prefix(MachineInstruction *instruction,
    MachineOperand *operand, unsigned bytes)
{
    unsigned kind = operand->flags & 15;
    unsigned operation;
    if ((kind >= 3 && kind <= 5) || kind == 12) {
        listing_column += printf("%s PTR ", listing_data_type(bytes));
        if (operand->flags & 0xf0)
            listing_column += printf("%s:", machine_register_name(
                (operand->flags >> 4) & 15, 2));
    } else if (kind == OPERAND_SYMBOL) {
        operation = (instruction->opcode >> 8) & 255;
        if ((operation == MC_JCC || operation == MC_JMP) && bytes == 1)
            listing_column += printf("SHORT ");
    }
}

/* Listing-only names for additional inline-assembly operand forms. */
enum ListingOperandKind {
    LISTING_SEGMENT_ADDRESS = 7, LISTING_RELATIVE_OFFSET = 10,
    LISTING_SEGMENT_REGISTER = 11, LISTING_MEMORY_DISPLACEMENT = 12,
    LISTING_NO_BASE_REGISTER = 27
};

/* Original29d00..2a10f; twelve-way kind table2a110..2a140. The prefix
   writer owns size/segment qualifiers; this function owns the operand. */
void __fastcall write_instruction_operand(MachineInstruction *instruction,
    MachineOperand *operand, unsigned bytes)
{
    MachineAddress *address;
    unsigned operation, hint, reg, scale;
    write_operand_prefix(instruction, operand, bytes);
    hint = (instruction->record.source_location >> 13) & 7;
    switch (operand->flags & 15) {
    case OPERAND_REGISTER:
        operation = (instruction->opcode >> 8) & 255;
        if (((operation >= MC_FADD && operation <= MC_FLD1) ||
             (operation >= MC_FUCOM && operation <= MC_FCOS)) &&
            operation != MC_FSTSW && operation != MC_FNSTSW) {
            listing_column += printf("ST(%d)",
                (int)operand->value.reg.words.number - REG_ST0);
        } else {
            listing_column += printf("%s", machine_register_name(
                operand->value.reg.words.number, bytes));
        }
        break;
    case OPERAND_IMMEDIATE:
        listing_column += printf("%ld", (long)operand->value.constant);
        break;
    case OPERAND_ABSOLUTE:
        address = operand->value.address;
        if (address->symbol) {
            write_listing_symbol(address->symbol);
            write_listing_displacement(address->first.displacement, hint);
        } else if (address->first.displacement) {
            listing_column += printf("$+%ld", address->first.displacement);
        } else listing_column += printf("$");
        break;
    case OPERAND_BASED:
        address = operand->value.address;
        write_listing_symbol(address->symbol);
        listing_column += printf("[%s", machine_register_name(
            address->base.words.number, 4));
        write_listing_displacement(address->first.displacement, hint);
        listing_column += printf("]");
        break;
    case OPERAND_INDEXED:
        address = operand->value.address;
        write_listing_symbol(address->symbol);
        listing_column += printf("[");
        reg = address->base.words.number;
        if (reg != LISTING_NO_BASE_REGISTER)
            listing_column += printf("%s+", machine_register_name(reg, 4));
        listing_column += printf("%s", machine_register_name(
            address->index.words.number, 4));
        scale = (operand->flags >> 8) & 3;
        if (scale) listing_column += printf("*%d", 1U << scale);
        write_listing_displacement(address->first.displacement, hint);
        listing_column += printf("]");
        break;
    case OPERAND_LABEL_ADDRESS:
        listing_column += printf("OFFSET ");
        if (dos_object) listing_column += printf("FLAT:");
        address = operand->value.address;
        if (address->symbol) write_listing_symbol(address->symbol);
        else listing_column += printf("$");
        write_listing_displacement(address->first.displacement, hint);
        break;
    case LISTING_SEGMENT_ADDRESS:
        listing_column += printf("SEG ");
        address = operand->value.address;
        write_listing_symbol(address->symbol);
        write_listing_displacement(address->first.displacement, hint);
        break;
    case OPERAND_SYMBOL:
        write_listing_symbol(operand->value.symbol);
        break;
    case LISTING_RELATIVE_OFFSET:
        if (operand->value.constant)
            listing_column += printf("$+%d", (int)operand->value.constant);
        else listing_column += printf("$");
        break;
    case LISTING_SEGMENT_REGISTER:
        listing_column += printf("%s", machine_register_name(
            operand->value.reg.words.number, 0));
        break;
    case LISTING_MEMORY_DISPLACEMENT:
        address = operand->value.address;
        if (address->symbol) {
            write_listing_symbol(address->symbol);
            write_listing_displacement(address->first.displacement, hint);
        } else listing_column += printf("%ld", address->first.displacement);
        break;
    }
}

/* Original811c0 has one pointer per MachineOpcode. No pointer identity is
   exposed: the writer passes each selected spelling to printf("%s"). */
const char *listing_mnemonics[MC_OPCODE_COUNT] = {
#define MN(operation, spelling) spelling,
#include "listing-mnemonics.def"
#undef MN
};
const char *listing_condition_names[16] = {
    "o", "no", "b", "ae", "e", "ne", "be", "a",
    "s", "ns", "p", "np", "l", "ge", "le", "g"
};

/* Original2a140..2a36a; switch data2a36c..2a3d8. Type suffixes and
   pseudo-operation spellings retain the original listing dialect. */
void __fastcall write_instruction_mnemonic(MachineInstruction *instruction)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    unsigned bytes = (instruction->opcode >> 4) & 15;
    unsigned column;
    const char *name = listing_mnemonics[operation], *suffix = NULL;
    switch (operation) {
    case MC_RET:
        if (bytes != 4) suffix = "f";
        break;
    case MC_JCC: case MC_SCC:
        name = operation == MC_JCC ? "j" : "set";
        suffix = listing_condition_names[instruction->operands[1].flags & 15];
        break;
    case MC_JCXZ:
        if (bytes == 4) name = "jecxz";
        break;
    case MC_INC:
        if ((instruction->record.source_location & 0xe000) &&
            (instruction->operands[0].flags & 15) == OPERAND_REGISTER)
            name = "dpad";
        break;
    case MC_CMPS: case MC_INS: case MC_LODS: case MC_MOVS:
    case MC_OUTS: case MC_SCAS: case MC_STOS:
        suffix = listing_size_suffix(bytes);
        break;
    case MC_POPA: case MC_POPF: case MC_PUSHA: case MC_PUSHF:
        if (bytes == 4) suffix = listing_size_suffix(4);
        break;
    case MC_IMUL3: case MC_IMUL2:
        name = listing_mnemonics[MC_IMUL];
        break;
    case MC_NOP:
        name = "npad";
        break;
    case MC_LOCK:
        suffix = "\\";
        break;
    case MC_DATA:
        name = listing_data_directive(bytes);
        break;
    case MC_MOVZXW:
        name = listing_mnemonics[MC_MOVZX];
        break;
    case MC_MOVSXW:
        name = listing_mnemonics[MC_MOVSX];
        break;
    case MC_MOVSR:
        name = listing_mnemonics[MC_MOV];
        break;
    }
    column = listing_column;
    listing_column += printf("%s", name);
    if (suffix) listing_column += printf("%s", suffix);
    if (machine_operand_counts[operation] > 0 ||
        (machine_selection_properties[operation] & MC_REPEAT_PREFIX) ||
        operation == MC_NOP)
        write_listing_spaces((int)(column - listing_column + 8));
}

/* Original2a3e0..2a6f6. These compiler forms have implicit operands or
   carry an extra immediate/register inside their address record. */
int __fastcall write_special_instruction(MachineInstruction *instruction)
{
    unsigned operation = (instruction->opcode >> 8) & 255;
    unsigned bytes = (instruction->opcode >> 4) & 15;
    MachineOperand *operand;
    MachineAddress *address;
    switch (operation) {
    case MC_INC:
        if (!(instruction->record.source_location & 0xe000) ||
            (instruction->operands[0].flags & 15) != OPERAND_REGISTER)
            return 0;
        write_instruction_mnemonic(instruction);
        listing_column += printf("1, %s", machine_register_name(
            instruction->operands[0].value.reg.words.number, 4));
        return 1;
    case MC_FIST: case MC_FISTP: case MC_FST: case MC_FSTP:
        write_instruction_mnemonic(instruction);
        write_instruction_operand(instruction, &instruction->operands[0], bytes);
        return 1;
    case MC_FCOM: case MC_FCOMP: case MC_FLD:
        write_instruction_mnemonic(instruction);
        write_instruction_operand(instruction, &instruction->operands[1], bytes);
        return 1;
    case MC_IMUL3:
        write_instruction_mnemonic(instruction);
        address = instruction->operands[1].value.address;
        listing_column += printf("%s, ", machine_register_name(
            address->base.words.number, bytes));
        write_instruction_operand(instruction, &instruction->operands[0], bytes);
        if (!address->symbol) {
            listing_column += printf(", %ld", address->first.displacement);
        } else {
            operand = &instruction->operands[1];
            operand->flags = (operand->flags & ~9U) | OPERAND_LABEL_ADDRESS;
            listing_column += printf(", ");
            write_instruction_operand(instruction, operand, bytes);
            operand->flags = (operand->flags & ~11U) | OPERAND_BASED;
        }
        return 1;
    case MC_NOP:
        write_instruction_mnemonic(instruction);
        listing_column += printf("%d", bytes);
        return 1;
    case MC_SHLD: case MC_SHRD:
        write_instruction_mnemonic(instruction);
        write_instruction_operand(instruction, &instruction->operands[0], bytes);
        operand = &instruction->operands[1];
        if ((operand->flags & 15) == OPERAND_REGISTER) {
            listing_column += printf(", %s, cl", machine_register_name(
                operand->value.reg.words.number, bytes));
        } else {
            address = operand->value.address;
            listing_column += printf(", %s, %d", machine_register_name(
                address->base.words.number, bytes), address->first.displacement);
        }
        return 1;
    default:
        /* Original property bit0 classifies x87 operations. The selection
           table uses its own normalized bits; implicit registers is a
           different property and must not substitute for this test. */
        if (!((operation >= MC_FADD && operation <= MC_FLD1) ||
              (operation >= MC_FUCOM && operation <= MC_FCOS)) ||
            machine_operand_counts[operation] != 2 ||
            (instruction->operands[1].flags & 15) == OPERAND_REGISTER)
            return 0;
        write_instruction_mnemonic(instruction);
        write_instruction_operand(instruction, &instruction->operands[1], bytes);
        return 1;
    }
}

/* Original29b30..29ba5. Special forms own all their text; ordinary forms
   print the mnemonic followed by the declared machine operands. */
void __fastcall write_instruction_text(MachineInstruction *instruction)
{
    int count;
    if (write_special_instruction(instruction)) return;
    write_instruction_mnemonic(instruction);
    count = machine_operand_counts[(unsigned char)(instruction->opcode >> 8)];
    if (count > 0) {
        write_instruction_operand(instruction, &instruction->operands[0],
            machine_operand_bytes(instruction, 0));
        if (count > 1) {
            listing_column += printf(", ");
            write_instruction_operand(instruction, &instruction->operands[1],
                machine_operand_bytes(instruction, 1));
        }
    }
}

/* Original29990..29a55. Unlike pad_listing_column, count is relative to
   the current column. A nonpositive count still emits one separator. */
void __fastcall write_listing_spaces(int count)
{
    int current, first_tab, tabs;
    if (count <= 0) {
        listing_column += printf(" ");
        return;
    }
    current = (int)listing_column;
    first_tab = 8 - (current > 8 ? current % 8 : current);
    if (first_tab <= count) {
        count -= first_tab;
        printf("\t");
        listing_column += first_tab;
        if (count >= 8) {
            tabs = (unsigned)count >> 3;
            count -= tabs * 8;
            do {
                printf("\t");
                listing_column += 8;
            } while (--tabs);
        }
    }
    while (count-- > 0) listing_column += printf(" ");
}

/* Original29a60..29b2b. The offset prefix is followed by five bytes per
   physical row; continuation rows retain the prefix's final column. */
void __fastcall write_listing_bytes(unsigned offset, int count, unsigned char *bytes)
{
    unsigned indent;
    int i;
    pad_listing_column(2);
    listing_column += printf("%05lx", (unsigned long)offset);
    write_listing_spaces(1);
    indent = listing_column;
    for (i = 1; i <= count; ++i) {
        listing_column += printf("%02x", (unsigned)bytes[i - 1]);
        if (i != count) {
            if (i % 5) {
                listing_column += printf(" ");
            } else {
                listing_column = 0;
                printf("\n");
                write_listing_spaces(indent);
            }
        }
    }
}
/* Original [00028930,000289b0). Successful completion keeps the assembly
   output and closes any cached source files used by interleaved listings. */
void finish_globals(void)
{
    int i;
    if (!listing_enabled) return;
    listed_segment = close_segment_listing(listed_segment);
    printf("END");
    listing_column = 0;
    printf("\n");
    if (assembly_source) {
        for (i = 0; i < listing_source_count; ++i) fclose(listing_sources[i].file);
    }
    assembly_cleanup_pending = 0;
}
/* Original [00027ca0,00027fc0), including nine-way machine-record switch.
   Source interleaving owns cached FILEs; the ordinary listing only prints
   source-line annotations. Both use the same selected logical segment. */
void __fastcall write_function_listing(MachineInstruction *instruction)
{
    unsigned last_line = 0, offset = (unsigned)-1;
    int first = 1, i;
    MachineInstruction *next;
    if (!listing_enabled) return;
    if (assembly_source) prepare_source_listing(instruction);
    current_source_location = 0;
    listed_segment = write_segment_listing((Segment *)lookup_global_symbol(
        current_symbol->symbol.field18.identifier), 0);
    while (instruction) {
        if (!(instruction->opcode & 15)) {
            if (current_source_location != last_line) {
                last_line = current_source_location;
                if (assembly_source) write_listing_source(last_line);
                else write_listing_line(last_line);
            }
            next = write_instruction_listing(instruction, &offset);
        } else {
            next = instruction->next;
            switch (instruction->opcode & 15) {
            case 1:
                if (current_source_location != last_line && !assembly_source)
                    last_line = write_listing_line(current_source_location);
                write_listing_symbol(instruction->record.symbol);
                printf(":");
                listing_column = 0; printf("\n");
                offset = instruction->record.symbol->field04;
                break;
            case 2:
                for_each_symbol(1, write_local_listing);
                write_listing_symbol(&current_symbol->symbol);
                pad_listing_column(8);
                listing_column += printf("PROC NEAR");
                if (*current_symbol->symbol.name == '?')
                    first = write_demangled_listing(&current_symbol->symbol, first);
                if (current_symbol->symbol.field0e & 0x20) {
                    write_listing_comment(first);
                    printf("COMDAT");
                }
                listing_column = 0; printf("\n");
                first = 0;
                offset = current_symbol->symbol.field04;
                break;
            case 3:
                write_listing_symbol(&current_symbol->symbol);
                pad_listing_column(8);
                listing_column += printf("ENDP");
                if (*current_symbol->symbol.name == '?')
                    write_demangled_listing(&current_symbol->symbol, 1);
                listing_column = 0; printf("\n");
                if (assembly_source && listing_source_count) {
                    for (i = listing_source_index + 1; i < listing_source_count; ++i)
                        fclose(listing_sources[i].file);
                    listing_source_count = listing_source_index + 1;
                }
                break;
            case 4:
                current_source_location = instruction->record.source_location;
                break;
            case 5:
                if (instruction->record.filename != assembly_source_filename) {
                    current_source_location = last_line = 0;
                    assembly_source_filename = instruction->record.filename;
                    printf("; File %s", assembly_source_filename);
                    listing_column = 0; printf("\n");
                    if (assembly_source) open_listing_source(assembly_source_filename);
                }
                break;
            }
        }
        instruction = next;
    }
}
/* Original [00028760,000288d0). */
void __fastcall write_module_listing(char *source)
{
    Segment *segment;
    unsigned count = 0;
    char *code_group;
    listing_enabled = assembly_output || assembly_code;
    if (!listing_enabled) return;
    listing_column = 0;
    printf("\tTITLE\t%s\n\t.386P\n", source);
    printf("include listing.inc\n");
    if (dos_object) printf("if @Version gt 510\n.model FLAT\nelse\n");
    for (segment = segment_list; segment; segment = segment->segment_next) {
        if (segment->section_number != 10) {
            write_segment_listing(segment, 1);
            close_segment_listing(segment);
        }
    }
    if (!segment_list) compiler_internal_error("@(#)listing.c:1.76", 0xc4);
    printf("%s\tGROUP", target_segment_group);
    for (segment = segment_list; segment; segment = segment->segment_next) {
        if (!segment->format_flags) {
            if (count++) printf(",");
            printf(" %s", listing_segment_name(segment));
        }
    }
    listing_column = 0;
    printf("\n");
    code_group = dos_object ? "FLAT" : listing_segment_name(code_segment);
    printf("\tASSUME\tCS: %s, DS: %s, SS: %s\n", code_group,
           target_segment_group, target_segment_group);
    if (dos_object) printf("endif\n");
    listed_segment = 0;
}

/* Original [000289b0,00028a10). Listing declarations are emitted in phases. */
void write_symbol_declarations(void)
{
    if (!listing_enabled) return;
    for_each_symbol(0, write_public_listing);
    for_each_symbol(0, write_external_listing);
    for_each_symbol(1, write_external_listing);
    for_each_symbol(0, write_common_listing);
    for_each_symbol(1, write_common_listing);
    for_each_symbol(0, mark_listing_symbols);
}

#include "symbol-output.h"
#include <recon.h>
/* Original [000298f0,00029920). */
void __fastcall write_initializer_listing(BackendSymbolList *symbols)
{
    if (!listing_enabled) return;
    for (; symbols; symbols = symbols->next)
        write_initializer_assembly(find_initializer(symbols->symbol->identifier));
}

char *__fastcall listing_alignment_name(unsigned alignment);
/* Original28510..28674. A COMDAT descriptor names its own contribution
   before opening the parent's segment; ordinary segments use their class. */
Segment *__fastcall write_segment_listing(Segment *segment, int definition)
{
    char *modifier;
    Segment *parent;
    if (!segment) compiler_internal_error("@(#)listing.c:1.76", 0x4a2);
    if (segment != listed_segment) {
        listed_segment = close_segment_listing(listed_segment);
        if ((signed char)segment->word_size == 7) {
            parent = segment->parent;
            printf(";\tCOMDAT %s", listing_segment_name(segment));
            listing_column = 0;
            printf("\n");
            listing_column += printf("%s\tSEGMENT", listing_segment_name(parent));
            if (definition)
                printf(" %s USE32%s '%s'", listing_alignment_name(parent->alignment),
                    " PUBLIC", parent->class_name);
        } else {
            switch ((signed char)segment->word_size) {
            case 2: modifier = " PUBLIC"; break;
            case 6: modifier = " COMMON"; segment->field2c = 0; break;
            default: modifier = ""; break;
            }
            listing_column += printf("%s\tSEGMENT", listing_segment_name(segment));
            if (definition)
                printf(" %s USE32%s '%s'", listing_alignment_name(segment->alignment),
                    modifier, segment->class_name);
        }
        listing_column = 0;
        printf("\n");
    }
    return segment;
}

/* Original288e0..2891a. COMDAT contributions close their parent segment. */
Segment *__fastcall close_segment_listing(Segment *segment)
{
    if (segment) {
        if (segment->word_size == 7) segment = segment->parent;
        printf("%s\tENDS", listing_segment_name(segment));
        listing_column = 0;
        printf("\n");
    }
    return NULL;
}

/* Original288d0..288da; return the existing name's interior pointer. */
char *__fastcall listing_segment_name(Segment *segment)
{
    char *name = segment->symbol.name;
    if (*name == '.') ++name;
    return name;
}

/* Original29700..2973c. Only the original assembler alignment classes
   have names; the original returns NULL for every other value. */
char *__fastcall listing_alignment_name(unsigned alignment)
{
    switch (alignment) {
    case 1: return "BYTE";
    case 2: return "WORD";
    case 4: return "DWORD";
    case 8: case 16: return "PARA";
    default: return NULL;
    }
}

/* Original28dc0..28e97: emit public definitions that have not already
   been declared. Function and data descriptors share the output prefix. */
void __fastcall write_public_listing(SymbolHeader *symbol)
{
    BackendSymbol *entry = (BackendSymbol *)symbol;
    unsigned attributes;
    if (!(entry->output_flags & 4) || (entry->output_flags & 2)) return;
    if (symbol->kind != 1 && symbol->kind != 4) return;
    attributes = entry->attributes;
    if (symbol->kind == 4) {
        if ((attributes & 0xf0) != 0x50) return;
    } else if ((attributes & 0x780) != 0x80 || (attributes & 0x70) == 0x60)
        return;
    listed_segment = close_segment_listing(listed_segment);
    listing_column += printf("PUBLIC");
    pad_listing_column(8);
    listing_column += printf("%s", symbol_display_name(symbol));
    if (*symbol->name == '?') write_demangled_listing(symbol, 1);
    listing_column = 0;
    printf("\n");
}

char *__fastcall listing_data_type(long bytes);
/* Original28cc0..28db4. External declarations use the same output flag
   prefix as public declarations, with a function or sized-data class. */
void __fastcall write_external_listing(SymbolHeader *symbol)
{
    BackendSymbol *entry = (BackendSymbol *)symbol;
    char *type;
    if (!(entry->output_flags & 4) || (entry->output_flags & 2)) return;
    if (symbol->kind == 4) {
        if ((entry->attributes & 0xf0) != 0x40) return;
    } else if (symbol->kind != 1 || (entry->attributes & 0x780) != 0x100)
        return;
    listed_segment = close_segment_listing(listed_segment);
    if (symbol->kind == 4) type = "NEAR";
    else {
        type = listing_data_type(symbol->value);
        if (!type) type = listing_data_type(1);
    }
    listing_column += printf("EXTRN");
    pad_listing_column(8);
    listing_column += printf("%s:%s", symbol_display_name(symbol), type);
    if (*symbol->name == '?') write_demangled_listing(symbol, 1);
    listing_column = 0;
    printf("\n");
}

/* The object-data descriptor shares BackendSymbol's prefix. Original+2c
   supplies an explicit element size when attributes10000 is set. */
typedef struct ListingDataSymbol {
    BackendSymbol descriptor;
    unsigned field28;
    long element_bytes;
} ListingDataSymbol;
typedef char ListingDataSymbolSizeCheck[sizeof(ListingDataSymbol) == 48 ? 1 : -1];
char *__fastcall listing_data_directive(long bytes);
/* Original28a30..28cc0. Reserve uninitialized/common objects while keeping
   the listing's separate per-segment allocation position at Segment+2c. */
void __fastcall write_common_listing(SymbolHeader *symbol)
{
    BackendSymbol *entry = (BackendSymbol *)symbol;
    unsigned attributes, storage, category;
    long element_bytes, repeat, alignment;
    char *type, *name;
    if (!(entry->output_flags & 4) || (entry->output_flags & 2) || symbol->kind != 1)
        return;
    attributes = entry->attributes;
    if ((attributes & 0x780) == 0x100) return;
    category = attributes & 0x70;
    if (category != 0x20 && category != 0x60 && category != 0x50) return;
    if (symbol->field0e & 0x480) return;
    listed_segment = write_segment_listing((Segment *)lookup_global_symbol(
        symbol->field18.identifier), 0);
    attributes = entry->attributes;
    element_bytes = attributes & 0x10000 ?
        ((ListingDataSymbol *)symbol)->element_bytes : symbol->value;
    if (!listing_data_type(element_bytes)) element_bytes = 1;
    repeat = symbol->value / element_bytes;
    alignment = (long)listed_segment->alignment;
    if (element_bytes > 4) {
        if (alignment >= 8) alignment = 8;
    } else if (alignment >= 4) alignment = 4;
    switch ((attributes & 0x70) >> 4) {
    case 2:
        storage = (attributes & 0x780) >> 7;
        if (storage == 3) {
            type = listing_data_type(element_bytes);
            name = symbol_display_name(symbol);
            printf("COMM\t%s:%s", name, type);
            if (repeat > 1) printf(":0%lxH", (unsigned long)repeat);
            if (symbol->name && *symbol->name == '?') write_demangled_listing(symbol, 1);
            listing_column = 0;
            printf("\n");
            return;
        }
        if (storage != 1 && storage != 4) {
            compiler_internal_error("@(#)listing.c:1.76", 0x284);
            return;
        }
        break;
    case 6: break;
    default: return;
    }
    if (listed_segment->field2c % (unsigned)alignment) {
        printf("\tALIGN\t%d\n", alignment);
        listing_column = 0;
        printf("\n");
        listed_segment->field2c = (listed_segment->field2c + alignment - 1) &
            ~((unsigned)alignment - 1);
    }
    listing_column += printf("%s", symbol_display_name(symbol));
    pad_listing_column(8);
    listing_column += printf("%s", listing_data_directive(element_bytes));
    pad_listing_column(16);
    listing_column += printf("0%lxH DUP (?)", (unsigned long)repeat);
    if (symbol->name && *symbol->name == '?') write_demangled_listing(symbol, 1);
    listing_column = 0;
    printf("\n");
    listed_segment->field2c += (unsigned)repeat * (unsigned)element_bytes;
}

/* Original28a10..28a21. Remember declarations already emitted this pass. */
void __fastcall mark_listing_symbols(SymbolHeader *symbol)
{
    BackendSymbol *entry = (BackendSymbol *)symbol;
    if (entry->output_flags & 4) entry->output_flags |= 2;
}

SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol);
void __fastcall begin_initializer_listing(SymbolHeader *symbol, long offset, Segment *segment);
void __fastcall write_listing_origin(long offset);
unsigned __fastcall write_initializer_listing_item(Initializer *initializer);
/* Original29810..298e3. Block markers describe gaps relative to the
   symbol's start; other items return the number of bytes they printed. */
void __fastcall write_initializer_assembly(Initializer *initializer)
{
    SymbolHeader *symbol;
    Segment *segment;
    Initializer *item;
    unsigned bytes;
    if (!listing_enabled) return;
    symbol = lookup_global_symbol(initializer->content.reference.symbol_id);
    if ((symbol->kind == 1 || symbol->kind == 4) &&
        (((BackendSymbol *)symbol)->attributes & 0x1000))
        symbol = lookup_alias_target(symbol);
    segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
    begin_initializer_listing(symbol, initializer->content.reference.value, segment);
    for (item = initializer->next; item; item = item->next) {
        if (!item->kind) {
            bytes = (unsigned)item->content.reference.value -
                (segment->field2c - symbol->field04);
            if (!bytes) continue;
            write_listing_origin((long)bytes);
        } else {
            bytes = write_initializer_listing_item(item);
            if (initializer->next == item && symbol->name && *symbol->name == '?')
                write_demangled_listing(symbol, 1);
        }
        listing_column = 0;
        printf("\n");
        segment->field2c += bytes;
    }
}

/* Original29740..2980b, ECX symbol/EDX prefix and one callee-cleaned
   stack argument. Position the label, then reserve its initial byte prefix. */
void __fastcall begin_initializer_listing(SymbolHeader *symbol, long offset, Segment *segment)
{
    listed_segment = write_segment_listing(segment, 0);
    if (symbol->field04 != segment->field2c) {
        write_listing_origin((long)(symbol->field04 - segment->field2c));
        listing_column = 0;
        printf("\n");
        segment->field2c = symbol->field04;
    }
    listing_column += printf("%s", symbol_display_name(symbol));
    if (offset) {
        pad_listing_column(8);
        listing_column += printf("DB");
        pad_listing_column(16);
        printf("0%xH DUP (?)", (unsigned)offset);
        listing_column = 0;
        printf("\n");
        segment->field2c += (unsigned)offset;
    }
}

void __fastcall write_listing_string(unsigned char *bytes, int count);
void __fastcall write_listing_datum(Initializer *initializer);
/* Original29510..296f6. Each initializer contributes its original byte
   count; string repetitions share their payload and print on separate lines. */
unsigned __fastcall write_initializer_listing_item(Initializer *initializer)
{
    int bytes = -1, repeat, alignment;
    SymbolHeader *symbol;
    switch (initializer->kind) {
    case 1:
        repeat = initializer->repeat;
        bytes = initializer->content.datum.bytes * repeat;
        if (initializer->content.datum.type == 6) {
            if (repeat < 1) compiler_internal_error("@(#)listing.c:1.76", 0x1a6);
            while (repeat--) {
                write_listing_string(initializer->content.datum.value.bytes,
                    initializer->content.datum.bytes);
                if (repeat) {
                    listing_column = 0;
                    printf("\n");
                    pad_listing_column(8);
                }
            }
        } else write_listing_datum(initializer);
        break;
    case 2:
        bytes = (signed char)initializer->content.relocation.flags;
        pad_listing_column(8);
        listing_column += printf("%s", listing_data_directive(bytes));
        pad_listing_column(16);
        symbol = lookup_global_symbol(initializer->content.relocation.symbol_id);
        if ((symbol->kind == 4 || symbol->kind == 1) &&
            (((BackendSymbol *)symbol)->attributes & 0x1000))
            symbol = lookup_alias_target(symbol);
        listing_column += printf("%s%s", dos_object ? "FLAT:" : "",
            symbol_display_name(symbol));
        if (initializer->content.relocation.offset)
            listing_column += printf("%+ld", initializer->content.relocation.offset);
        break;
    case 4:
        alignment = (signed char)initializer->content.byte_argument;
        bytes = (alignment - listed_segment->field2c) & (alignment - 1);
        pad_listing_column(8);
        /* Original passes the alignment-name pointer to this %d conversion.
           Keep that original call shape; this case has not reached it. */
        listing_column += printf("ALIGN\t%d", listing_alignment_name(alignment));
        break;
    case 8:
        bytes = initializer->content.argument;
        write_listing_origin(bytes);
        break;
    case 3:
        break;
    default:
        compiler_internal_error("@(#)listing.c:1.76", 0x1cc);
        break;
    }
    return (unsigned)bytes;
}

void __fastcall write_listing_float(unsigned type, InitializerValue *value, int repeat);
/* Original29340..29483. Integer literals retain their encoded width;
   floating literals are delegated with the original precision selector. */
void __fastcall write_listing_datum(Initializer *initializer)
{
    int repeat = initializer->repeat;
    if (initializer->content.datum.type == 5) {
        switch (initializer->content.datum.bytes) {
        case 4: write_listing_float(0x1004, &initializer->content.datum.value, repeat); break;
        case 8: write_listing_float(0x1008, &initializer->content.datum.value, repeat); break;
        case 10: write_listing_float(0x100a, &initializer->content.datum.value, repeat); break;
        default: compiler_internal_error("@(#)listing.c:1.76", 0x17e); break;
        }
        return;
    }
    pad_listing_column(8);
    listing_column += printf("%s", listing_data_directive(initializer->content.datum.bytes));
    pad_listing_column(16);
    if (repeat > 1) listing_column += printf("%ld DUP(", repeat);
    switch (initializer->content.datum.bytes) {
    case 1: listing_column += printf("0%01xH", (unsigned)initializer->content.datum.value.byte); break;
    case 2: listing_column += printf("0%xH", (unsigned)(unsigned short)initializer->content.datum.value.word); break;
    case 4: listing_column += printf("0%lxH", initializer->content.datum.value.integer); break;
    default: compiler_internal_error("@(#)listing.c:1.76", 0x191); break;
    }
    if (repeat > 1) listing_column += printf(")");
}

/* Original28160..28225. Account for tab stops in the listing's logical
   columns, emitting at least one separator when the target is behind us. */
void __fastcall pad_listing_column(unsigned column)
{
    int current = (int)listing_column, remaining, first_tab;
    if (current >= (int)column) {
        listing_column += printf(" ");
        return;
    }
    remaining = (int)column - current;
    first_tab = 8 - (current > 8 ? current % 8 : current);
    if (remaining >= first_tab) {
        remaining -= first_tab;
        printf("\t");
        listing_column += first_tab;
    }
    while (remaining >= 8) {
        remaining -= 8;
        printf("\t");
        listing_column += 8;
    }
    while (remaining-- > 0) listing_column += printf(" ");
}

/* Original29490..294dc with10-entry jump table294dc..29504. */
char *__fastcall listing_data_directive(long bytes)
{
    switch (bytes) {
    case 1: return "DB";
    case 2: return "DW";
    case 4: return "DD";
    case 6: return "DF";
    case 8: return "DQ";
    case 10: return "DT";
    default:
        compiler_internal_error("@(#)listing.c:1.76", 0x514);
        return NULL;
    }
}

/* Original28920..2892f. The caller owns column reset and line ending. */
void __fastcall write_listing_origin(long offset)
{
    printf("\tORG $%+ld", offset);
}

/* Original28ea0..290b0. Printable bytes share quoted runs; control bytes
   are hexadecimal operands. A doubled quote represents an apostrophe. */
void __fastcall write_listing_string(unsigned char *bytes, int count)
{
    int first = 1, quoted = 0, printable;
    unsigned byte;
    pad_listing_column(8);
    listing_column += printf("DB");
    pad_listing_column(16);
    while (count--) {
        if ((int)listing_column > 72) {
            if (quoted) {
                printf("%c", '\'');
                quoted = 0;
            }
            listing_column = 0;
            printf("\n");
            pad_listing_column(8);
            listing_column += printf("DB\t");
            first = 1;
        }
        byte = *bytes++;
        printable = MB_CUR_MAX > 1 ? _isctype(byte, 0x157) : (_pctype[byte] & 0x157);
        if (printable) {
            if (!quoted) {
                if (!first) listing_column += printf(", ");
                listing_column += printf("%c", '\'');
                quoted = 1;
            }
            if (byte == '\'') listing_column += printf("%c", '\'');
            listing_column += printf("%c", byte);
        } else {
            if (quoted) {
                listing_column += printf("%c", '\'');
                quoted = 0;
            }
            if (!first) listing_column += printf(", ");
            listing_column += printf("0%xH", byte);
        }
        first = 0;
    }
    if (quoted) listing_column += printf("%c", '\'');
}

/* Original290b0..2933e. The assembler's real-number suffix prints the raw
   encoded bits; decimal comments use the original first4/8 payload bytes.
   The target's long double is8 bytes. Explicit double-to-long-double promotion
   keeps its first8-byte interpretation and the printf L argument type in both
   the original compiler and the native host's wider printf ABI. */
#define BEGIN_FLOAT_LISTING(directive) \
    pad_listing_column(8); \
    listing_column += printf(directive); \
    pad_listing_column(16); \
    if (repeat != 1) listing_column += printf("%ld DUP(", (long)repeat)
#define END_FLOAT_BITS() \
    if (repeat != 1) listing_column += printf(")"); \
    pad_listing_column(48)
void __fastcall write_listing_float(unsigned type, InitializerValue *value, int repeat)
{
    switch (type) {
    case 0x1004:
        BEGIN_FLOAT_LISTING("DD");
        listing_column += printf("0%.8lxr", (unsigned long)value->float_bits);
        END_FLOAT_BITS();
        listing_column += printf("; %g", (double)*(float *)value);
        break;
    case 0x1008:
        BEGIN_FLOAT_LISTING("DQ");
        listing_column += printf("0%.8lx%.8lxr", value->double_bits.high, value->double_bits.low);
        END_FLOAT_BITS();
        listing_column += printf("; %g", *(double *)value);
        break;
    case 0x100a:
        BEGIN_FLOAT_LISTING("DT");
        listing_column += printf("0%.8lx%.8lx%.4xr",
            *(unsigned long *)((unsigned char *)value + 6),
            *(unsigned long *)((unsigned char *)value + 2), *(unsigned short *)value);
        END_FLOAT_BITS();
        /* VC1 reports C4069 for the intentional identity conversion: its
           long double is double. The wider native vararg requires the cast. */
#ifdef _MSC_VER
#pragma warning(disable:4069)
#endif
        listing_column += printf("; %Lg", (long double)*(double *)value);
#ifdef _MSC_VER
#pragma warning(default:4069)
#endif
        break;
    }
}
#undef BEGIN_FLOAT_LISTING
#undef END_FLOAT_BITS
