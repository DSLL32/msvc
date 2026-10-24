/* OMF module names, segment declarations and predefined fixup threads. */
#include "omf-output.h"
#include "options.h"
#include "records.h"
#include "machine.h"
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
void __fastcall omf_write_index(OmfRecord *record, int index);
unsigned char __fastcall omf_comdat_selection(unsigned char selection);
unsigned __fastcall omf_alignment(unsigned alignment);
void __fastcall omf_append_bytes(OmfRecord *record, char *bytes, unsigned count);
void __fastcall omf_write_record(OmfRecord *record);
void omf_write_fixups(void);
OmfRecord *__fastcall omf_reserve_bytes(OmfRecord *record, unsigned bytes);
void __fastcall omf_write_comment(unsigned char kind, char *text);
int __fastcall omf_intern_name(char *name, unsigned *index);
void __fastcall omf_emit_public(SymbolHeader *symbol, Segment *segment);
void __fastcall omf_emit_external(SymbolHeader *symbol);
void __fastcall omf_emit_local_external(SymbolHeader *symbol);
void __fastcall omf_emit_comdat_external(SymbolHeader *symbol);
unsigned __fastcall omf_local_name_index(char *name);
void __fastcall omf_emit_communal(SymbolHeader *symbol);
void __fastcall omf_write_communal_size(OmfRecord *record, int size);
void __fastcall omf_emit_weak_external(FunctionSymbol *symbol);
OmfFixup *omf_allocate_fixup(void);
void __fastcall omf_add_fixup(SymbolHeader *symbol, unsigned char type, unsigned displacement, int relative);
void __fastcall omf_write_byte_array(Segment *segment, unsigned bytes, unsigned repeat, void *data);
void __fastcall omf_write_iterated_data(Segment *segment, int repeat, unsigned bytes, void *data);
unsigned __fastcall omf_write_datum(OmfRecord *record, Initializer *initializer);
/* Original name interner: hash first five signed bytes into16 buckets.
   Nodes allocated at6c5ad have name/index/next at0/4/8 and size12. */
typedef struct OmfName {
    char *name;
    unsigned index;
    struct OmfName *next;
} OmfName;
typedef char OmfNameSizeCheck[sizeof(OmfName) == 12 ? 1 : -1];
OmfName *omf_name_buckets[16]; /* 0007b890..0007b8d0, original BSS */
unsigned omf_next_name = 1; /* 00085084, original PE initializer */
/* Original84fc4 points at the existing two-word object84fb8. Preserve that
   identity, rather than allocating a new group or inventing initialization. */
OmfGroup omf_group_storage;
OmfGroup *omf_default_group = &omf_group_storage;
OmfFixupThreads omf_fixup_threads; /* 0007a8c4..0007a8d8 */
unsigned omf_empty_name; /* 0007a8d8 */
unsigned omf_next_external = 1; /* 00085078, original PE initializer */
unsigned omf_next_group = 1; /* 0008507c */
unsigned omf_next_segment = 1; /* 00085080 */
unsigned omf_next_frame_thread; /* 00085088 */
unsigned omf_next_target_thread; /* 0008508c */
unsigned char omf_comdat_flags; /* 00085090, cached COMDAT flags */
OmfFixup omf_fixups[250]; /* 00079154..0007a8c4, original BSS pool */
typedef char OmfFixupPoolSizeCheck[sizeof(omf_fixups) == 6000 ? 1 : -1];
int omf_fixup_count; /* 00085094, pending fixup entries */
unsigned omf_last_comdat_name; /* 0007e8b0, last LINSYM name index */
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
                    segment->symbol.field18.group->number ||
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
                    segment->symbol.field18.group->name_index || remaining < 25)
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
/* Original [0006c060,0006c243). The three wire-header bytes precede a
   type-specific prefix. Count tracks writes while building that prefix, then
   resets to zero: subsequent count is payload beyond the prefix, not its size.
   The flush may return another physical slot, so every field uses its result. */
#define OMF_RECORD_BYTE(r,v) do { \
    ++(r)->count; *(r)->cursor = (unsigned char)(v); ++(r)->cursor; \
} while (0)
OmfRecord *__fastcall omf_initialize_record(OmfRecord *record, unsigned type, void *context)
{
    Segment *segment = (Segment *)context, *parent;
    unsigned value;
    if (record->type) record = omf_flush_record(record);
    record->type = type;
    record->context = context;
    record->context_offset = context ? ((SymbolHeader *)context)->field04 : 0;
    record->bytes[0] = (unsigned char)type;
    record->cursor = record->bytes + 3;
    switch (type) {
    case 0x91: case 0x95: case 0xb7:
        omf_write_index(record, segment->symbol.field18.group->number);
        omf_write_index(record, segment->identity.order);
        break;
    case 0x9a:
        OMF_RECORD_BYTE(record, segment->symbol.field18.group->name_index);
        break;
    case 0xa1: case 0xa3:
        omf_write_index(record, segment->identity.order);
        value = segment->symbol.field04;
        OMF_RECORD_BYTE(record, value);
        OMF_RECORD_BYTE(record, value >> 8);
        OMF_RECORD_BYTE(record, value >> 16);
        OMF_RECORD_BYTE(record, value >> 24);
        break;
    case 0xc3:
        omf_comdat_flags = segment->object_attributes.omf_comdat.flags;
        OMF_RECORD_BYTE(record, omf_comdat_flags | (segment->symbol.field04 != 0));
        value = omf_comdat_selection(segment->object_attributes.omf_comdat.selection);
        OMF_RECORD_BYTE(record, value);
        OMF_RECORD_BYTE(record, omf_alignment(segment->alignment));
        value = segment->symbol.field04;
        OMF_RECORD_BYTE(record, value);
        OMF_RECORD_BYTE(record, value >> 8);
        OMF_RECORD_BYTE(record, value >> 16);
        OMF_RECORD_BYTE(record, value >> 24);
        omf_write_index(record, (short)((FunctionSymbol *)segment->identity.owner)->field20);
        parent = segment->parent;
        omf_write_index(record, parent->symbol.field18.group->number);
        omf_write_index(record, parent->identity.order);
        omf_write_index(record, segment->object_section);
        break;
    case 0xc5:
        value = segment->object_section;
        OMF_RECORD_BYTE(record, value == omf_last_comdat_name);
        omf_last_comdat_name = value;
        omf_write_index(record, value);
        break;
    }
    record->count = 0;
    return record;
}
#undef OMF_RECORD_BYTE
/* Original [0006c4e0,0006c540). Long counted strings warn and truncate the
   caller's mutable source, then pass the same pointer to the byte writer. */
