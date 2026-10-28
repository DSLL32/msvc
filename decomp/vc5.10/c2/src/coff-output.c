#include "coff-output.h"
#include <string.h>
#include <time.h>
#include <recon.h>

extern char *c2_output_name, *c2_source_name;
extern int deferred_compilation, c2_codeview_types, c2_line_numbers, c2_reproducible;
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
    writer->output_error = writer_callback_48511c;
    writer->output_allocator = allocate_output_bytes;
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

C2OutputAllocator output_allocate;
C2OutputErrorHandler output_error_handler;
C2OutputClockCallback output_clock_handler;
int coff_new_output;
unsigned short coff_machine;
char *coff_source_name;
FILE *coff_output;
long coff_pending04;
unsigned char coff_debug;
C2OutputTable *coff_table;
C2OutputStorage *coff_storage;
C2CoffRecord **coff_symbol_buffer;
unsigned coff_symbol_capacity, coff_symbol_total, coff_next_symbol;
unsigned short coff_default_flags, coff_flags, coff_pending_e18, coff_section_count;
C2OutputSection *coff_current_section;
int coff_pending_c90 = 1;
C2OutputList *coff_output_list, *coff_current_list;
C2OutputChunk *coff_free_chunks;
C2OutputBlock508 *coff_free_blocks_508;
C2OutputBlock308 *coff_free_blocks_308;
unsigned long coff_pending_499690;

int __fastcall initialize_coff_output(C2ObjectOutput *writer)
{
    if (writer->format != 1)
        return 0;
    coff_new_output = !deferred_compilation;
    coff_machine = writer->machine;
    coff_source_name = writer->source_name;
    coff_output = writer->file;
    coff_pending04 = writer->pending04;
    output_allocate = writer->output_allocator;
    output_error_handler = writer->output_error;
    output_clock_handler = writer->output_clock;
    coff_debug = writer->debug;
    coff_table = new_output_table(300);
    coff_storage = output_allocate(16);
    if (!deferred_compilation) {
        coff_symbol_buffer = malloc(4096);
        if (!coff_symbol_buffer)
            c2_report_internal("E:\\utc\\src\\\\P2\\cofflib.c", 807);
        coff_symbol_capacity = 1023;
    }
    coff_pending_e18 = 0;
    coff_section_count = 0;
    coff_flags = coff_default_flags;
    coff_current_section = NULL;
    coff_pending_c90 = 1;
    initialize_coff_lists();
    coff_output_list = new_output_list(1, 0);
    if (deferred_compilation) {
        if (!read_incremental_output()) {
            coff_symbol_buffer = malloc(4096);
            if (!coff_symbol_buffer)
                c2_report_internal("E:\\utc\\src\\\\P2\\cofflib.c", 838);
            coff_symbol_capacity = 1023;
        }
    }
    if (deferred_compilation)
        restore_incremental_output();
    else
        add_output_source(coff_source_name);
    return 1;
}

C2OutputTable *__fastcall new_output_table(unsigned short capacity)
{
    C2OutputTable *table;
    C2OutputPage *page;

    table = output_allocate(sizeof(*table));
    table->capacity = capacity;
    table->used = 0;
    table->count = 0;
    page = output_allocate(capacity * sizeof(void *) + sizeof(*page));
    table->first = page;
    page->items[0] = NULL;
    table->last = page;
    table->iterator_index = 0;
    table->iterator_used = 0;
    table->iterator_page = NULL;
    return table;
}

void *__fastcall allocate_output_bytes(unsigned bytes)
{
    return c2_pool_allocate(7, bytes);
}

void initialize_coff_lists(void)
{
    coff_pending_499690 = 0;
    coff_output_list = NULL;
    coff_current_list = NULL;
    coff_free_chunks = NULL;
    coff_free_blocks_508 = NULL;
    coff_free_blocks_308 = NULL;
}

C2OutputList *__fastcall new_output_list(int buffered, int count_only)
{
    C2OutputList *list;
    C2OutputChunk *chunk;

    list = output_allocate(sizeof(*list));
    chunk = new_output_chunk();
    chunk->next = chunk;
    list->count_only = count_only;
    list->last = chunk;
    list->buffered = buffered;
    list->completed_bytes = 0;
    list->used_bytes = 0;
    list->write_count = 0;
    return list;
}

C2OutputChunk *new_output_chunk(void)
{
    C2OutputChunk *chunk = coff_free_chunks;
    if (chunk) {
        coff_free_chunks = chunk->next;
        return chunk;
    }
    return output_allocate(508);
}

unsigned coff_reserved_symbols;
C2CoffSource *coff_previous_source;

void __fastcall add_output_source(char *name)
{
    C2CoffRecord *record, *auxiliary;
    C2CoffSource *source;
    unsigned count, index;

    record = output_allocate(sizeof(*record));
    record->kind = 1;
    source = output_allocate(sizeof(*source));
    record->value.source = source;
    source->name = name;
    source->next_source = 0;
    count = strlen(name) / 18 + 1;
    if (c2_internal_listing) {
        if (!coff_reserved_symbols)
            reserve_incremental_symbols(count + 1, 0, 0);
    }
    if (c2_internal_listing)
        index = add_incremental_symbol(record);
    else
        index = add_coff_symbol(record);
    if (coff_previous_source)
        coff_previous_source->next_source = index;
    coff_previous_source = source;
    auxiliary = output_allocate(sizeof(*auxiliary));
    auxiliary->kind = 5;
    while (count--) {
        if (c2_internal_listing)
            add_incremental_symbol(auxiliary);
        else
            add_coff_symbol(auxiliary);
    }
}

unsigned __fastcall add_coff_symbol(C2CoffRecord *record)
{
    unsigned index;
    index = coff_next_symbol;
    if (index > coff_symbol_capacity) {
        grow_coff_symbol_buffer();
        index = coff_next_symbol;
    }
    ++coff_symbol_total;
    coff_symbol_buffer[index] = record;
    coff_next_symbol = index + 1;
    return index;
}

void __fastcall write_coff_directive(char *prefix, char *value, char *suffix, int quoted)
{
    unsigned bytes;
    select_output_section(directive_section);
    bytes = strlen(prefix);
    write_output_bytes(prefix, bytes);
    directive_section->symbol.section_size += bytes;
    if (quoted) {
        write_output_byte('"');
        bytes = strlen(value);
        write_output_bytes(value, bytes);
        write_output_byte('"');
        bytes += 2;
    } else {
        bytes = strlen(value);
        write_output_bytes(value, bytes);
    }
    directive_section->symbol.section_size += bytes;
    if (suffix) {
        bytes = strlen(suffix);
        write_output_bytes(suffix, bytes);
        directive_section->symbol.section_size += bytes;
    }
    write_output_byte(' ');
    ++directive_section->symbol.section_size;
}

void __fastcall select_output_section(C2Section *section)
{
    unsigned short index;
    unsigned position;
    if (!section->coff_section)
        create_coff_section(section);
    index = (unsigned short)section->coff_section;
    select_coff_section(index);
    position = query_coff_position(index);
    if (position < section->symbol.section_size)
        write_output_fill(0, section->symbol.section_size - position);
}

/* Original48fb60 is c2_favor_speed, updated by the real bit23 decoder. */
extern int c2_favor_speed;

