/* OMF module names, segment declarations and predefined fixup threads. */
#include "omf-output.h"
#include "options.h"
#include "records.h"
#include <recon.h>
extern int floating_frontend, codeview_debug, dos_object;
extern char *generated_source;
extern int object_generation_started;
extern Segment *segment_list;
OmfRecord *__fastcall omf_begin_record(unsigned type, void *context);
void __fastcall omf_append_string(OmfRecord *record, char *text);
/* Original84fc4 points at the existing two-word object84fb8. Preserve that
   identity, rather than allocating a new group or inventing initialization. */
OmfGroup omf_group_storage;
OmfGroup *omf_default_group = &omf_group_storage;
OmfFixupThreads omf_fixup_threads; /* 0007a8c4..0007a8d8 */
unsigned omf_empty_name; /* 0007a8d8 */
unsigned omf_next_group = 1; /* 0008507c */
unsigned omf_next_segment = 1; /* 00085080 */
unsigned omf_next_frame_thread; /* 00085088 */
unsigned omf_next_target_thread; /* 0008508c */
OmfRecord omf_records[4]; /* 0007a8dc..0007b88c, original BSS */
OmfRecord *omf_current_record; /* 0007b88c, original BSS */
/* Original [0006a9a0,0006a9c2). Tags alone are reset; buffered fields and
   serialized bytes retain their previous contents when the cache is reused. */
void omf_initialize_records(void)
{
    OmfRecord *record;
    for (record = omf_records; record < omf_records + 4; ++record)
        record->type = 0;
    omf_current_record = omf_records;
}
/* Original [0006a980,0006a99f), followed by a single alignment NOP. */
void __fastcall omf_write_module_header(char *source)
{
    OmfRecord *record = omf_begin_record(0x80, NULL);
    omf_append_string(record, source);
    omf_flush_records();
}
/* Original [0006a350,0006a7e5); eleven following bytes are alignment.
   The four tail words of Segment have format-specific meanings: OMF names,
   header end positions and class indexes share the COFF descriptor layout. */
void omf_open_module(void)
{
    Segment *segment;
    OmfGroup *group;
    FunctionSymbol *symbol;
    unsigned empty;
    omf_initialize_records();
    object_file = fopen(object_filename, "w");
    if (!object_file) report_context_error(0x53, generated_source, object_filename);
    object_generation_started = 1;
    omf_write_module_header(source_filename);
    add_module_directive(8, floating_frontend ? "MS F" : "MS C");
    add_module_directive(9, "3sO");
    add_module_directive(11, "\001CV");
    write_module_directives();
    omf_flush_records();
    omf_empty_name = omf_name_index("");
    group = omf_default_group;
    group->number = omf_next_group++;
    group->name_index = omf_name_index(target_segment_group);
    segment = code_segment;
    segment->object_section = omf_name_index(segment->symbol.name);
    segment = code_segment;
    segment->object_attributes.omf_class_index = omf_name_index(segment->class_name);
    segment = data_segment;
    segment->object_section = omf_name_index(segment->symbol.name);
    segment = data_segment;
    segment->object_attributes.omf_class_index = omf_name_index(segment->class_name);
    segment = const_segment;
    segment->object_section = omf_name_index(segment->symbol.name);
    segment = const_segment;
    segment->object_attributes.omf_class_index = segment->object_section;
    segment = bss_segment;
    segment->object_section = omf_name_index(segment->symbol.name);
    segment = bss_segment;
    segment->object_attributes.omf_class_index = omf_name_index(segment->class_name);
    if (codeview_debug) {
        segment = debug_symbols_segment;
        segment->object_section = omf_name_index(segment->symbol.name);
        segment = debug_symbols_segment;
        segment->object_attributes.omf_class_index = omf_name_index(segment->class_name);
        segment = debug_types_segment;
        segment->object_section = omf_name_index(segment->symbol.name);
        segment = debug_types_segment;
        segment->object_attributes.omf_class_index = omf_name_index(segment->class_name);
    }
    empty = omf_empty_name;
    for (segment = segment_list; segment; segment = segment->segment_next) {
        if (segment->word_size == 7) {
            symbol = (FunctionSymbol *)segment->identity.owner;
            if (symbol->symbol.kind != 4 || (symbol->field24 & 0xf0) != 0x50 ||
                !(symbol->flags52 & 0x20)) continue;
            if (!*symbol->symbol.name) segment->object_section = empty;
            else {
                segment->object_section = omf_name_index(symbol_display_name(&symbol->symbol));
                empty = omf_empty_name;
            }
        } else if (!segment->object_attributes.omf_class_index) {
            if (!*segment->symbol.name) segment->object_section = empty;
            else {
                segment->object_section = omf_name_index(segment->symbol.name);
                empty = omf_empty_name;
            }
            if (!*segment->class_name) {
                segment->object_attributes.omf_class_index = segment->section_number ?
                    data_segment->object_attributes.omf_class_index :
                    code_segment->object_attributes.omf_class_index;
            } else {
                segment->object_attributes.omf_class_index = omf_name_index(segment->class_name);
                empty = omf_empty_name;
            }
        }
    }
    omf_flush_records();
    omf_write_segment_definition(code_segment);
    code_segment->omf_header_end = ftell(object_file);
    omf_write_segment_definition(data_segment);
    data_segment->omf_header_end = ftell(object_file);
    omf_write_segment_definition(const_segment);
    const_segment->omf_header_end = ftell(object_file);
    omf_write_segment_definition(bss_segment);
    bss_segment->omf_header_end = ftell(object_file);
    if (codeview_debug) {
        omf_write_segment_definition(debug_symbols_segment);
        debug_symbols_segment->omf_header_end = ftell(object_file);
        omf_write_segment_definition(debug_types_segment);
        debug_types_segment->omf_header_end = ftell(object_file);
    }
    for (segment = segment_list; segment; segment = segment->segment_next) {
        if (segment->word_size != 7 && !segment->omf_header_end) {
            segment->identity.order = omf_next_segment;
            omf_write_segment_definition(segment);
            segment->omf_header_end = ftell(object_file);
        }
    }
    omf_flush_records();
    if (dos_object) omf_write_group_definition(data_segment, omf_default_group);
    else {
        for (segment = segment_list; segment; segment = segment->segment_next) {
            if (!segment->format_flags)
                omf_write_group_definition(segment, omf_default_group);
        }
    }
    omf_flush_records();
    omf_fixup_threads.const_target = omf_define_fixup_thread(0, 0,
        const_segment->identity.order, omf_next_target_thread++);
    omf_fixup_threads.data_target = omf_define_fixup_thread(0, 0,
        data_segment->identity.order, omf_next_target_thread++);
    omf_fixup_threads.code_target = omf_define_fixup_thread(0, 0,
        code_segment->identity.order, omf_next_target_thread++);
    omf_fixup_threads.bss_target = omf_define_fixup_thread(0, 0,
        bss_segment->identity.order, omf_next_target_thread++);
    omf_fixup_threads.group_frame = omf_define_fixup_thread(0x40, 1,
        omf_default_group->number, omf_next_frame_thread++);
    omf_flush_records();
}
