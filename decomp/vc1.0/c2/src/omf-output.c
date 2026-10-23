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
int __fastcall omf_flush_record_type(unsigned type);
OmfRecord *__fastcall omf_find_record(unsigned type, void *context);
OmfRecord *__fastcall omf_allocate_record(unsigned type, void *context);
OmfRecord *__fastcall omf_flush_record(OmfRecord *record);
OmfRecord *__fastcall omf_initialize_record(OmfRecord *record, unsigned type, void *context);
void __fastcall omf_pad_record(OmfRecord *record, unsigned bytes);
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
unsigned char omf_comdat_flags; /* 00085090, cached COMDAT flags */
int omf_fixup_count; /* 00085094, pending fixup entries */
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
/* Original [0006b480,0006b64f). Flush incompatible record classes before
   looking for a reusable slot. Record ids remain explicit here; vendor
   record writers supply their meanings as those paths are recovered. */
OmfRecord *__fastcall omf_begin_record(unsigned type, void *context)
{
    OmfRecord *record;
    switch (type) {
    case 0x80:
        omf_flush_record_type(0x95);
        omf_flush_record_type(0xc5);
        break;
    case 0x8c:
        if (!omf_flush_record_type(0xb0) && !omf_flush_record_type(0xb4))
            omf_flush_record_type(0xbc);
        omf_flush_record_type(0x91);
        break;
    case 0x91:
        omf_flush_record_type(0x8c);
        break;
    case 0x96:
        omf_flush_record_type(0xca);
        break;
    case 0xa1:
        omf_flush_record_type(0xc3);
        break;
    case 0xb0:
        if (!omf_flush_record_type(0x8c) && !omf_flush_record_type(0xb4))
            omf_flush_record_type(0xbc);
        break;
    case 0xb4:
        if (!omf_flush_record_type(0x8c) && !omf_flush_record_type(0xb0))
            omf_flush_record_type(0xbc);
        omf_flush_record_type(0xb7);
        break;
    case 0xb7:
        omf_flush_record_type(0xb4);
        break;
    case 0xbc:
        if (!omf_flush_record_type(0x8c) && !omf_flush_record_type(0xb4))
            omf_flush_record_type(0xb0);
        break;
    case 0xc3:
        omf_flush_record_type(0xa1);
        if (!((SymbolHeader *)context)->field04) omf_flush_record_type(0xc5);
        break;
    case 0xca:
        omf_flush_record_type(0x96);
        break;
    }
    record = omf_find_record(type, context);
    if (!record) record = omf_allocate_record(type, context);
    omf_current_record = record;
    return record;
}
/* Original [0006aaf0,0006ab15). The first matching slot is sufficient,
   and a found record is reported even if its writer returns another slot. */
int __fastcall omf_flush_record_type(unsigned type)
{
    OmfRecord *record;
    for (record = omf_records; record < omf_records + 4; ++record) {
        if (record->type == type) {
            omf_flush_record(record);
            return 1;
        }
    }
    return 0;
}
/* Original [0006bd50,0006bf27), with its75-way dispatch table at
   [0006bf28,0006c054). Search from the current slot, wrapping at slot3.
   Capacity is signed, but stream positions, padding and ring addresses use
   the original unsigned32-bit arithmetic. The cursor limit is slot+998. */
OmfRecord *__fastcall omf_find_record(unsigned type, void *context)
{
    OmfRecord *first = omf_current_record, *record = first;
    Segment *old_segment, *segment = (Segment *)context;
    long remaining;
    unsigned end, gap, alignment;
    do {
        if (record->type == type) {
            remaining = (long)((unsigned)record - (unsigned)record->cursor + 998);
            switch (type) {
            case 0x80: case 0x88: case 0x8a: case 0x8e: case 0x99: case 0xa3:
                return omf_initialize_record(record, type, context);
            case 0x8c: case 0x96: case 0xb0: case 0xb4: case 0xca:
                if (remaining < 259)
                    return omf_initialize_record(record, type, context);
                return record;
            case 0x91: case 0xb7:
                old_segment = (Segment *)record->context;
                if (((OmfGroup *)old_segment->symbol.field18.pointer)->number !=
                    ((OmfGroup *)segment->symbol.field18.pointer)->number ||
                    old_segment->identity.order != segment->identity.order || remaining < 256)
                    return omf_initialize_record(record, type, context);
                return record;
            case 0x95: case 0xc5:
                if (remaining < 6 || record->context != context)
                    return omf_initialize_record(record, type, context);
                return record;
            case 0x9a:
                old_segment = (Segment *)record->context;
                if (((OmfGroup *)old_segment->symbol.field18.pointer)->name_index !=
                    ((OmfGroup *)segment->symbol.field18.pointer)->name_index || remaining < 25)
                    return omf_initialize_record(record, type, context);
                return record;
            case 0x9d:
                if (remaining < 11)
                    return omf_initialize_record(record, type, context);
                return record;
            case 0xbc:
                if (remaining < 4)
                    return omf_initialize_record(record, type, context);
                return record;
            case 0xc3:
                if ((segment->object_attributes.omf_comdat.flags ^ omf_comdat_flags) & 2)
                    return omf_initialize_record(record, type, context);
                /* Matching COMDAT flags use the ordinary data-run rules. */
            case 0xa1:
                if (record->context != context || remaining <= 20 || omf_fixup_count >= 249)
                    return omf_initialize_record(record, type, context);
                end = record->context_offset + record->count;
                if (end == segment->symbol.field04) return record;
                if (end < segment->symbol.field04) {
                    gap = segment->symbol.field04 - end;
                    alignment = segment->alignment;
                    if ((long)alignment >= 4) alignment = 4;
                    if (alignment > gap && gap + 20 < (unsigned)remaining) {
                        omf_pad_record(record, gap);
                        return record;
                    }
                }
                return omf_initialize_record(record, type, context);
            }
        }
        if ((unsigned)record < (unsigned)omf_records ||
            (unsigned)record >= (unsigned)(omf_records + 3)) record = omf_records;
        else ++record;
    } while (record != first);
    return NULL;
}
/* Original [0006c250,0006c2a3). Prefer the first unused slot; a full cache
   replaces the successor of the current slot, with the same address wrap
   rule as the lookup. Initialization may flush and return a different slot. */
OmfRecord *__fastcall omf_allocate_record(unsigned type, void *context)
{
    OmfRecord *record;
    for (record = omf_records; record < omf_records + 4; ++record)
        if (!record->type) return omf_initialize_record(record, type, context);
    record = omf_current_record;
    if ((unsigned)record < (unsigned)omf_records ||
        (unsigned)record >= (unsigned)(omf_records + 3)) record = omf_records;
    else ++record;
    return omf_initialize_record(record, type, context);
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