unsigned short __fastcall create_coff_section(C2Section *section)
{
    C2Section *base;
    char *name;
    unsigned flags;
    unsigned short association;
    int marked;
    if (section->coff_section)
        return (unsigned short)section->coff_section;
    base = section->combine == 7 ? section->parent : section;
    if (section->section_kind == 1 &&
        !(section->combine == 7 && section->selection == 5)) {
        name = base->symbol.name;
        if (!strncmp(name, ".rdata", 6) || !strncmp(name, ".xdata", 6) ||
            !strncmp(name, ".const", 6))
            section->section_kind = 4;
        else if (!strncmp(name, ".text", 5))
            section->section_kind = 0;
        else if (!strncmp(name, ".drectve", 8)) {
            section->section_kind = 10;
            section->alignment = directive_section->alignment;
        }
    }
    switch (section->section_kind) {
    case 0:
        name = base->symbol.serial == 5 ||
            (section->combine == 7 && section->selection == 5) ?
            ".text" : base->symbol.name;
        flags = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        section->alignment = c2_favor_speed ? stack_alignment :
            c2_constant_strings ? 2 : 1;
        break;
    case 1:
        name = base->symbol.serial == 2 ||
            (section->combine == 7 && section->selection == 5) ?
            ".data" : base->symbol.name;
        flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        break;
    case 3:
        name = base->symbol.serial == 4 ||
            (section->combine == 7 && section->selection == 5) ?
            ".bss" : base->symbol.name;
        flags = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        break;
    case 4:
        name = base->symbol.serial == 3 ||
            (section->combine == 7 && section->selection == 5) ?
            ".rdata" : base->symbol.name;
        flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
        break;
    case 9:
        name = section->symbol.name;
        flags = IMAGE_SCN_TYPE_NO_PAD | IMAGE_SCN_CNT_INITIALIZED_DATA |
            IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;
        break;
    case 10:
        name = section->symbol.name;
        flags = IMAGE_SCN_LNK_INFO | IMAGE_SCN_LNK_REMOVE;
        break;
    case 11:
        name = base->symbol.serial == 9 ||
            (section->combine == 7 && section->selection == 5) ?
            ".tls$" : base->symbol.name;
        flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        break;
    case 17:
        name = ".xdata$x";
        flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
        break;
    case 18:
        name = ".text$x";
        flags = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        if (c2_favor_speed)
            section->alignment = stack_alignment;
        break;
    case 19:
        name = base->symbol.serial == 7 ||
            (section->combine == 7 && section->selection == 5) ?
            ".debug$S" : base->symbol.name;
        flags = IMAGE_SCN_TYPE_NO_PAD | IMAGE_SCN_CNT_INITIALIZED_DATA |
            IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;
        break;
    case 20:
        if (base->symbol.serial == 8 ||
            (section->combine == 7 && section->selection == 5))
            name = coff_directive_option ? ".debug$P" : ".debug$T";
        else
            name = base->symbol.name;
        flags = IMAGE_SCN_TYPE_NO_PAD | IMAGE_SCN_CNT_INITIALIZED_DATA |
            IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ;
        break;
    default:
        return coff_default_flags;
    }
    marked = 0;
    if (c2_internal_listing && current_symbol &&
        (current_symbol->association.section_serial == section->symbol.serial ||
         (section->parent && current_symbol->association.section_serial ==
             section->parent->symbol.serial)))
        marked = 1;
    if (section->combine == 7) {
        association = section->selection == 5 ?
            (unsigned short)section->parent->coff_section : 0;
        section->coff_section = add_associative_coff_section(name,
            flags | IMAGE_SCN_LNK_COMDAT, coff_selection_code(section->selection),
            association, 1, marked);
    } else
        section->coff_section = add_coff_section(name, flags, 1, marked);
    if (marked)
        mark_debug_section(section->coff_section);
    initialize_coff_section((unsigned short)section->coff_section, flags);
    record_output_section(section);
    return (unsigned short)section->coff_section;
}

int coff_reuse_index = -1;
C2OutputSection *coff_reuse_sections[8];

unsigned short __fastcall add_coff_section(char *name, unsigned flags, int flag8, int reuse)
{
    C2OutputSection *section;
    if (reuse && coff_reuse_index >= 0 &&
        (section = coff_reuse_sections[coff_reuse_index])) {
        ++coff_reuse_index;
        if (flag8)
            section->flags |= 8;
        section->characteristics = flags;
        section->name = name;
        return section->index;
    }
    section = output_allocate(sizeof(*section));
    if (flag8)
        section->flags |= 8;
    section->characteristics = flags;
    section->index = (unsigned short)append_output_table(coff_table, section);
    section->name = name;
    return section->index;
}

unsigned __fastcall append_output_table(C2OutputTable *table, void *item)
{
    C2OutputPage *page, *last;
    if (table->used >= table->capacity) {
        page = output_allocate(table->capacity * sizeof(void *) + sizeof(C2OutputPage));
        last = table->last;
        table->last = page;
        table->used = 0;
        page->next = last->next;
        last->next = page;
    }
    table->last->items[table->used] = item;
    ++table->used;
    return ++table->count;
}

/* Original497da8 is the existing deferred_compilation control, also read
 * by initialize_coff_output. The older duplicate zero-only view is retired. */

int __fastcall initialize_coff_section(unsigned short section, unsigned flags)
{
    C2OutputSection *entry;
    C2CoffSectionData *data, *old;
    unsigned base;
    entry = lookup_output_table(coff_table, section);
    if (!entry)
        return 0;
    if (entry->flags & 2)
        return 0;
    if (entry->flags & 1) {
        if (entry->characteristics & IMAGE_SCN_LNK_COMDAT)
            return 0;
        if (coff_pending_e18)
            return 0;
    }
    if (coff_pending_c90 == 3) {
        if (coff_section_count >= 1)
            return 0;
    }
    old = entry->data;
    if (deferred_compilation) {
        if (!old)
            goto new_data;
        old->flags = 0;
        old->output = new_output_list(1, flags ==
            (IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE));
        old->relocations = output_allocate(508);
        old->relocations->count = 0;
        old->relocations->last = old->relocations;
        old->line_numbers = output_allocate(308);
        old->line_numbers->count = 0;
        old->line_numbers->last = old->line_numbers;
        initialize_coff_symbols(old);
        old->storage_index = (unsigned short)register_coff_storage(old);
        goto initialized;
    }
    if (old) {
        if (!(old->flags & 1))
            goto initialized;
    }
new_data:
    data = output_allocate(60);
    data->flags = 0;
    data->output = new_output_list(!(entry->flags & 4), flags ==
        (IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE));
    data->relocations = output_allocate(508);
    data->relocations->count = 0;
    data->relocations->last = data->relocations;
    data->line_numbers = output_allocate(308);
    data->line_numbers->count = 0;
    data->line_numbers->last = data->line_numbers;
    data->section = entry;
    if (old) {
        data->next = old->next;
        base = entry->base_offset + old->size;
        old->next = data;
        entry->data = data;
        entry->base_offset = base;
    } else {
        data->next = data;
        entry->data = data;
        entry->base_offset = 0;
    }
    initialize_coff_symbols(data);
    data->storage_index = (unsigned short)register_coff_storage(data);
initialized:
    entry->flags |= 3;
    ++coff_section_count;
    return 1;
}

void *__fastcall lookup_output_table(C2OutputTable *table, unsigned index)
{
    C2OutputPage *page;
    unsigned capacity, limit;
    if (index > table->count)
        return NULL;
    page = table->first;
    capacity = table->capacity;
    limit = capacity;
    while (index > limit) {
        if (!page)
            c2_report_internal("E:\\utc\\src\\\\P2\\coffemit.c", 3551);
        page = page->next;
        limit += capacity;
    }
    return page->items[index - 1 - (limit - capacity)];
}

