/* Assembly listing output. */
#include <ctype.h>
#include "listing.h"
#include "options.h"
#include "output.h"
#include "function-input.h"
#include "records.h"
extern int dos_object;
extern Segment *segment_list;
ListingSource listing_sources[20]; /* 0007c954..0007ca94: up to twenty source files */
int listing_source_count, listing_source_index; /* 0007e1c8,0007e1cc */
char *assembly_source_filename; /* 0007e1c0 */
int listing_enabled; /* 0007ca9c */
unsigned listing_column; /* 00080d40 */
Segment *listed_segment; /* 0007caa0 */
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
