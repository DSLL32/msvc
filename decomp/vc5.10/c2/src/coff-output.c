#include "coff-output.h"
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
    writer->output_error.target = writer_callback_48511c;
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
C2PendingCallback output_error_handler, output_clock_handler;
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