void __fastcall initialize_coff_symbols(C2CoffSectionData *data)
{
    C2CoffRecord *record;
    unsigned index;
    if (c2_internal_listing) {
        if (!coff_reserved_symbols)
            reserve_incremental_symbols(2, 0, data->section->flags & 16);
    }
    record = output_allocate(sizeof(*record));
    record->kind = 2;
    record->value.section = data;
    if (c2_internal_listing)
        index = add_incremental_symbol(record);
    else
        index = add_coff_symbol(record);
    data->symbol_index = index;
    record = output_allocate(sizeof(*record));
    record->kind = 5;
    if (c2_internal_listing)
        add_incremental_symbol(record);
    else
        add_coff_symbol(record);
}

unsigned __fastcall register_coff_storage(C2CoffSectionData *data)
{
    C2OutputStorage *storage;
    C2OutputStorageEntry *entry, *first, *previous, *next, *empty;
    C2CoffSectionData *last;
    unsigned index = data->symbol_index;
    if (deferred_compilation) {
        storage = coff_storage;
        first = storage->first;
        if (first) {
            last = storage->last->data;
            if (last && index > last->symbol_index) {
                entry = output_allocate(8);
                storage = coff_storage;
                entry->data = data;
                previous = storage->last;
                previous->next = entry;
                storage->last = entry;
                goto inserted;
            }
            if (first->data && index < first->data->symbol_index) {
                entry = output_allocate(8);
                storage = coff_storage;
                entry->data = data;
                previous = storage->first;
                storage->first = entry;
                entry->next = previous;
                goto inserted;
            }
            empty = first->data ? NULL : first;
            next = first->next;
            previous = first;
            while (next) {
                if (next->data) {
                    if (next->data->symbol_index > index)
                        break;
                    empty = NULL;
                } else if (!empty)
                    empty = next;
                next = next->next;
                previous = previous->next;
            }
            if (empty) {
                empty->data = data;
                goto inserted;
            }
            entry = output_allocate(8);
            storage = coff_storage;
            entry->next = next;
            entry->data = data;
            previous->next = entry;
            goto inserted;
        }
    }
    entry = output_allocate(8);
    storage = coff_storage;
    entry->data = data;
    previous = storage->last;
    if (previous)
        previous->next = entry;
    else
        storage->first = entry;
    storage->last = entry;
inserted:
    ++storage->count;
    return ++storage->next_index;
}

C2OutputSectionLink *coff_section_buckets[16];

void __fastcall record_output_section(C2Section *section)
{
    C2OutputSectionLink *link, *previous;
    unsigned bucket;
    link = c2_pool_allocate(7, sizeof(*link));
    link->section = section;
    bucket = section->symbol.serial & 15;
    previous = coff_section_buckets[bucket];
    coff_section_buckets[bucket] = link;
    link->next = previous;
}

unsigned short __fastcall select_coff_section(unsigned short section)
{
    C2OutputSection *entry;
    C2CoffSectionData *data;
    unsigned short previous;
    entry = lookup_output_table(coff_table, section);
    if (!entry)
        return coff_default_flags;
    if (!(entry->flags & 2))
        return coff_default_flags;
    previous = coff_flags;
    coff_current_section = entry;
    data = entry->data;
    coff_flags = section;
    coff_current_list = data->output;
    return previous;
}

unsigned __fastcall query_coff_position(unsigned short section)
{
    C2OutputSection *entry;
    void *observed;
    unsigned position;
    if (section == coff_default_flags) {
        entry = coff_current_section;
        observed = lookup_output_table(coff_table, coff_flags);
        if (coff_current_section != observed)
            c2_report_internal("E:\\utc\\src\\\\P2\\coffemit.c", 2404);
    } else
        entry = lookup_output_table(coff_table, section);
    if (!entry) {
        c2_report_internal("E:\\utc\\src\\\\P2\\coffemit.c", 2413);
        return 0;
    }
    if (!(entry->flags & 1))
        return 0;
    position = output_list_position(entry->data->output);
    return position + entry->base_offset;
}

unsigned __fastcall output_list_position(C2OutputList *list)
{
    return list->used_bytes + list->completed_bytes;
}

void __fastcall write_output_bytes(const void *data, unsigned bytes)
{
    C2OutputList *list = coff_current_list;
    C2OutputChunk *old, *chunk;
    unsigned count;

    if (list->count_only == 1)
        goto count_only;
    if (!list->buffered) {
        if (fwrite(data, bytes, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 3254);
        goto count_only;
    }
    if (500 - list->used_bytes < bytes) {
        do {
            count = 500 - list->used_bytes;
            bytes -= count;
            old = list->last;
            memcpy(old->data + list->used_bytes, data, count);
            list->used_bytes += count;
            data = (const char *)data + count;
            if (list->used_bytes != 500)
                c2_report_internal("E:\\utc\\src\\\\P2\\coffemit.c", 3265);
            chunk = new_output_chunk();
            list->last = chunk;
            chunk->next = old->next;
            old->next = chunk;
            list->completed_bytes += list->used_bytes;
            list->used_bytes = 0;
        } while (bytes > 500);
    }
    memcpy(list->last->data + list->used_bytes, data, bytes);
    list->used_bytes += bytes;
    ++list->write_count;
    return;

count_only:
    list->used_bytes += bytes;
    ++list->write_count;
}

void __fastcall write_output_byte(unsigned char value)
{
    write_output_bytes(&value, 1);
}

extern int c2_line_numbers;
extern char *default_function_source_filename;
unsigned coff_function_source_line; /*497e50*/
C2Section *function_fpo_section; /*497e60*/
char *coff_current_source_filename; /*497f00*/
unsigned coff_function_symbol_index; /*497f08*/
/* Exact scalar controls on the deferred-output path. No surrounding object
 * extent or unvisited incremental state is represented by these DWORDs. */
unsigned incremental_symbol_write_mode; /*49e334*/
void missing_listing_symbols(void) { __asm { int 3 } }
void __fastcall restore_listing_symbol_indices(unsigned short *indices, int count, int mode)
{ __asm { int 3 } }
extern unsigned last_section_serial;
C2Section *__fastcall clone_function_section(C2Section *function_section, C2Section *section)
{
    C2Section *copy;
    unsigned char kind = section->section_kind;
    copy = create_section(section->symbol.name, section->class_name, next_symbol_serial());
    copy->alignment = section->alignment;
    if (function_section->combine == 7) {
        copy->combine = 7;
        copy->selection = 5;
        if (kind == 1 && section->symbol.serial != last_section_serial)
            copy->symbol.name = ".data";
    } else {
        switch (kind) {
        case 1:
            if (section->symbol.serial != last_section_serial) copy->symbol.name = ".data";
            break;
        case 3: copy->symbol.name = ".bss"; break;
        case 4: copy->symbol.name = ".rdata"; break;
        case 9: break; /* retain the FPO section's genuine input name */
        case 11: copy->symbol.name = ".tls$"; break;
        case 17: copy->symbol.name = ".xdata$x"; break;
        case 18: copy->symbol.name = ".text$x"; break;
        case 19: copy->symbol.name = ".debug$S"; break;
        default: c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 2010); break;
        }
    }
    copy->section_kind = kind;
    copy->parent = function_section;
    return copy;
}
unsigned __fastcall coff_symbol_type(C2Symbol *symbol)
{
    return symbol->kind == 4 ? 0x20 : 0; /* IMAGE_SYM_DTYPE_FUNCTION << 4 */
}
extern char decorated_name[2059];
extern int c2_identifier_limit;
int __fastcall format_extended_symbol_serial(unsigned serial, char *buffer)
{ __asm { int 3 } return 0; }
char *__fastcall coff_symbol_name(C2Symbol *symbol)
{
    char *name = symbol->name, *tail;
    unsigned storage;
    if (name) {
        strcpy(decorated_name, name);
        decorated_name[c2_identifier_limit] = 0;
        if (decorated_name[0] == '?') return decorated_name;
    }
    switch (symbol->kind) {
    case 1:
        if (!name) {
            decorated_name[0] = '$';
            if (symbol->source_kind == '$') {
                decorated_name[1] = 'S';
                tail = decorated_name + 2;
            } else if (symbol->source_kind == '%') {
                decorated_name[1] = 'S'; decorated_name[2] = 'G';
                tail = decorated_name + 3;
            } else {
                decorated_name[1] = 'T';
                tail = decorated_name + 2;
            }
            goto append_serial;
        }
        if (symbol->source_kind == '$') {
            strcat(decorated_name, "$S");
            goto append_to_name;
        }
        storage = ((C2InputSymbol *)symbol)->attributes & 0x70;
        if (storage == 0x10 || storage == 0x30) {
            strcat(decorated_name, "$");
            if (((C2InputSymbol *)symbol)->storage.local.lifetime > 2) goto append_to_name;
        }
        break;
    case 3:
        if (symbol->source_kind) strcpy(decorated_name, "$L");
        else strcat(decorated_name, "$");
        /* A missing kind3 name retains the existing scratch contents, as in
         * the original; no invented empty-name initialization is performed. */
append_to_name:
        tail = decorated_name + strlen(decorated_name);
        goto append_serial;
    case 4:
        if (!name) {
            decorated_name[0] = '$'; decorated_name[1] = 'E';
            tail = decorated_name + 2;
            goto append_serial;
        }
        break;
    case 9: break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 1146);
        break;
    }
    return decorated_name;