void __fastcall omf_append_string(OmfRecord *record, char *text)
{
    char *end;
    int length = 0;
    if (text) {
        end = text;
        while (*end++) ;
        length = (int)(end - text) - 1;
    }
    if (length > 255) {
        compiler_warning(1, 0x312, 255);
        length = 255;
        text[255] = 0;
    }
    ++record->count;
    *record->cursor = (unsigned char)length;
    ++record->cursor;
    omf_append_bytes(record, text, length);
}
/* Original [0006b650,0006b69b). The capacity comparison is signed32-bit;
   the copy count is the original unsigned decrement-to-zero sequence. */
void __fastcall omf_append_bytes(OmfRecord *record, char *bytes, unsigned count)
{
    long remaining = (long)((unsigned)record - (unsigned)record->cursor + 998);
    if (remaining < (long)count) compiler_internal_error("@(#)omf.c:1.10", 0x727);
    while (count--) {
        ++record->count;
        *record->cursor = *bytes++;
        ++record->cursor;
    }
}
/* Original [0006a7f0,0006a7fb): load the current slot and tail-transfer to
   its writer. Preserve the child's EAX pointer even where callers ignore it. */
OmfRecord *omf_flush_records(void)
{
    return omf_flush_record(omf_current_record);
}
/* Original [0006c330,0006c3f5). Dependency flushes precede writing this
   record. Keep its original type across those calls and clear only its tag. */
