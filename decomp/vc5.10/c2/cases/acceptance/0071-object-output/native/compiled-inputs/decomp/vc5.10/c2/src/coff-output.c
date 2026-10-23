#include "coff-output.h"
#include <recon.h>

extern char *c2_output_name, *c2_source_name;
extern int deferred_compilation, c2_codeview_types, c2_line_numbers;
FILE *object_output;
int object_output_open;
void *incremental_data;
C2Section *directive_section, *fpo_section;

void initialize_object_output(void)
{
    C2ObjectOutput *writer;
    C2Section *section;
    unsigned bytes;

    if (c2_internal_listing) {
        object_output = fopen(c2_output_name, "rb");
        if (object_output) {
            fclose(object_output);
            bytes = query_incremental_stream_size();
            if (bytes > 0) {
                incremental_data = c2_pool_allocate(4, bytes + 1);
                if (read_incremental_stream(incremental_data, bytes) != bytes)
                    c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 195);
                if (read_incremental_byte() == 5 && validate_incremental_options())
                    deferred_compilation = 1;
            }
        }
    }
    object_output = fopen(c2_output_name, deferred_compilation ? "r+b" : "wb");
    if (!object_output)
        c2_system_fatal(83, c2_get_diagnostic_string(310), c2_output_name);
    object_output_open = 1;
    writer = c2_pool_allocate(7, sizeof(*writer));
    writer->file = object_output;
    writer->format = 1;
    writer->machine = 0x14c;
    writer->pending04 = -1;
    writer->source_name = c2_source_name ? arena_strdup(c2_source_name, 7) : "";
    writer->output_error.target = writer_callback_48511c;
    writer->output_allocator.target = writer_callback_41eb8a;
    writer->output_clock.target = writer_callback_45477a;
    writer->debug = c2_codeview_types || c2_line_numbers ? 1 : 0;
    if (!initialize_coff_output(writer))
        c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 234);
    if (deferred_compilation) {
        prepare_output_486e67();
        prepare_output_486a23();
        prepare_output_48710f();
    }
    section = create_section(".drectve", "DRECTVE", next_symbol_serial());
    section->alignment = 1;
    section->combine = 0;
    section->attribute = 1;
    directive_section = section;
    section->section_kind = 10;
    section = create_section(".debug$F", "DEBFPO", next_symbol_serial());
    section->alignment = 1;
    section->combine = 0;
    section->section_kind = 9;
    fpo_section = section;
    section->attribute = 1;
    write_output_directives();
}
