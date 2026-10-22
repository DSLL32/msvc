/* Assembly listing output. */
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
/* Original [000298f0,00029920). */
void __fastcall write_initializer_listing(BackendSymbolList *symbols)
{
    if (!listing_enabled) return;
    for (; symbols; symbols = symbols->next)
        write_initializer_assembly(find_initializer(symbols->symbol->identifier));
}