append_serial:
    if (c2_il_extended_header && symbol->serial >= 0x3f0000 && symbol->serial <= 0x3fffff)
        format_extended_symbol_serial(symbol->serial, tail);
    else format_integer((int)symbol->serial, tail, 10);
    return decorated_name;
}
/* Incremental symbol-slot bookkeeping, not an inferred enclosing object. */
unsigned incremental_symbol_mode; /*497d5c*/
unsigned incremental_symbol_cursor; /*497d60*/
unsigned incremental_symbol_base; /*497d64*/
unsigned incremental_symbol_limit; /*497d68*/
unsigned __fastcall write_coff_symbol(char *name, unsigned short section, unsigned char storage,
    unsigned value, unsigned short type, unsigned serial);
unsigned __fastcall write_auxiliary_coff_symbol(char *name, unsigned short section, unsigned char storage,
    unsigned value, unsigned short type, unsigned short *auxiliary, unsigned serial)
{ __asm { int 3 } return 0; }
/* Only the written +4 index is modeled in a deferred reference target. */
typedef struct C2CoffIndexTarget {
    unsigned char pending00[4];
    unsigned index;
} C2CoffIndexTarget;
typedef struct C2CoffIndexFixup {
    unsigned serial;
    C2CoffIndexTarget *target;
    struct C2CoffIndexFixup *next;
} C2CoffIndexFixup;
C2CoffIndexFixup *coff_index_fixups[32]; /*497e80, initialized NULL heads*/
void __fastcall remember_function_coff_symbol(unsigned serial, unsigned index)
{
    C2CoffIndexFixup *node, *previous = NULL;
    unsigned bucket = serial & 31;
    for (node = coff_index_fixups[bucket]; node; node = node->next) {
        if (node->serial == serial) {
            node->target->index = index;
            if (previous) previous->next = node->next;
            else coff_index_fixups[bucket] = node->next;
        } else previous = node;
    }
}
unsigned __fastcall write_function_coff_symbol(unsigned serial, char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, unsigned short auxiliary)
{
    unsigned index;
    if (!auxiliary || !(coff_debug & 1)) {
        index = write_coff_symbol(name, section, storage, offset, type, serial);
        if (c2_internal_listing && !coff_reserved_symbols)
            incremental_symbol_limit = incremental_symbol_cursor - 1;
    } else {
        index = write_auxiliary_coff_symbol(name, section, storage, offset, type, &auxiliary, serial);
        if (c2_internal_listing && !coff_reserved_symbols) {
            incremental_symbol_base = incremental_symbol_cursor;
            incremental_symbol_limit = incremental_symbol_cursor + 4;
            incremental_symbol_mode = 0;
            incremental_symbol_cursor = 0;
        }
    }
    remember_function_coff_symbol(serial, index);
    return index;
}

void __fastcall output_callback_434139(C2Symbol *header, char *file, int unwind, unsigned line)
{
    C2FunctionSymbol *symbol = (C2FunctionSymbol *)header;
    C2Section *section, *debug;
    C2ListingSymbol *entry;
    unsigned short indices[8], type, section_number;
    unsigned serial, extra, index;
    unsigned char storage;
    char *name;
    int count;
    coff_function_source_line = line;
    if (deferred_compilation) {
        entry = symbol->listing->symbols;
        if (!entry) missing_listing_symbols();
        else {
            count = 0;
            /* The original fixed local array has no oversized-list check. */
            for (; entry; entry = entry->next) indices[count++] = entry->index;
            restore_listing_symbol_indices(indices, count, 0);
            symbol->listing->symbols = NULL;
        }
    }
    output_callback_41fa04(header);
    if (c2_internal_listing && !coff_reserved_symbols) {
        extra = line && (c2_codeview_types || c2_line_numbers) ? strlen(file) / 18 + 8 : 0;
        reserve_incremental_symbols(extra + 3, 0, incremental_symbol_write_mode ^ 1);
    }
    serial = header->association.section_serial;
    section = (C2Section *)lookup_global_symbol(serial);
    create_coff_section(section);
    if (c2_codeview_types) {
        if ((header->flags & 0x20) || c2_internal_listing) {
            debug = (C2Section *)lookup_global_symbol(7);
            section = (C2Section *)lookup_global_symbol(serial);
            debug_symbols_section = clone_function_section(section, debug);
        } else debug_symbols_section = (C2Section *)lookup_global_symbol(7);
    }
    if (unwind) {
        if ((header->flags & 0x20) || c2_internal_listing) {
            section = (C2Section *)lookup_global_symbol(serial);
            function_fpo_section = clone_function_section(section, fpo_section);
        } else function_fpo_section = fpo_section;
    }
    if (!coff_current_source_filename && (!default_function_source_filename ||
        !strcmp(default_function_source_filename, file))) coff_current_source_filename = file;
    if ((file != coff_current_source_filename || c2_internal_listing) && line &&
        (c2_codeview_types || c2_line_numbers)) {
        add_output_source(file);
        coff_current_source_filename = file;
    }
    switch ((symbol->attributes >> 4) & 15) {
    case 3: storage = 3; break;
    case 5: storage = 2; break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 548);
        return;
    }
    section = (C2Section *)lookup_global_symbol(serial);
    section_number = (unsigned short)section->coff_section;
    type = (unsigned short)coff_symbol_type(header);
    name = coff_symbol_name(header);
    index = write_function_coff_symbol(header->serial, name, section_number, storage,
        header->parameter_offset, type, line != 0);
    coff_function_symbol_index = index;
    if (!header->coff_symbol_index) {
        header->coff_symbol_index = index;
        header->output_flags |= 1;
    }
}

