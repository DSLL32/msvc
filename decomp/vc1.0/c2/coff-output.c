/* COFF module adapter, original [0006c820,0006c920). */
#include "coff-output.h"
#include "backend.h"
#include "records.h"
#include "options.h"
extern int codeview_debug, line_debug;
extern char *generated_source;
extern unsigned next_symbol_identifier(void);
void report_context_error(void);
FILE *object_file; /* 00084fb0 */
Segment *directive_segment; /* 00079144 */
int object_generation_started; /* 00083158 */
void coff_open_module(void)
{
    CoffConfiguration *configuration;
    object_file = fopen(object_filename, "w");
    if (!object_file) {
        __asm {
            mov eax, object_filename
            push eax
            mov edx, generated_source
            mov ecx, 53h
            call report_context_error
            add esp, 4
        }
    }
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
