/* Assembly listing output. */
#include "listing.h"
#include "options.h"
#include "output.h"
extern int dos_object;
extern Segment *segment_list;
int listing_enabled; /* 0007ca9c */
unsigned listing_column; /* 00080d40 */
Segment *listed_segment; /* 0007caa0 */
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