void __fastcall remove_incremental_symbol(unsigned serial) { __asm { int 3 } }
unsigned __fastcall find_incremental_coff_symbol(unsigned serial, unsigned short section)
{ __asm { int 3 } return 0; }
unsigned __fastcall coff_string_position(C2OutputList *list)
{
    return list->completed_bytes + list->used_bytes;
}
void __fastcall set_coff_symbol_name(IMAGE_SYMBOL *symbol, char *name, int copy)
{
    C2OutputList *saved;
    char *text;
    int length = (int)strlen(name);
    if (length < 9) {
        strncpy((char *)symbol->N.ShortName, name, length);
        memset(symbol->N.ShortName + length, 0, 8 - length);
        return;
    }
    if (!copy && deferred_compilation) {
        symbol->N.Name.Short = 0;
        text = output_allocate(length + 1);
        symbol->N.LongName[1] = (unsigned char *)text;
        memcpy(text, name, length + 1);
        return;
    }
    symbol->N.Name.Short = 0;
    /* The separate string-position entry is a genuine physical function,
     * distinct from the already known output-list-position helper. */
    symbol->N.Name.Long = coff_string_position(coff_output_list) + 4;
    saved = coff_current_list;
    coff_current_list = coff_output_list;
    write_output_bytes(name, length + 1);
    coff_current_list = saved;
}
unsigned __fastcall write_coff_symbol(char *name, unsigned short section, unsigned char storage,
    unsigned value, unsigned short type, unsigned serial)
{
    C2CoffRecord *record;
    C2OutputSection *entry;
    IMAGE_SYMBOL *symbol;
    unsigned index;
    if (deferred_compilation && serial) {
        index = find_incremental_coff_symbol(serial, section);
        if (index) {
            symbol = coff_symbol_buffer[index]->value.symbol;
            if (section) {
                entry = lookup_output_table(coff_table, section);
                symbol->SectionNumber = coff_new_output ? (short)entry->data->storage_index :
                    (short)(-2 - (int)entry->data->storage_index);
            }
            symbol->Value = value;
            if (!section && (type & 0x30) == 0x20) set_coff_symbol_name(symbol, name, 0);
            return index;
        }
    }
    if (c2_internal_listing && !incremental_symbol_mode) reserve_incremental_symbols(1, section, 0);
    record = output_allocate(sizeof(*record));
    symbol = output_allocate(sizeof(*symbol));
    symbol->Value = value;
    symbol->StorageClass = storage;
    if (section == coff_default_flags) symbol->SectionNumber = 0;
    else {
        entry = lookup_output_table(coff_table, section);
        symbol->SectionNumber = coff_new_output ? (short)entry->data->storage_index :
            (short)(-2 - (int)entry->data->storage_index);
    }
    symbol->Type = type;
    symbol->NumberOfAuxSymbols = 0;
    set_coff_symbol_name(symbol, name, 0);
    record->kind = 3;
    record->value.symbol = symbol;
    record->serial = serial;
    return c2_internal_listing ? add_incremental_symbol(record) : add_coff_symbol(record);
}
unsigned __fastcall write_weak_coff_symbol(char *name, unsigned short section, unsigned char storage,
    unsigned value, unsigned short type, unsigned target, unsigned search, unsigned serial)
{ __asm { int 3 } return 0; }

void __fastcall output_callback_41fa04(C2Symbol *symbol)
{
    C2FunctionSymbol *function = (C2FunctionSymbol *)symbol;
    C2InputSymbol *input = (C2InputSymbol *)symbol;
    C2Symbol *target;
    C2Section *section;
    unsigned flags = symbol->output_flags, serial, value, attributes;
    unsigned short section_number = coff_default_flags, type;
    unsigned char storage;
    char *name;

    if (flags & 1) return;
    if (!(flags & 4)) c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 681);
    symbol->output_flags = (unsigned char)(flags | 1);
    switch (symbol->kind) {
    case 4:
        attributes = function->attributes;
        if (attributes & 0x800) {
            name = coff_symbol_name(symbol);
            write_coff_directive("-export:", name, NULL, 0);
        }
        if (attributes & 0x8000) {
            /* The same input-order DWORD carries the weak target serial for
             * this function-symbol attribute domain. */
            target = lookup_global_symbol((unsigned)function->input_order);
            ((C2FunctionSymbol *)target)->additional_flags |= 0x20;
            target->output_flags |= 4;
            output_callback_41fa04(target);
            if (!target->coff_symbol_index) {
                serial = target->serial;
                type = (unsigned short)coff_symbol_type(target);
                name = coff_symbol_name(target);
                target->coff_symbol_index = write_coff_symbol(name, coff_default_flags, 2, 0, type, serial);
            }
            type = (unsigned short)coff_symbol_type(symbol);
            name = coff_symbol_name(symbol);
            symbol->coff_symbol_index = write_weak_coff_symbol(name, coff_default_flags, 0x69,
                symbol->parameter_offset, type, target->coff_symbol_index, 2, symbol->serial);
            return;
        }
        switch ((attributes >> 4) & 15) {
        case 3: case 5: return;
        case 4:
            type = (unsigned short)coff_symbol_type(symbol);
            name = coff_symbol_name(symbol);
            symbol->coff_symbol_index = write_coff_symbol(name, section_number, 2, 0, type, symbol->serial);
            return;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 865);
            return;
        }
    case 3:
        value = symbol->parameter_offset;
        section = (C2Section *)lookup_global_symbol(symbol->association.section_serial);
        section_number = create_coff_section(section);
        serial = is_known_symbol(symbol->serial) ? symbol->serial : 0;
        storage = 6;
        break;
    case 1:
        attributes = input->attributes;
        if ((attributes & 0x70) == 0x10 || (attributes & 0x70) == 0x30) return;
        if (attributes & 0x800) {
            name = coff_symbol_name(symbol);
            write_coff_directive("-export:", name, ",data", 0);
        }
        switch ((attributes >> 7) & 15) {
        case 1:
            value = symbol->flags & 0x80000 ? 4 : symbol->parameter_offset;
            storage = 2;
            if (deferred_compilation && (symbol->flags & 0x20) && is_known_symbol(symbol->serial))
                remove_incremental_symbol(symbol->serial);
            section = (C2Section *)lookup_global_symbol(symbol->association.section_serial);
            section_number = create_coff_section(section);
            break;
        case 2:
            value = 0; storage = 2;
            break;
        case 3:
            value = symbol->value; storage = 2;
            break;
        case 4:
            value = symbol->parameter_offset; storage = 3;
            section = (C2Section *)lookup_global_symbol(symbol->association.section_serial);
            section_number = create_coff_section(section);
            break;
        default: return;
        }
        serial = is_known_symbol(symbol->serial) ? symbol->serial : 0;
        break;
    default: return;
    }
    type = (unsigned short)coff_symbol_type(symbol);
    name = coff_symbol_name(symbol);
    symbol->coff_symbol_index = write_coff_symbol(name, section_number, storage, value, type, serial);
}

unsigned char __fastcall coff_selection_code(unsigned char selection)
{
    switch (selection) {
    case 1: return 2; /* choose any definition */
    case 2: return 1; /* reject duplicate definitions */
    case 3: return 3; /* require the same size */
    case 4: return 4; /* require exact contents */
    case 5: return 5; /* associative section */
    case 6: return 6; /* choose the largest definition */
    case 7: return 7; /* choose the newest definition */
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 1901);
        return 0;
    }
}

unsigned short __fastcall add_associative_coff_section(char *name, unsigned flags,
    unsigned char selection, unsigned short association, int flag8, int reuse)
{
    C2OutputSection *section;
    if (reuse && coff_reuse_index >= 0) {
        /* Only the existing encountered eight-pointer reuse view is modeled. */
        if (coff_reuse_index >= 8) { __asm { int 3 } }
        section = coff_reuse_sections[coff_reuse_index];
        if (section) {
            ++coff_reuse_index;
            if (flag8) section->flags |= 8;
            section->characteristics = flags;
            section->selection = selection;
            section->association = association;
            section->name = name;
            return section->index;
        }
    }
    section = output_allocate(sizeof(*section));
    if (flag8) section->flags |= 8;
    section->association = association;
    section->characteristics = flags;
    section->selection = selection;
    section->name = name;
    section->index = (unsigned short)append_output_table(coff_table, section);
    return section->index;
}

