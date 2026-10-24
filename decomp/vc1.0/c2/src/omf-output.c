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
void __fastcall omf_write_index(OmfRecord *record, int index);
unsigned char __fastcall omf_comdat_selection(unsigned char selection);
unsigned __fastcall omf_alignment(unsigned alignment);
void __fastcall omf_append_bytes(OmfRecord *record, char *bytes, unsigned count);
void __fastcall omf_write_record(OmfRecord *record);
void omf_write_fixups(void);
void __fastcall omf_write_comment(unsigned char kind, char *text);
int __fastcall omf_intern_name(char *name, unsigned *index);
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
unsigned omf_next_group = 1; /* 0008507c */
unsigned omf_next_segment = 1; /* 00085080 */
unsigned omf_next_frame_thread; /* 00085088 */
unsigned omf_next_target_thread; /* 0008508c */
unsigned char omf_comdat_flags; /* 00085090, cached COMDAT flags */
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
        omf_write_index(record, ((OmfGroup *)segment->symbol.field18.pointer)->number);
        omf_write_index(record, segment->identity.order);
        break;
    case 0x9a:
        OMF_RECORD_BYTE(record, ((OmfGroup *)segment->symbol.field18.pointer)->name_index);
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
        omf_write_index(record, ((OmfGroup *)parent->symbol.field18.pointer)->number);
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