OmfRecord *__fastcall omf_flush_record(OmfRecord *record)
{
    unsigned type = record->type;
    if (!type) return record;
    if (type == 0xa3 || type == 0xa1 || type == 0xc3) {
        if (!omf_flush_record_type(0xb0) && !omf_flush_record_type(0x8c) &&
            !omf_flush_record_type(0xb4)) omf_flush_record_type(0xbc);
        omf_flush_record_type(0x91);
        omf_flush_record_type(0xb7);
    }
    if (type == 0xbc && !omf_flush_record_type(0x96)) omf_flush_record_type(0xca);
    omf_write_record(record);
    record->type = 0;
    if ((type == 0xa1 || type == 0xc3) && omf_fixup_count) {
        omf_write_fixups();
        omf_flush_records();
    }
    return record;
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
        if (segment->combine == 7) {
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
        if (segment->combine != 7 && !segment->omf_header_end) {
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

/* Original [0006c440,0006c4d7). Emit little-endian record length and the
   additive8-bit checksum; the cursor remains at payload start after write. */
void __fastcall omf_write_record(OmfRecord *record)
{
    unsigned char *end = record->cursor;
    unsigned size, index, length;
    unsigned char checksum = 0;
    if ((long)((unsigned)record - (unsigned)end + 998) < 0)
        compiler_internal_error("@(#)omf.c:1.10", 0x520);
    size = (unsigned)(end - record->bytes);
    length = size - 2;
    record->cursor = record->bytes + 1;
    ++record->count;
    *record->cursor = (unsigned char)length;
    ++record->cursor;
    ++record->count;
    *record->cursor = (unsigned char)(length >> 8);
    ++record->cursor;
    for (index = 0; index < size; ++index) checksum += record->bytes[index];
    record->bytes[size] = (unsigned char)-checksum;
    ++size;
    if (fwrite(record->bytes, 1, size, object_file) != size)
        report_context_error(0x55, generated_source, object_filename);
    record->count = 0;
}
/* Original [0006b860,0006b912), dispatch data [0006b914,0006b940).
   Unsupported directive ids call the original internal-error boundary. That
   path has no defined class if the fatal diagnostic were ever to return;
   retain the original assumption rather than inventing a default class. */
void __fastcall omf_output_record(ModuleDirective *directive)
{
    unsigned kind;
    OmfRecord *record;
    char *text;
    switch (directive->kind) {
    case 1:
        directive->text = compiler_identification;
        kind = 0xdb;
        break;
    case 2: kind = 0xdc; break;
    case 3: kind = 0xa4; break;
    case 4: kind = 0x9f; break;
    case 5: kind = 0xdd; break;
    case 6: kind = 0xdf; break;
    case 7:
        record = omf_begin_record(0x88, NULL);
        text = directive->text;
        omf_append_bytes(record, text + 1, (int)*(signed char *)text);
        return;
    case 8: kind = 0; break;
    case 9: kind = 0x9d; break;
    case 10: kind = 0xa0; break;
    case 11: kind = 0xa1; break;
    default:
        compiler_internal_error("@(#)omf.c:1.10", 0x294);
        break;
    }
    omf_write_comment((unsigned char)kind, directive->text);
}
/* Original [0006b940,0006b98e). COMENT records begin with zero attributes
   and the low-byte comment class, followed by the uncounted string bytes. */
void __fastcall omf_write_comment(unsigned char kind, char *text)
{
    OmfRecord *record = omf_begin_record(0x88, NULL);
    char *end;
    ++record->count;
    *record->cursor = 0;
    ++record->cursor;
    ++record->count;
    *record->cursor = kind;
    ++record->cursor;
    end = text;
    while (*end++) ;
    omf_append_bytes(record, text, (unsigned)(end - text) - 1);
}
/* Original [0006a940,0006a971). The interner always writes the returned
   index; only a newly assigned name is appended to an LNAMES record. */
unsigned __fastcall omf_name_index(char *name)
{
    unsigned index;
    OmfRecord *record;
    if (!omf_intern_name(name, &index)) {
        record = omf_begin_record(0x96, NULL);
        omf_append_string(record, name);
    }
    return index;
}
/* Original [0006c540,0006c5ff). Equality is inlined in the original;
   keep the byte comparison here rather than adding a strcmp call edge. */
int __fastcall omf_intern_name(char *name, unsigned *index)
{
    int hash = 0;
    unsigned i;
    OmfName **bucket, *entry;
    unsigned char *left, *right;
    for (i = 0; i < 5 && name[i]; ++i) hash += (signed char)name[i];
    bucket = &omf_name_buckets[hash & 15];
    for (entry = *bucket; entry; entry = entry->next) {
        left = (unsigned char *)name;
        right = (unsigned char *)entry->name;
        while (*left == *right && *left) { ++left; ++right; }
        if (*left == *right) {
            *index = entry->index;
            return 1;
        }
    }
    entry = arena_alloc(7, sizeof(*entry));
    entry->name = arena_strdup(name, 7);
    entry->index = omf_next_name++;
    entry->next = *bucket;
    *bucket = entry;
    *index = entry->index;
    return 0;
}
/* Original [0006a890,0006a940). SEGDEF32 attributes are formed in a byte;
   cached count/cursor values survive the alignment call just as in the original. */
void __fastcall omf_write_segment_definition(Segment *segment)
{
    OmfRecord *record;
    unsigned count, value;
    unsigned char *cursor, attributes;
    if (segment->identity.order != omf_next_segment)
        compiler_internal_error("@(#)omf.c:1.10", 0x5b2);
    ++omf_next_segment;
    record = omf_begin_record(0x99, NULL);
    count = record->count;
    record->count = ++count;
    cursor = record->cursor;
    attributes = (unsigned char)omf_alignment(segment->alignment);
    attributes = (unsigned char)((attributes << 3) | segment->combine);
    attributes = (unsigned char)((attributes << 2) | 1);
    *cursor++ = attributes;
    record->cursor = cursor;
    value = segment->symbol.field04;
    record->count = ++count; *cursor++ = (unsigned char)value; record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(value >> 8); record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(value >> 16); record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(value >> 24); record->cursor = cursor;
    omf_write_index(record, segment->object_section);
    omf_write_index(record, segment->object_attributes.omf_class_index);
    omf_write_index(record, omf_empty_name);
}

/* Original [0006a310,0006a346). Only these four alignment values have
   defined results. The original default RET does not assign EAX: preserve the
   Win32 missing return rather than initialize a made-up result. Native callers
   outside this proved producer contract trap explicitly; a real such case
   requires its incoming-register ABI to be recovered before proceeding. */
unsigned __fastcall omf_alignment(unsigned alignment)
{
    switch (alignment) {
    case 1: return 1;
    case 2: return 2;
    case 4: return 5;
    case 16: return 3;
    }
#ifdef RECON_NATIVE_TEST
    __asm int 3
#endif
}

/* Original [0006c2b0,0006c2ee). The original comparison is signed:
   negative indices take the one-byte path; the two-byte path truncates each
   encoded byte without an invented range or capacity check. */
void __fastcall omf_write_index(OmfRecord *record, int index)
{
    unsigned char *cursor;
    unsigned count;
    if (index < 128) {
        cursor = record->cursor;
        ++record->count;
        *cursor++ = (unsigned char)index;
        record->cursor = cursor;
    } else {
        count = record->count + 1;
        record->count = count;
        cursor = record->cursor;
        *cursor++ = (unsigned char)((index >> 8) | 0x80);
        record->cursor = cursor;
        record->count = ++count;
        *cursor++ = (unsigned char)index;
        record->cursor = cursor;
    }
}

/* Original [0006a850,0006a884). Set the context's group before record lookup;
   the DOS flat-group mode does not append explicit segment members. */
void __fastcall omf_write_group_definition(Segment *segment, OmfGroup *group)
{
    OmfRecord *record;
    unsigned char *cursor;
    segment->symbol.field18.group = group;
    record = omf_begin_record(0x9a, segment);
    if (!dos_object) {
        cursor = record->cursor;
        ++record->count;
        *cursor++ = 0xff;
        record->cursor = cursor;
        omf_write_index(record, segment->identity.order);
    }
}

/* Original [0006a800,0006a843). The wire tag truncates method/thread to a
   byte, while the caller receives the complete thread number in EAX. */
unsigned __fastcall omf_define_fixup_thread(unsigned char flags, unsigned char method,
    unsigned target, unsigned thread)
{
    OmfRecord *record = omf_begin_record(0x9d, NULL);
    unsigned char *cursor;
    ++record->count;
    cursor = record->cursor;
    *cursor++ = (unsigned char)((method << 2) | (unsigned char)thread | flags);
    record->cursor = cursor;
    omf_write_index(record, (int)target);
    return thread;
}

/* Original [0006b6a0,0006b6bf). The byte subtract/borrow sequence selects
   COMDAT32 exactly for combine7; ordinary segments use LEDATA32. */
void __fastcall omf_select_segment(Segment *segment)
{
    omf_current_record = omf_begin_record(segment->combine == 7 ? 0xc3 : 0xa1, segment);
}

/* Original [0006b6c0,0006b6e4). Keep the source/count through record
   renewal and publish the returned slot before appending the bytes. */
void __fastcall omf_output_data(void *data, unsigned bytes)
{
    omf_current_record = omf_reserve_bytes(omf_current_record, bytes);
    omf_append_bytes(omf_current_record, data, bytes);
}

/* Original [0006b6f0,0006b70d). Equality also renews the record; both sides
   of the capacity comparison use their original signed32 interpretation. */
OmfRecord *__fastcall omf_reserve_bytes(OmfRecord *record, unsigned bytes)
{
    if ((long)((unsigned)record - (unsigned)record->cursor + 998) <= (int)bytes)
        return omf_initialize_record(record, record->type, record->context);
    return record;
}

/* Original [0006ab20,0006acb7). OMF only marks queued, not-yet-emitted
   symbols. COMDATs refer to their segment name; ordinary variable/function
   storage classes choose public, external, local-external or communal records. */
void __fastcall omf_mark_symbol(SymbolHeader *symbol)
{
    BackendSymbol *backend = (BackendSymbol *)symbol;
    Segment *segment;
    unsigned storage;
    if (!(backend->output_flags & 4) || (backend->output_flags & 1)) return;
    backend->output_flags |= 1;
    if (symbol->kind == 1) {
        if (symbol->field0e & 0x20) {
            segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
            if (!segment->object_section) {
                if ((backend->attributes & 0x780) == 0x200) {
                    segment->object_attributes.omf_comdat.flags |= 4;
                    segment->object_section = omf_local_name_index(symbol_display_name(symbol));
                } else segment->object_section = omf_name_index(symbol_display_name(symbol));
            }
            omf_emit_comdat_external(symbol);
        } else {
            storage = backend->attributes;
            if (((storage & 0x780) >> 7) == 1) {
                segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
                omf_emit_public(symbol, segment);
            } else if (((storage & 0x780) >> 7) == 2)
                omf_emit_external(symbol);
            else if (((storage & 0x780) >> 7) == 3) {
                if ((storage & 0x70) == 0x40) {
                    segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
                    omf_emit_public(symbol, segment);
                } else omf_emit_communal(symbol);
            }
        }
    } else if (symbol->kind == 4) {
        if (symbol->field0e & 0x20) {
            segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
            if (!segment->object_section) {
                if ((backend->attributes & 0xf0) == 0x50)
                    segment->object_section = omf_name_index(symbol_display_name(symbol));
                else {
                    segment->object_attributes.omf_comdat.flags |= 4;
                    segment->object_section = omf_local_name_index(symbol_display_name(symbol));
                }
            }
            omf_emit_comdat_external(symbol);
        } else {
            storage = (backend->attributes & 0xf0) >> 4;
            if (storage == 2 || storage == 3) omf_emit_local_external(symbol);
            else if (storage == 4 || storage == 5) {
                omf_emit_external(symbol);
                if (backend->attributes & 0x8000)
                    omf_emit_weak_external((FunctionSymbol *)symbol);
            }
        }
    }
}

/* Original [0006aef0,0006afbe). Offset is cached before all children,
   but the type WORD is read after name/prefix output. The original assumes
   kind1/3/4: other kinds use an uninitialized local record type. Preserve that
   Win32 source assumption; unsupported native kinds trap rather than acquire
   a guessed type. A real such input holds for contract/ABI review. */
void __fastcall omf_emit_public(SymbolHeader *symbol, Segment *segment)
{
    unsigned type, value = symbol->field04, count;
    OmfRecord *record;
    unsigned char *cursor;
    if (symbol->kind == 1) type = 0x91;
    else if (symbol->kind == 3) {
        if (((LabelSymbol *)symbol)->label_kind != 4)
            compiler_internal_error("@(#)omf.c:1.10", 0x638);
        type = 0xb7;
    } else if (symbol->kind == 4)
        type = (((FunctionSymbol *)symbol)->field24 & 0xf0) == 0x50 ? 0x91 : 0xb7;
#ifdef RECON_NATIVE_TEST
    else { __asm int 3 }
#endif
    record = omf_begin_record(type, segment);
    omf_append_string(record, symbol_display_name(symbol));
    count = record->count + 1;
    cursor = record->cursor;
    record->count = count; *cursor++ = (unsigned char)value; record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(value >> 8); record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(value >> 16); record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(value >> 24); record->cursor = cursor;
    omf_write_index(record, (short)((BackendSymbol *)symbol)->field20);
}

/* Original [0006ad80,0006adc0). EXTDEF assigns the index after record
   lookup and before name formatting. The zero type index is one raw byte. */
void __fastcall omf_emit_external(SymbolHeader *symbol)
{
    OmfRecord *record = omf_begin_record(0x8c, NULL);
    unsigned index;
    unsigned char *cursor;
    index = omf_next_external;
    symbol->field14 = index;
    omf_next_external = index + 1;
    omf_append_string(record, symbol_display_name(symbol));
    cursor = record->cursor;
    ++record->count;
    *cursor++ = 0;
    record->cursor = cursor;
}

/* Original [0006afc0,0006b1a9), eight dispatch DWORDs [6b1ac,6b1cc).
   Keep the entry kind across record lookup, but reload the active operands
   afterwards. Symbol is an unused formal in the original callback ABI. */
unsigned __fastcall omf_output_relocation(Initializer *initializer,
    SymbolHeader *symbol, Segment *segment)
{
    int kind = initializer->kind, width, divisor, remainder;
    unsigned bytes = (unsigned)-1, repeat, value, count, zero;
    OmfRecord *record = NULL;
    SymbolHeader *referenced;
    unsigned char *cursor;
    (void)symbol;
    if ((kind != 1 || initializer->repeat == 1) &&
        (kind != 1 || initializer->content.datum.type != 6) &&
        (kind != 8 || initializer->content.argument < 20))
        record = omf_begin_record(segment->combine == 7 ? 0xc3 : 0xa1, segment);
    switch (kind) {
    case 1:
        if (initializer->content.datum.type == 6) {
            width = initializer->content.datum.bytes;
            repeat = initializer->repeat;
            bytes = (unsigned)width * repeat;
            omf_write_byte_array(segment, (unsigned)width, repeat,
                initializer->content.datum.value.bytes);
        } else if ((int)initializer->repeat > 1) {
            repeat = initializer->repeat;
            width = initializer->content.datum.bytes;
            bytes = (unsigned)width * repeat;
            omf_write_iterated_data(segment, (int)repeat, (unsigned)width,
                &initializer->content.datum.value);
        } else bytes = omf_write_datum(record, initializer);
        break;
    case 2:
        bytes = (unsigned)(int)(signed char)initializer->content.relocation.flags;
        referenced = lookup_global_symbol(initializer->content.relocation.symbol_id);
        if (referenced->kind == 1) value = initializer->content.relocation.offset;
        else if (referenced->kind == 3) {
            if (((LabelSymbol *)referenced)->label_kind != 4)
                compiler_internal_error("@(#)omf.c:1.10", 0x1c8);
            value = 0;
        } else if (referenced->kind == 4) value = 0;
        else compiler_internal_error("@(#)omf.c:1.10", 0x1da);
        count = record->count + 1;
        cursor = record->cursor;
        record->count = count; *cursor++ = (unsigned char)value; record->cursor = cursor;
        record->count = ++count; *cursor++ = (unsigned char)(value >> 8); record->cursor = cursor;
        record->count = ++count; *cursor++ = (unsigned char)(value >> 16); record->cursor = cursor;
        record->count = ++count; *cursor++ = (unsigned char)(value >> 24); record->cursor = cursor;
        omf_output_bytes((unsigned)-4, referenced);
        break;
    case 3:
    case 5:
        break;
    case 4:
        count = record->count;
        divisor = (signed char)initializer->content.byte_argument;
        remainder = (int)(record->context_offset + count) % divisor;
        bytes = (unsigned)remainder;
        if (remainder) {
            record->count = count + 1;
            cursor = record->cursor;
            *cursor++ = 0;
            record->cursor = cursor;
        }
        break;
    case 8:
        bytes = initializer->content.argument;
        if ((int)bytes < 20) omf_pad_record(record, bytes);
        else {
            zero = 0;
            omf_write_iterated_data(segment, (int)bytes, 1, &zero);
        }
        break;
    default:
        compiler_internal_error("@(#)omf.c:1.10", 0x1fe);
        break;
    }
    return bytes;
}

/* Original [0006b3d0,0006b47c). Split each repeated pattern between record
   capacity and the remaining signed-offset domain; restore the caller's
   segment position after emitting all copies. A null source requests zeroes. */
void __fastcall omf_write_byte_array(Segment *segment, unsigned bytes,
    unsigned repeat, void *data)
{
    unsigned pattern_bytes = bytes, position = segment->symbol.field04;
    unsigned remaining = 0x7fffffff - position, count, space;
    char *source = data;
    OmfRecord *record;
    do {
        record = omf_begin_record(segment->combine == 7 ? 0xc3 : 0xa1, segment);
        count = remaining;
        if (count > bytes) count = bytes;
        space = (unsigned)((char *)record + 998 - (char *)record->cursor);
        if (count > space) count = space;
        if (source) {
            omf_append_bytes(record, source, count);
            source += count;
        } else omf_pad_record(record, count);
        bytes -= count;
        segment->symbol.field04 += count;
        remaining -= count;
        if (!bytes) {
            if (source) source -= pattern_bytes;
            bytes = pattern_bytes;
            --repeat;
        }
    } while (repeat);
    segment->symbol.field04 = position;
}

/* Original [0006b2a0,0006b2bc). No reserve/flush: the caller provides space. */
void __fastcall omf_pad_record(OmfRecord *record, unsigned bytes)
{
    unsigned char *cursor;
    while (bytes--) {
        cursor = record->cursor;
        ++record->count;
        *cursor++ = 0;
        record->cursor = cursor;
    }
}

/* Original [0006b2d0,0006b3c5). Integer widths are emitted little-endian;
   floating bit patterns are copied unchanged. Return the width reloaded after
   the append child, since that child is an observable call boundary. */
unsigned __fastcall omf_write_datum(OmfRecord *record, Initializer *initializer)
{
    unsigned value, count;
    unsigned char *cursor;
    switch (initializer->content.datum.bytes) {
    case 1:
        ++record->count;
        cursor = record->cursor;
        *cursor++ = initializer->content.datum.value.byte;
        record->cursor = cursor;
        break;
    case 2:
        value = (unsigned short)initializer->content.datum.value.word;
        count = record->count + 1;
        cursor = record->cursor;
        record->count = count; *cursor++ = (unsigned char)value; record->cursor = cursor;
        record->count = ++count; *cursor++ = (unsigned char)(value >> 8); record->cursor = cursor;
        break;
    case 4:
        if (initializer->content.datum.type == 5)
            omf_append_bytes(record, (char *)&initializer->content.datum.value, 4);
        else {
            value = initializer->content.datum.value.integer;
            count = record->count + 1;
            cursor = record->cursor;
            record->count = count; *cursor++ = (unsigned char)value; record->cursor = cursor;
            record->count = ++count; *cursor++ = (unsigned char)(value >> 8); record->cursor = cursor;
            record->count = ++count; *cursor++ = (unsigned char)(value >> 16); record->cursor = cursor;
            record->count = ++count; *cursor++ = (unsigned char)(value >> 24); record->cursor = cursor;
        }
        break;
    case 8:
        omf_append_bytes(record, (char *)&initializer->content.datum.value, 8);
        break;
    case 10:
        omf_append_bytes(record, (char *)&initializer->content.datum.value, 10);
        break;
    default:
        compiler_internal_error("@(#)omf.c:1.10", 0x242);
        break;
    }
    return initializer->content.datum.bytes;
}

/* Original [0006b2c0,0006b2d0). Absolute32 reference adapter. */
void __fastcall omf_output_bytes(unsigned displacement, SymbolHeader *symbol)
{
    omf_add_fixup(symbol, 9, displacement, 0);
}

/* Original [0006b990,0006bcd8). Old entry flags are deliberately preserved
   except for the named bits. Each branch keeps its original cached-versus-
   reloaded flag ordering across lookup calls. No entry-wide initialization. */
void __fastcall omf_add_fixup(SymbolHeader *symbol, unsigned char type,
    unsigned displacement, int relative)
{
    OmfFixup *fixup = omf_allocate_fixup();
    unsigned flags, storage, section;
    unsigned short symbol_flags;
    unsigned char frame;
    Segment *segment;
    fixup->type = type;
    fixup->location = (unsigned short)(omf_current_record->count + displacement);
    flags = (fixup->flags & ~OMF_FIXUP_SEGMENT_RELATIVE) | (relative == 0);
    flags |= OMF_FIXUP_FRAME_THREAD;
    fixup->flags = flags;
    frame = (unsigned char)omf_fixup_threads.group_frame;
    fixup->frame_code = frame;
    fixup->frame_datum = 0;
    switch (symbol->kind) {
    case 1:
        symbol_flags = symbol->field0e;
        storage = symbol_flags & 0x20 ? 2 :
            (((BackendSymbol *)symbol)->attributes & 0x780) >> 7;
        if (storage == 1 || storage == 4) {
            section = (((BackendSymbol *)symbol)->attributes & 0x70) >> 4;
            switch (section) {
            case 2:
                if (symbol_flags & 0x200) {
                    if (symbol_flags & 0x20)
                        compiler_internal_error("@(#)omf.c:1.10", 0x33e);
                    segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
                    fixup->flags = flags & ~OMF_FIXUP_TARGET_THREAD;
                    fixup->target_code = 4;
                    fixup->target_datum = (unsigned short)segment->identity.order;
                } else {
                    segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
                    if (segment->identity.order != 2)
                        compiler_internal_error("@(#)omf.c:1.10", 0x358);
                    fixup->flags = flags | OMF_FIXUP_TARGET_THREAD;
                    fixup->target_code = (unsigned char)omf_fixup_threads.data_target;
                }
                break;
            case 4:
                if (symbol_flags & 0x200) {
                    if (symbol_flags & 0x20)
                        compiler_internal_error("@(#)omf.c:1.10", 0x315);
                    segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
                    fixup->flags = flags & ~OMF_FIXUP_TARGET_THREAD;
                    fixup->target_code = 4;
                    fixup->target_datum = (unsigned short)segment->identity.order;
                } else {
                    segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
                    if (segment->identity.order != 3)
                        compiler_internal_error("@(#)omf.c:1.10", 0x322);
                    fixup->flags = flags | OMF_FIXUP_TARGET_THREAD;
                    fixup->target_code = (unsigned char)omf_fixup_threads.const_target;
                }
                break;
            case 6:
                segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
                if (segment->identity.order != 4)
                    compiler_internal_error("@(#)omf.c:1.10", 0x336);
                fixup->flags = flags | OMF_FIXUP_TARGET_THREAD;
                fixup->target_code = (unsigned char)omf_fixup_threads.bss_target;
                break;
            default:
                compiler_internal_error("@(#)omf.c:1.10", 0x35e);
                break;
            }
            fixup->flags &= ~OMF_FIXUP_NO_ADDEND;
            fixup->addend = symbol->field04;
        } else if (storage == 2 || storage == 3) {
            if (!dos_object) {
                fixup->flags = flags & ~OMF_FIXUP_FRAME_THREAD;
                fixup->frame_code = 5;
            }
            fixup->flags &= ~OMF_FIXUP_TARGET_THREAD;
            fixup->target_code = 6;
            fixup->target_datum = (unsigned short)symbol->field14;
            fixup->flags |= OMF_FIXUP_NO_ADDEND;
        } else compiler_internal_error("@(#)omf.c:1.10", 0x368);
        break;
    case 3:
        if (dos_object) {
            fixup->flags = flags | OMF_FIXUP_FRAME_THREAD;
            fixup->frame_code = frame;
        } else {
            fixup->flags = flags & ~OMF_FIXUP_FRAME_THREAD;
            fixup->frame_code = 5;
        }
        if (current_symbol->symbol.field0e & 0x20) {
            fixup->flags &= ~OMF_FIXUP_TARGET_THREAD;
            fixup->target_code = 6;
            fixup->target_datum = (unsigned short)current_symbol->symbol.field14;
            fixup->flags |= OMF_FIXUP_NO_ADDEND;
        } else if (current_symbol->symbol.field0e & 0x200) {
            flags = fixup->flags & ~OMF_FIXUP_TARGET_THREAD;
            fixup->flags = flags;
            fixup->target_code = 4;
            segment = (Segment *)lookup_global_symbol(current_symbol->symbol.field18.identifier);
            fixup->target_datum = (unsigned short)segment->identity.order;
            fixup->flags = flags & ~OMF_FIXUP_NO_ADDEND;
            fixup->addend = symbol->field04;
        } else {
            flags = fixup->flags | OMF_FIXUP_TARGET_THREAD;
            fixup->flags = flags;
            fixup->target_code = (unsigned char)omf_fixup_threads.code_target;
            fixup->flags = flags & ~OMF_FIXUP_NO_ADDEND;
            fixup->addend = symbol->field04;
        }
        break;
    case 4:
        if (dos_object) {
            fixup->flags = flags | OMF_FIXUP_FRAME_THREAD;
            fixup->frame_code = frame;
        } else {
            fixup->flags = flags & ~OMF_FIXUP_FRAME_THREAD;
            fixup->frame_code = 5;
        }
        fixup->flags &= ~OMF_FIXUP_TARGET_THREAD;
        fixup->target_code = 6;
        fixup->target_datum = (unsigned short)symbol->field14;
        fixup->flags |= OMF_FIXUP_NO_ADDEND;
        if ((((BackendSymbol *)symbol)->attributes & 0xf0) == 0x20)
            compiler_error(0x81, format_function_name(symbol));
        break;
    default:
        compiler_internal_error("@(#)omf.c:1.10", 0x3a5);
        break;
    }
}

/* Original [0006bd10,0006bd46). The known counter lifecycle is zero/reset
   then increment; the signed upper-bound diagnostic precedes slot selection. */
OmfFixup *omf_allocate_fixup(void)
{
    int index = omf_fixup_count;
    OmfFixup *fixup;
    if (index >= 250) {
        compiler_internal_error("@(#)omf.c:1.10", 0x772);
        index = omf_fixup_count;
    }
    fixup = &omf_fixups[index];
    omf_fixup_count = index + 1;
    return fixup;
}

/* Original [0006adc0,0006ae00). LEXTDEF shares the external index domain,
   but its distinct entry/call graph is retained. Old field14 is write-only. */
void __fastcall omf_emit_local_external(SymbolHeader *symbol)
{
    OmfRecord *record = omf_begin_record(0xb4, NULL);
    unsigned index;
    unsigned char *cursor;
    index = omf_next_external;
    symbol->field14 = index;
    omf_next_external = index + 1;
    omf_append_string(record, symbol_display_name(symbol));
    cursor = record->cursor;
    ++record->count;
    *cursor++ = 0;
    record->cursor = cursor;
}

/* Original [0006b730,0006b735): filename callback tail-forwards to THEADR.
   The accepted header writer receives only ECX, with no return-address input. */
void __fastcall omf_output_debug_pair(char *filename)
{
    omf_write_module_header(filename);
}

/* Original [0006c400,0006c437). Callback may change the pending count,
   so reload the signed loop bound after each individually serialized entry. */
void omf_write_fixups(void)
{
    int index;
    OmfFixup *fixup;
    if (omf_fixup_count) {
        for (index = 0, fixup = omf_fixups; index < omf_fixup_count;
             ++index, ++fixup)
            omf_write_fixup(fixup);
        omf_fixup_count = 0;
    }
}

/* Original [0006c680,0006c7b2). FIXUPP32 entry. Preserve cached flags
   across index children, but reload their later datum/addend and record state. */
void __fastcall omf_write_fixup(OmfFixup *fixup)
{
    unsigned flags = fixup->flags, location, count, addend;
    unsigned char fix_data, *cursor;
    OmfRecord *record;
    if (!(flags & OMF_FIXUP_NO_ADDEND) && !fixup->addend)
        fixup->flags = flags | OMF_FIXUP_NO_ADDEND;
    record = omf_begin_record(0x9d, NULL);
    location = ((fixup->location & 0xff) << 8) |
        ((fixup->location & 0x300) >> 8) | 0x80;
    flags = fixup->flags;
    location |= (flags & OMF_FIXUP_SEGMENT_RELATIVE) << 6;
    location |= (unsigned)(signed char)fixup->type << 2;
    count = record->count + 1;
    cursor = record->cursor;
    record->count = count; *cursor++ = (unsigned char)location; record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(location >> 8); record->cursor = cursor;
    fix_data = (unsigned char)(((flags >> 1) << 7) |
        (fixup->frame_code << 4) | (((flags >> 2) & 1) << 3) |
        (((flags >> 3) & 1) << 2));
    fix_data += fixup->target_code & 3;
    record->count = ++count; *cursor++ = fix_data; record->cursor = cursor;
    if (!(flags & OMF_FIXUP_FRAME_THREAD) && fixup->frame_code <= 2)
        omf_write_index(record, fixup->frame_datum);
    if (!(flags & OMF_FIXUP_TARGET_THREAD))
        omf_write_index(record, fixup->target_datum);
    if (!(flags & OMF_FIXUP_NO_ADDEND)) {
        addend = fixup->addend;
        count = record->count + 1;
        cursor = record->cursor;
        record->count = count; *cursor++ = (unsigned char)addend; record->cursor = cursor;
        record->count = ++count; *cursor++ = (unsigned char)(addend >> 8); record->cursor = cursor;
        record->count = ++count; *cursor++ = (unsigned char)(addend >> 16); record->cursor = cursor;
        record->count = ++count; *cursor++ = (unsigned char)(addend >> 24); record->cursor = cursor;
    }
}

/* Original [0006b710,0006b72c). Non-COMDAT function public declaration. */
void __fastcall omf_output_symbol(FunctionSymbol *symbol)
{
    Segment *segment;
    if (!(symbol->symbol.field0e & 0x20)) {
        segment = (Segment *)lookup_global_symbol(symbol->symbol.field18.identifier);
        omf_emit_public(&symbol->symbol, segment);
    }
}

/* Original [0006b740,0006b780). Look up the current segment first, then
   reload current_symbol before choosing the ordinary or COMDAT line writer. */
void __fastcall omf_output_debug_record(unsigned line, unsigned offset)
{
    Segment *segment = (Segment *)lookup_global_symbol(current_symbol->symbol.field18.identifier);
    if (current_symbol->symbol.field0e & 0x20)
        omf_write_comdat_line((unsigned short)line, offset, segment);
    else
        omf_write_line((unsigned short)line, offset, segment);
}

/* Original [0006b780,0006b7ee). LINNUM32 contains the source WORD followed
   by the four-byte offset, with the segment supplied only as record context. */
void __fastcall omf_write_line(unsigned short line, unsigned offset, Segment *segment)
{
    OmfRecord *record = omf_begin_record(0x95, segment);
    unsigned count = record->count + 1;
    unsigned char *cursor = record->cursor;
    record->count = count; *cursor++ = (unsigned char)line; record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(line >> 8); record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)offset; record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(offset >> 8); record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(offset >> 16); record->cursor = cursor;
    record->count = ++count; *cursor++ = (unsigned char)(offset >> 24); record->cursor = cursor;
}

/* Original [0006bd00,0006bd10): CodeView section-reference fixup. */
void __fastcall omf_output_size(unsigned displacement, SymbolHeader *symbol)
{
    omf_add_fixup(symbol, 11, displacement, 0);
}

/* Original [0006bcf0,0006bd00): relative32 operand reference. */
void __fastcall omf_output_offset(unsigned displacement, SymbolHeader *symbol)
{
    omf_add_fixup(symbol, 9, displacement, 1);
}

/* Original [0006a9d0,0006aacd). Pending payloads precede rewriting the
   original segment headers. Preserve the end position for final MODEND. */
void omf_finish_module(void)
{
    long end;
    Segment *segment;
    omf_flush_record_type(0x91);
    if (!omf_flush_record_type(0xb0) && !omf_flush_record_type(0x8c) &&
        !omf_flush_record_type(0xb4)) omf_flush_record_type(0xbc);
    omf_flush_record_type(0xa3);
    omf_flush_record_type(0xa1);
    omf_flush_record_type(0xc3);
    omf_flush_record_type(0xc5);
    omf_flush_record_type(0x95);
    end = ftell(object_file);
    for (segment = segment_list; segment; segment = segment->segment_next) {
        if (segment->combine != 7) {
            fseek(object_file, segment->omf_header_end, SEEK_SET);
            omf_next_segment = segment->identity.order;
            omf_write_segment_definition(segment);
            omf_flush_records();
        }
    }
    fseek(object_file, end, SEEK_SET);
    omf_write_module_end();
    omf_flush_records();
    fclose(object_file);
    object_generation_started = 0;
}

/* Original [0006aad0,0006aaea). MODEND has one zero module-type byte. */
void omf_write_module_end(void)
{
    OmfRecord *record = omf_begin_record(0x8a, NULL);
    unsigned char *cursor = record->cursor;
    ++record->count;
    *cursor++ = 0;
    record->cursor = cursor;
}

/* Original [0006ae90,0006aee5). COMDEF shares the external index domain.
   Reload type after the name callbacks, and size after the index callback. */
void __fastcall omf_emit_communal(SymbolHeader *symbol)
{
    OmfRecord *record = omf_begin_record(0xb0, NULL);
    unsigned index = omf_next_external;
    unsigned char *cursor;
    symbol->field14 = index;
    omf_next_external = index + 1;
    omf_append_string(record, symbol_display_name(symbol));
    omf_write_index(record, (short)((BackendSymbol *)symbol)->field20);
    cursor = record->cursor;
    ++record->count;
    *cursor++ = 0x62;
    record->cursor = cursor;
    omf_write_communal_size(record, symbol->value);
}

/* Original [0006c600,0006c674). Signed size thresholds are deliberate;
   larger values carry three low bytes after84, not a four-byte integer. */
void __fastcall omf_write_communal_size(OmfRecord *record, int size)
{
    unsigned char *cursor = record->cursor;
    unsigned count = record->count;
    if (size <= 127) {
        ++record->count;
        *cursor++ = (unsigned char)size;
        record->cursor = cursor;
    } else {
        record->count = ++count;
        *cursor++ = size <= 65535 ? 0x81 : 0x84;
        record->cursor = cursor;
        record->count = ++count;
        *cursor++ = (unsigned char)size;
        record->cursor = cursor;
        record->count = ++count;
        *cursor++ = (unsigned char)(size >> 8);
        record->cursor = cursor;
        if (size > 65535) {
            record->count = ++count;
            *cursor++ = (unsigned char)(size >> 16);
            record->cursor = cursor;
        }
    }
}

/* Original [0006b1d0,0006b293). Even repetition counts use two adjacent
   copies of the pattern. Keep the signed shift and low-byte block length. */
void __fastcall omf_write_iterated_data(Segment *segment, int repeat,
    unsigned bytes, void *data)
{
    OmfRecord *record;
    unsigned count, index, shift = 0;
    unsigned char *cursor;
    if (segment->combine == 7) {
        segment->object_attributes.omf_comdat.flags |= 2;
        record = omf_begin_record(0xc3, segment);
    } else record = omf_begin_record(0xa3, segment);
    if (!(repeat & 1)) shift = 1;
    repeat >>= shift;
    count = record->count;
    cursor = record->cursor;
    for (index = 0; index < 4; ++index) {
        record->count = ++count;
        *cursor++ = (unsigned char)((unsigned)repeat >> (index * 8));
        record->cursor = cursor;
    }
    record->count = ++count; *cursor++ = 0; record->cursor = cursor;
    record->count = ++count; *cursor++ = 0; record->cursor = cursor;
    record->count = ++count;
    *cursor++ = (unsigned char)(bytes << shift);
    record->cursor = cursor;
    omf_append_bytes(record, data, bytes);
    if (shift) omf_append_bytes(record, data, bytes);
}