/* WORD writes to the real497e68 DWORD preserve its upper half. */
unsigned coff_function_line_count;
void __fastcall begin_coff_function_lines(unsigned index, unsigned short line,
    unsigned code_bytes, unsigned offset) { __asm { int 3 } }
void __fastcall output_callback_4346cf(C2Symbol *header)
{
    C2FunctionSymbol *symbol = (C2FunctionSymbol *)header;
    if (coff_function_source_line && (c2_codeview_types || c2_line_numbers)) {
        begin_coff_function_lines(coff_function_symbol_index,
            (unsigned short)coff_function_source_line, symbol->code_bytes,
            header->parameter_offset);
        *(unsigned short *)&coff_function_line_count = 1;
    } else *(unsigned short *)&coff_function_line_count = 0;
}

/* This independently named callback entry is the original five-byte tail
 * transfer; write_output_bytes retains its own separate function identity. */
void __fastcall output_callback_41f3f2(const void *bytes, unsigned count)
{
    write_output_bytes(bytes, count);
}

unsigned coff_line_restart; /*497e58: initialized0; real function end writes1.*/
unsigned coff_last_source_line; /*497e64: initialized0; line callback supplies it.*/
void __fastcall write_coff_line_count(unsigned short count) { __asm { int 3 } }
void __fastcall end_coff_function_lines(unsigned short line, unsigned offset) { __asm { int 3 } }
void __fastcall output_callback_435633(C2Symbol *header)
{
    C2FunctionSymbol *symbol = (C2FunctionSymbol *)header;
    if ((unsigned short)coff_function_line_count && (c2_codeview_types || c2_line_numbers)) {
        write_coff_line_count((unsigned short)coff_function_line_count);
        end_coff_function_lines((unsigned short)coff_last_source_line,
            symbol->code_bytes + header->parameter_offset);
    }
    coff_line_restart = 1;
}

void __fastcall queue_coff_symbol_fixup(unsigned serial, IMAGE_RELOCATION *relocation)
{ __asm { int 3 } }
void __fastcall write_coff_relocation(unsigned serial, unsigned offset,
    unsigned symbol_index, unsigned short type)
{
    C2OutputSection *section = coff_current_section;
    C2CoffSectionData *data = section->data;
    C2OutputBlock508 *block = data->relocations, *next;
    IMAGE_RELOCATION *relocation = &block->entries[block->count++];
    relocation->VirtualAddress = offset - section->base_offset;
    relocation->SymbolTableIndex = symbol_index;
    relocation->Type = type;
    if (!symbol_index) queue_coff_symbol_fixup(serial, relocation);
    ++data->relocation_count;
    if ((int)block->count >= 50) {
        next = coff_free_blocks_508;
        if (next) coff_free_blocks_508 = next->last;
        else next = output_allocate(508);
        next->count = 0;
        next->last = block->last;
        block->last = next;
        data->relocations = next;
    }
}
void __fastcall output_callback_4357b8(unsigned offset, C2Symbol *symbol)
{
    offset += query_coff_position(coff_default_flags);
    write_coff_relocation(symbol->serial, offset, symbol->coff_symbol_index, 7);
}

void __fastcall output_callback_435af6(unsigned offset, C2Symbol *symbol)
{
    offset += query_coff_position(coff_default_flags);
    write_coff_relocation(symbol->serial, offset, symbol->coff_symbol_index,
        IMAGE_REL_I386_REL32);
}

void __fastcall write_coff_symbol_relocation(unsigned offset, C2Symbol *symbol,
    unsigned short type)
{
    offset += query_coff_position(coff_default_flags);
    write_coff_relocation(symbol->serial, offset, symbol->coff_symbol_index, type);
}

void __fastcall output_callback_43696a(unsigned offset, C2Symbol *symbol)
{
    unsigned short type = IMAGE_REL_I386_DIR32;
    if (symbol->kind == 1 && (symbol->flags & 0x1000))
        type = IMAGE_REL_I386_SECREL;
    write_coff_symbol_relocation(offset, symbol, type);
}

/* Four independently owned incremental-output DWORDs. The existing
 * incremental_data is also the49e31c byte-buffer base; it is not duplicated.
 * Pristine flags/count/cursor are0; their real writers precede later use. */
unsigned incremental_buffer_left; /*49e314*/
unsigned char *incremental_buffer_cursor; /*49e324*/
unsigned incremental_stream_append; /*49e32c: first replace, subsequent append*/
unsigned incremental_extended_counts; /*49e330: extended relocation/line counts*/
unsigned __fastcall coff_alignment_characteristics(unsigned alignment)
{
    /* SDK IMAGE_SCN_ALIGN_nBYTES values for the seven supported alignments. */
    switch (alignment) {
    case 1: return IMAGE_SCN_ALIGN_1BYTES;
    case 2: return IMAGE_SCN_ALIGN_2BYTES;
    case 4: return IMAGE_SCN_ALIGN_4BYTES;
    case 8: return IMAGE_SCN_ALIGN_8BYTES;
    case 16: return IMAGE_SCN_ALIGN_16BYTES;
    case 32: return IMAGE_SCN_ALIGN_32BYTES;
    case 64: return IMAGE_SCN_ALIGN_64BYTES;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x79c);
        return 0;
    }
}
void __fastcall or_coff_section_characteristics(unsigned short section, unsigned flags)
{
    C2OutputSection *entry = lookup_output_table(coff_table, section);
    /* The original assumes this section lookup succeeds. */
    entry->characteristics |= flags;
}
void *__fastcall next_output_table_entry(C2OutputTable *table)
{
    unsigned index = table->iterator_index;
    if (!index) {
        table->iterator_used = 0;
        table->iterator_page = table->first;
    }
    if (index >= table->count) {
        table->iterator_index = 0;
        return NULL;
    }
    table->iterator_index = index + 1;
    if (table->iterator_used >= table->capacity) {
        table->iterator_used = 0;
        table->iterator_page = table->iterator_page->next;
    }
    return table->iterator_page->items[table->iterator_used++];
}

