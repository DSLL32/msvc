/* COFF module adapter, original [0006c820,0006c920). */
#include "coff-output.h"
#include "backend.h"
#include "records.h"
#include "options.h"
#include <time.h>
extern int codeview_debug, line_debug;
extern int reproducible_build;
extern char *generated_source;
extern unsigned next_symbol_identifier(void);
FILE *object_file; /* 00084fb0 */
Segment *directive_segment; /* 00079144 */
int object_generation_started; /* 00083158 */
/* Original [0006c920,0006c950). The reproducible-object switch suppresses
   the timestamp; otherwise time fills the original 32-bit time_t object. */
unsigned coff_timestamp(void)
{
    time_t stamp;
    if (reproducible_build) return 0;
    time(&stamp);
    return (unsigned)stamp;
}
void coff_open_module(void)
{
    CoffConfiguration *configuration;
    object_file = fopen(object_filename, "w");
    if (!object_file) report_context_error(0x53, generated_source, object_filename);
    object_generation_started = 1;
    configuration = arena_alloc(7, sizeof(CoffConfiguration));
    configuration->file = object_file;
    configuration->version = 1;
    configuration->machine = 0x14c;
    configuration->flags = (unsigned)-1;
    configuration->source = source_filename ? arena_strdup(source_filename, 7) : "";
    configuration->error = coff_error;
    configuration->allocate = coff_allocate;
    configuration->timestamp = coff_timestamp;
    configuration->debug = (codeview_debug || line_debug) ? 1 : 0;
    coff_initialize(configuration);
    directive_segment = create_segment(".drectve", "DRECTVE", next_symbol_identifier());
    directive_segment->alignment = 1;
    directive_segment->word_size = 0;
    directive_segment->format_flags = 1;
    directive_segment->section_number = 10;
    write_module_directives();
}

/* Original [0006c950,0006c960), arena-seven allocation adapter. */
void *__fastcall coff_allocate(unsigned bytes)
{
    return arena_alloc(7, bytes);
}

#include "output.h"
int coff_precompiled_types; /* 0007e8c0 */
/* Shared compiler identification used by object and CodeView records. */
char compiler_identification[] = "@(#) Microsoft C/C++ 32 bits x86 Compiler Version 8.00.XXXX";
/* Original [0006d9b0,0006da30). */
void __fastcall coff_output_record(ModuleDirective *directive)
{
    switch (directive->kind) {
    case 1: coff_write_comment(compiler_identification); break;
    case 3: coff_write_directive("-?comment:", directive->text, 1); break;
    case 4: coff_write_directive("-defaultlib:", directive->text, 0); break;
    case 6: coff_write_comment(directive->text); break;
    case 7: coff_precompiled_types = 1; break;
    default: compiler_internal_error("@(#)coff.c:1.14", 0x3d1); break;
    }
}

/* Original [0006d440,0006d4f0). */
void __fastcall coff_write_directive(char *prefix, char *text, int quote)
{
    unsigned length;
    coff_select_segment(directive_segment);
    length = 0;
    while (prefix[length]) ++length;
    coff_write_data(prefix, length);
    directive_segment->symbol.field04 += length;
    if (quote) {
        coff_write_byte('"');
        length = 0;
        while (text[length]) ++length;
        coff_write_data(text, length);
        coff_write_byte('"');
        length += 2;
    } else {
        length = 0;
        while (text[length]) ++length;
        coff_write_data(text, length);
    }
    coff_write_byte(' ');
    directive_segment->symbol.field04 += length + 1;
}

#include "coff-writer.h"
#include "coff-segments.h"
#include <recon.h>
/* Original [0006ca50,0006cac0). COFF encodes log2(alignment)+1 in the
   section-characteristics alignment field, starting at bit twenty. */
unsigned __fastcall coff_alignment_flags(unsigned alignment)
{
    switch (alignment) {
    case 1: return 0x00100000;
    case 2: return 0x00200000;
    case 4: return 0x00300000;
    case 8: return 0x00400000;
    case 16: return 0x00500000;
    case 32: return 0x00600000;
    case 64: return 0x00700000;
    }
    compiler_internal_error("@(#)coff.c:1.14", 0x518);
    return 0;
}
/* Original [0006c9a0,0006ca10). Finalize logical-section alignment before
   closing contributions and writing the COFF directory and symbol table. */
void coff_finish_module(void)
{
    unsigned i, flags;
    CoffSegmentLink *link;
    Segment *segment;
    for (i = 0; i < 16; ++i) {
        for (link = coff_segment_index[i]; link; link = link->next) {
            segment = link->segment;
            flags = coff_alignment_flags(segment->alignment);
            coff_set_section_flags((unsigned short)segment->object_section, flags);
            if (segment->symbol.field04) coff_select_segment(segment);
        }
    }
    coff_close_all_sections();
    coff_finalize();
    fclose(object_file);
    object_generation_started = 0;
}
/* Original [0006f030,0006f040): the distinct public data-output adapter. */
void __fastcall coff_write_data(void *data, unsigned bytes)
{
    coff_write_scope_data(data, bytes);
}
