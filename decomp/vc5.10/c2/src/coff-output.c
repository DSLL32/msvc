#include "coff-output.h"
#include <string.h>
#include <string.h>
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
C2PendingCallback output_clock_handler;
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

/* Function attribute bit23; its wider optimizer meaning is still pending. */
int function_option_23 = 1;

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
        section->alignment = function_option_23 ? stack_alignment :
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
        if (function_option_23)
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

int coff_reuse_output;

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
    if (coff_reuse_output) {
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
    if (coff_reuse_output) {
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