/* Finalize active entries; no section is removed from the table. */
void finish_coff_sections(void)
{
    C2OutputSection *section = next_output_table_entry(coff_table);
    while (section) {
        if (section->data->relocation_count >= 0xffffU)
            incremental_extended_counts = 1;
        if (section->flags & 2) {
            --coff_section_count;
            section->flags &= ~2;
        }
        section = next_output_table_entry(coff_table);
    }
}
void prepare_incremental_sections(void) { __asm { int 3 } }
void prepare_incremental_symbols(void) { __asm { int 3 } }
unsigned __fastcall output_table_count(C2OutputTable *table)
{
    __asm { int 3 }
    return 0;
}
void __fastcall reserve_coff_headers(unsigned short count)
{
    unsigned needed;

    if (coff_pending_e18)
        return;
    if (!count)
        coff_pending_e18 = (unsigned short)coff_storage->count;
    else if ((unsigned)count < output_table_count(coff_table))
        output_error_handler(7, "E:\\utc\\src\\\\P2\\coffemit.c", 0x3a7);
    else {
        needed = coff_storage->count + count - output_table_count(coff_table);
        if (needed > 0xffffU)
            output_error_handler(6, "E:\\utc\\src\\\\P2\\coffemit.c", 0x3b0);
        else
            coff_pending_e18 = (unsigned short)needed;
    }
    /* If an error callback returns, the original still seeks using the
       unchanged reservation. It does not synthesize a replacement count. */
    fseek(coff_output, (unsigned)coff_pending_e18 * 40 + 20, SEEK_SET);
}
void __fastcall grow_incremental_headers(unsigned bytes) { __asm { int 3 } }
unsigned __fastcall coff_relocation_bytes(C2CoffSectionData *data) { __asm { int 3 } return 0; }
unsigned __fastcall coff_line_number_bytes(C2CoffSectionData *data) { __asm { int 3 } return 0; }
unsigned __fastcall reserve_incremental_section(unsigned bytes, unsigned previous_offset) { __asm { int 3 } return 0; }
void __fastcall write_coff_section_relocations(C2CoffSectionData *data, unsigned base)
{
    IMAGE_RELOCATION overflow;
    C2OutputBlock508 *last, *block, *first;
    unsigned index;

    if (data->relocation_count >= 0xffffU) {
        overflow.VirtualAddress = data->relocation_count + 1;
        overflow.SymbolTableIndex = 0;
        overflow.Type = 0;
        if (fwrite(&overflow, 10, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xd3f);
    }
    last = data->relocations;
    block = last;
    do {
        block = block->last;
        index = block->count;
        while (index) {
            --index;
            block->entries[index].VirtualAddress += base;
        }
        if (fwrite(block->entries, 10, block->count, coff_output) != block->count)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xd4c);
    } while (block != last);
    first = last->last;
    last->last = coff_free_blocks_508;
    coff_free_blocks_508 = first;
}
void __fastcall write_coff_section_lines(C2CoffSectionData *data, unsigned base) { __asm { int 3 } }
void __fastcall write_coff_section_data(C2OutputSection *section)
{
    C2CoffSectionData *data = section->data;
    C2OutputList *list = data->output;
    unsigned size = output_list_position(list), offset, lines, relocations;

    if (list->count_only == 1)
        data->raw_data_offset = 0;
    else {
        if (incremental_header_output) {
            offset = data->raw_data_offset;
            lines = coff_line_number_bytes(data);
            relocations = coff_relocation_bytes(data);
            offset = reserve_incremental_section(size + lines + relocations, offset);
            if (fseek(coff_output, offset, SEEK_SET))
                c2_report_internal("E:\\utc\\src\\\\P2\\coffemit.c", 0xa74);
        } else
            offset = ftell(coff_output);
        data->raw_data_offset = offset;
    }
    data->virtual_address = 0;
    data->virtual_size = section->flags & 0x10 ? 1 : 0;
    data->size = size;
    flush_coff_list(data->output);
    if (data->relocation_count) {
        data->relocation_offset = ftell(coff_output);
        write_coff_section_relocations(data, 0);
    } else
        data->relocation_offset = 0;
    if (data->line_number_count) {
        data->line_number_offset = ftell(coff_output);
        write_coff_section_lines(data, data->virtual_address);
    } else
        data->line_number_offset = 0;
    data->flags |= 1;
}
void __fastcall write_incremental_ulong(unsigned value) { __asm { int 3 } }
void write_incremental_header_metadata(void) { __asm { int 3 } }
unsigned long writer_callback_45477a(void)
{
    time_t stamp;
    if (c2_reproducible)
        return 0;
    time(&stamp);
    return (unsigned long)stamp;
}
IMAGE_SYMBOL *coff_current_symbol; /*499acc; initialized NULL, then owned by this writer. */
void __fastcall write_coff_symbol_record(C2CoffRecord *record)
{
    IMAGE_SYMBOL symbol;
    IMAGE_AUX_SYMBOL auxiliary;
    C2CoffSource *source;
    C2CoffSectionData *data;
    C2OutputSection *section, *associated;
    C2OutputList *saved;
    char *name;
    unsigned length, padding;

    switch (record->kind) {
    case 1:
        strncpy((char *)symbol.N.ShortName, ".file", 8);
        source = record->value.source;
        symbol.Value = source->next_source;
        symbol.SectionNumber = IMAGE_SYM_DEBUG;
        symbol.Type = 0;
        symbol.StorageClass = IMAGE_SYM_CLASS_FILE;
        length = strlen(source->name);
        symbol.NumberOfAuxSymbols = (unsigned char)(length / 18 + 1);
        if (fwrite(&symbol, 18, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb00);
        if (fwrite(source->name, length + 1, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb04);
        /* The original BYTE auxiliary count wraps for very long filenames;
           retain its DWORD padding arithmetic rather than widening the count. */
        padding = (unsigned)symbol.NumberOfAuxSymbols * 18 - length - 1;
        while (padding--) fputc(0, coff_output);
        coff_current_symbol = NULL;
        break;
    case 2:
        data = record->value.section;
        section = data->section;
        set_coff_symbol_name(&symbol, section->name, 1);
        symbol.Value = 0;
        symbol.SectionNumber = coff_new_output ? (short)data->storage_index :
            (short)(-2 - (int)data->storage_index);
        symbol.Type = 0;
        symbol.StorageClass = IMAGE_SYM_CLASS_STATIC;
        symbol.NumberOfAuxSymbols = 1;
        if (fwrite(&symbol, 18, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb19);
        memset(&auxiliary, 0, sizeof(auxiliary));
        auxiliary.Section.Length = data->size;
        auxiliary.Section.NumberOfRelocations = data->relocation_count >= 0xffffU ?
            0xffff : (unsigned short)data->relocation_count;
        auxiliary.Section.NumberOfLinenumbers = data->line_number_count;
        if (section->characteristics & IMAGE_SCN_LNK_COMDAT) {
            auxiliary.Section.CheckSum = 0;
            auxiliary.Section.Selection = section->selection;
            if (section->association) {
                associated = lookup_output_table(coff_table, section->association);
                auxiliary.Section.Number = coff_new_output ? (short)associated->data->storage_index :
                    (short)(-2 - (int)associated->data->storage_index);
            } else
                auxiliary.Section.Number = 0;
        }
        if (fwrite(&auxiliary, 18, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb33);
        coff_current_symbol = NULL;
        break;
    case 3:
        coff_current_symbol = record->value.symbol;
        if (deferred_compilation && !coff_current_symbol->N.Name.Short) {
            name = (char *)coff_current_symbol->N.LongName[1];
            coff_current_symbol->N.Name.Long = coff_string_position(coff_output_list) + 4;
            saved = coff_current_list;
            coff_current_list = coff_output_list;
            write_output_bytes(name, strlen(name) + 1);
            coff_current_list = saved;
        }
        if (fwrite(coff_current_symbol, 18, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb46);
        break;
    case 4:
        if (fwrite(record->value.symbol, 18, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb4d);
        coff_current_symbol = NULL;
        break;
    case 5:
        break;
    case 6:
    case 7:
        strncpy((char *)symbol.N.ShortName, "$unused$", 8);
        symbol.Value = 0;
        symbol.SectionNumber = 0;
        symbol.Type = 0;
        symbol.StorageClass = 14;
        symbol.NumberOfAuxSymbols = 0;
        if (fwrite(&symbol, 18, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb60);
        break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\coffemit.c", 0xb65);
        break;
    }
}
void __fastcall encode_coff_section_name(unsigned char *destination, char *name)
{
    C2OutputList *saved;
    int length;

    memset(destination, 0, 8);
    length = (int)strlen(name);
    if (length < 9)
        strncpy((char *)destination, name, length);
    else {
        destination[0] = '/';
        /* Original defect: decimal offsets are formatted directly into the
           eight-byte name field without checking that they fit. */
        format_integer((int)(coff_string_position(coff_output_list) + 4),
            (char *)destination + 1, 10);
        saved = coff_current_list;
        coff_current_list = coff_output_list;
        write_output_bytes(name, length + 1);
        coff_current_list = saved;
    }
}
void __fastcall write_coff_section_header(C2CoffSectionData *data)
{
    IMAGE_SECTION_HEADER header;
    C2OutputSection *section = data->section;

    encode_coff_section_name(header.Name, section->name);
    header.Misc.PhysicalAddress = data->virtual_size;
    header.VirtualAddress = data->virtual_address;
    header.SizeOfRawData = data->size;
    header.PointerToRawData = data->raw_data_offset;
    header.PointerToRelocations = data->relocation_offset;
    header.PointerToLinenumbers = data->line_number_offset;
    header.NumberOfRelocations = (unsigned short)data->relocation_count;
    if (data->relocation_count >= 0xffffU)
        header.NumberOfRelocations = 0xffff;
    header.NumberOfLinenumbers = data->line_number_count;
    header.Characteristics = section->characteristics;
    if (data->relocation_count >= 0xffffU)
        header.Characteristics |= 0x01000000U; /* IMAGE_SCN_LNK_NRELOC_OVFL */
    if (fwrite(&header, 40, 1, coff_output) != 1)
        output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xb90);
}
void __fastcall flush_coff_list(C2OutputList *list)
{
    C2OutputChunk *chunk, *last;

    if (!list)
        return;
    if (list->count_only != 1) {
        last = list->last;
        chunk = last->next;
        while (chunk != last) {
            if (fwrite(chunk->data, 500, 1, coff_output) != 1)
                output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xceb);
            chunk = chunk->next;
        }
        if (list->used_bytes &&
            fwrite(chunk->data, list->used_bytes, 1, coff_output) != 1)
            output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0xcf0);
    }
    chunk = list->last->next;
    list->last->next = coff_free_chunks;
    coff_free_chunks = chunk;
}

unsigned incremental_header_position; /*49e488*/
unsigned incremental_data_position; /*49e48c*/
int incremental_header_output; /*499ad0*/
C2OutputList *coff_extra_output_list; /*499ad4*/
void finish_coff_image(void)
{
    IMAGE_FILE_HEADER header;
    C2OutputSection *section;
    C2CoffRecord *record;
    C2OutputStorageEntry *entry;
    unsigned bytes, index;

    if (deferred_compilation) {
        prepare_incremental_sections();
        prepare_incremental_symbols();
        coff_new_output = 1;
    }
    if (!coff_pending_e18)
        reserve_coff_headers(0);
    fseek(coff_output, (unsigned)coff_pending_e18 * 40 + 20, SEEK_SET);
    if (c2_internal_listing) {
        if (deferred_compilation) {
            bytes = (unsigned)coff_pending_e18 * 40;
            if (bytes > incremental_data_position - 20U)
                grow_incremental_headers(bytes - incremental_data_position + 20);
            incremental_header_output = 1;
        }
        for (section = next_output_table_entry(coff_table); section;
             section = next_output_table_entry(coff_table)) {
            if ((section->flags & 0x10) && (section->flags & 1))
                write_coff_section_data(section);
        }
        if (!deferred_compilation)
            incremental_header_position = ftell(coff_output);
        else {
            fseek(coff_output, incremental_header_position, SEEK_SET);
            incremental_header_output = 0;
        }
    }
    for (section = next_output_table_entry(coff_table); section;
         section = next_output_table_entry(coff_table)) {
        if ((!c2_internal_listing || !(section->flags & 0x10)) &&
            (section->flags & 1) && !(section->flags & 4))
            write_coff_section_data(section);
    }
    header.Machine = coff_machine;
    header.NumberOfSections = (unsigned short)coff_storage->count;
    header.TimeDateStamp = output_clock_handler.target();
    header.NumberOfSymbols = coff_symbol_total;
    header.SizeOfOptionalHeader = 0;
    header.Characteristics = 0;
    header.PointerToSymbolTable = ftell(coff_output);
    if (c2_internal_listing) {
        write_incremental_ulong(header.TimeDateStamp);
        write_incremental_ulong(incremental_header_position);
        write_incremental_header_metadata();
    }
    for (index = 0; index < coff_symbol_total; ++index) {
        record = coff_symbol_buffer[index];
        if (c2_internal_listing && record->kind == 3 && record->serial) {
            write_incremental_ulong(index);
            write_incremental_ulong(record->serial);
        }
        write_coff_symbol_record(record);
    }
    if (c2_internal_listing) {
        write_incremental_ulong(0);
        write_incremental_ulong(0);
    }
    fseek(coff_output, 0, SEEK_SET);
    if (fwrite(&header, sizeof(header), 1, coff_output) != 1)
        output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0x4b8);
    ftell(coff_output);
    for (entry = coff_storage->first; entry; entry = entry->next) {
        if (entry->data)
            write_coff_section_header(entry->data);
    }
    fseek(coff_output, 0, SEEK_END);
    bytes = output_list_position(coff_output_list) + 4;
    if (fwrite(&bytes, sizeof(bytes), 1, coff_output) != 1)
        output_error_handler(14, "E:\\utc\\src\\\\P2\\coffemit.c", 0x4ce);
    flush_coff_list(coff_output_list);
    if (coff_extra_output_list) {
        fseek(coff_output, 0, SEEK_END);
        flush_coff_list(coff_extra_output_list);
    }
}
void __fastcall write_incremental_byte(unsigned char value) { __asm { int 3 } }
void write_incremental_options(void) { __asm { int 3 } }
void write_incremental_objects(void) { __asm { int 3 } }
void write_incremental_sections(void) { __asm { int 3 } }
void write_incremental_global_symbols(void) { __asm { int 3 } }
void write_incremental_symbol_uses(void) { __asm { int 3 } }
int __fastcall replace_incremental_stream(const void *data, unsigned bytes)
{ __asm { int 3 } return 0; }
int __fastcall append_incremental_stream(const void *data, unsigned bytes)
{ __asm { int 3 } return 0; }

extern C2FunctionSymbol *listed_functions;
void output_callback_4541f3(void)
{
    C2OutputSectionLink *link;
    C2Section *section;
    C2FunctionSymbol *function;
    C2ListingSymbol *entry;
    unsigned flags, bucket;
    unsigned short indices[8];
    int count;

    for (bucket = 0; bucket < 16; ++bucket) {
        for (link = coff_section_buckets[bucket]; link; link = link->next) {
            section = link->section;
            flags = coff_alignment_characteristics(section->alignment);
            or_coff_section_characteristics((unsigned short)section->coff_section, flags);
            if (section->symbol.section_size)
                select_output_section(section);
        }
    }
    finish_coff_sections();
    if (c2_internal_listing) {
        incremental_data = c2_pool_allocate(0, 0x2800);
        incremental_buffer_cursor = incremental_data;
        incremental_buffer_left = 0x2800;
        write_incremental_byte(incremental_extended_counts ? 0 : 5);
        write_incremental_options();
        if (deferred_compilation) {
            for (function = listed_functions; function; function = function->pending_next) {
                entry = function->listing->symbols;
                if (entry) {
                    count = 0;
                    /* Original defect: the fixed eight-WORD local has no
                     * oversized-list check before these stores. */
                    for (; entry; entry = entry->next)
                        indices[count++] = entry->index;
                    restore_listing_symbol_indices(indices, count, 0);
                    function->listing->symbols = NULL;
                }
            }
        }
    }
    finish_coff_image();
    if (c2_internal_listing) {
        if (deferred_compilation)
            write_incremental_objects();
        write_incremental_sections();
        write_incremental_global_symbols();
        write_incremental_symbol_uses();
        if (incremental_stream_append) {
            if (!append_incremental_stream(incremental_data, 0x2800 - incremental_buffer_left))
                c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x172);
        } else {
            if (!replace_incremental_stream(incremental_data, 0x2800 - incremental_buffer_left))
                c2_report_internal("E:\\utc\\src\\\\P2\\coff.c", 0x175);
        }
    }
    fclose(object_output);
    object_output_open = 0;
}
