#include "input.h"
#include <new.h>
#include "keywords.h"
#include "frontend.h"
#include "global-record.h"
#include "packing.h"
#include "browser.h"
#include <recon.h>

extern "C" int __cdecl create_input(int, CompilerInput **, CompilerTextInput **);
extern "C" unsigned __cdecl save_type_context(void);
extern "C" unsigned __cdecl create_diagnostic_text(void);
extern "C" void __cdecl restore_type_context(void);
extern "C" void __cdecl cleanup_saved_contexts(void);
extern "C" char *__cdecl trim_include_name(char *, char *);
extern "C" void __cdecl prepend_diagnostic_text(unsigned, const char *);
extern "C" void __cdecl append_diagnostic_text(unsigned, const char *);
extern "C" void __cdecl append_diagnostic_character(unsigned, int);
extern "C" const char *__cdecl diagnostic_text_data(unsigned);
extern "C" void __cdecl release_diagnostic_text(unsigned);
extern "C" char *__cdecl reserve_diagnostic_text(unsigned, int, unsigned);

extern "C" int c1_test_diagnostic_text_growth(void)
{
    unsigned handle = create_diagnostic_text();
    CompilerSavedContextRecord *row = c1_data.saved_context_records;
    char tail[101];
    memset(tail, 'x', 100);
    tail[100] = 0;
    prepend_diagnostic_text(handle, "volatile ");
    prepend_diagnostic_text(handle, "const ");
    append_diagnostic_character(handle, '*');
    if (strcmp(diagnostic_text_data(handle), "const volatile *") ||
        row->prefix_offset != 49 || row->suffix_offset != 66) return 0;
    ((BYTE *)row->allocation)[40] = 0xa5;
    append_diagnostic_text(handle, tail);
    const char *text = diagnostic_text_data(handle);
    if (row->capacity != 260 || row->prefix_offset != 49 || row->suffix_offset != 166 ||
        ((BYTE *)row->allocation)[40] != 0xa5 || strncmp(text, "const volatile *", 16) ||
        memcmp(text + 16, tail, 101) || reserve_diagnostic_text(handle + 1000, 1, 3)) return 0;
    void *allocation = row->allocation;
    release_diagnostic_text(handle);
    if (row->handle || row->allocation != allocation ||
        create_diagnostic_text() != handle + 1 || row->allocation != allocation) return 0;
    for (unsigned i = 0; i < row->capacity; ++i)
        if (((BYTE *)row->allocation)[i]) return 0;
    cleanup_saved_contexts();
    c1_data.saved_context_records = NULL;
    c1_data.saved_context_count = 0;
    return 1;
}

extern "C" int c1_test_parameter_append_links(void)
{
    DWORD storage[5];
    CompilerIrSymbol nodes[3], sentinel;
    CompilerNameEntry names[3];
    CompilerParameterDescription description;
    memset(storage, 0xa5, sizeof(storage));
    memset(nodes, 0x5a, sizeof(nodes));
    memset(&sentinel, 0x37, sizeof(sentinel));
    description.bindings = NULL;
    description.kind = 6;
    description.scope_active = 1;
    c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 4);
    c1_data.parameter_sentinel = &sentinel;
    for (unsigned i = 0; i < 3; ++i) nodes[i].binding.name = &names[i];
    description.AppendParameter(&nodes[0]);
    CompilerParameterBindings *list = description.bindings;
    if (list != (CompilerParameterBindings *)(storage + 1) || list->arena_kind != 2 ||
        list->first != &nodes[0].binding || list->last != &nodes[0].binding ||
        nodes[0].binding.next || nodes[0].previous_parameter ||
        c1_bss.arenas[2].cursor != (BYTE *)(storage + 4) ||
        storage[0] != 0xa5a5a5a5 || storage[4] != 0xa5a5a5a5) return 0;
    description.AppendParameter(&nodes[1]);
    if (list->first != &nodes[0].binding || list->last != &nodes[1].binding ||
        nodes[0].binding.next != &nodes[1].binding || nodes[1].binding.next ||
        nodes[1].previous_parameter != &nodes[0].binding) return 0;
    list->first = list->last = &sentinel.binding;
    description.AppendParameter(&nodes[2]);
    if (list->first != &nodes[2].binding || list->last != &nodes[2].binding ||
        nodes[2].previous_parameter != &sentinel.binding || nodes[2].binding.next ||
        (unsigned)sentinel.binding.next != 0x37373737) return 0;
    for (unsigned j = 0; j < 3; ++j)
        if (nodes[j].binding.name != &names[j] || nodes[j].flags != 0x5a ||
            nodes[j].kind != 0x5a || nodes[j].record_id != 0x5a5a5a5a) return 0;
    return description.kind == 6 && description.scope_active == 1;
}

extern "C" int c1_test_include_trailing_space(void)
{
    char bounded[] = "name \t_suffix";
    char complete[] = " a b \t";
    char unchanged[sizeof(bounded)];
    memcpy(unchanged, bounded, sizeof(bounded));
    return trim_include_name(bounded, bounded + 6) == bounded + 4 &&
        memcmp(bounded, unchanged, sizeof(bounded)) == 0 &&
        trim_include_name(complete, NULL) == complete + 4 &&
        strcmp(complete, " a b \t") == 0 &&
        trim_include_name(bounded, bounded) == bounded;
}

extern "C" int c1_test_diagnostic_text_lifetime(void)
{
    if (c1_data.saved_context_records || c1_data.saved_context_count ||
        c1_data.diagnostic_context_depth || c1_data.next_diagnostic_text_handle != 100)
        return 0;
    unsigned first = save_type_context();
    CompilerSavedContextRecord *rows = c1_data.saved_context_records;
    if (first != 100 || c1_data.diagnostic_context_depth != 1 ||
        c1_data.saved_context_count != 64 || rows[0].handle != first ||
        rows[0].capacity != 128 || rows[0].prefix_offset != 64 ||
        rows[0].suffix_offset != 65) return 0;
    for (unsigned i = 0; i < 128; ++i)
        if (((BYTE *)rows[0].allocation)[i]) return 0;
    for (unsigned j = 1; j < 64; ++j)
        if (rows[j].handle || rows[j].allocation || rows[j].prefix_offset ||
            rows[j].suffix_offset || rows[j].capacity) return 0;
    memset(rows[0].allocation, 0xa5, 128);
    void *original_allocation = rows[0].allocation;
    if (save_type_context() != 101 || c1_data.diagnostic_context_depth != 2)
        return 0;
    restore_type_context();
    if (c1_data.diagnostic_context_depth != 1 || rows[0].handle != 100 ||
        rows[1].handle != 101) return 0;
    restore_type_context();
    if (c1_data.diagnostic_context_depth || rows[0].handle || rows[1].handle ||
        rows[0].allocation != original_allocation ||
        ((BYTE *)rows[0].allocation)[127] != 0xa5) return 0;
    if (create_diagnostic_text() != 102 || rows[0].allocation != original_allocation)
        return 0;
    for (unsigned k = 0; k < 128; ++k)
        if (((BYTE *)rows[0].allocation)[k]) return 0;
    cleanup_saved_contexts();
    c1_data.saved_context_records = NULL;
    c1_data.saved_context_count = 0;
    return 1;
}

/* Independent factory contract: two interface subobjects, and the recorded
 * stream's initially empty token buffer. No pending virtual method is called. */
extern "C" int c1_test_input_factory(void)
{
    CompilerInput *input = NULL;
    CompilerTextInput *text = NULL;
    int ok;
    if (create_input(0, &input, &text) != 1 || !input || !text)
        return 0;
    ok = (BYTE *)text == (BYTE *)input + 4 &&
        *(void **)input != NULL && *(void **)text != NULL &&
        *(void **)input != *(void **)text;
    recon_cpp_delete(input);
    if (!ok || create_input(1, &input, &text) != 1 || !input || !text)
        return 0;
    CompilerRecordedInput *recorded = (CompilerRecordedInput *)input;
    ok = (BYTE *)text == (BYTE *)input + 4 &&
        *(void **)input != NULL && *(void **)text != NULL &&
        *(void **)input != *(void **)text &&
        recorded->token_bytes == (BYTE *)-1 && recorded->token_length == 0;
    /* token_start is deliberately neither read nor assigned here. */
    recon_cpp_delete(input);
    return ok;
}

extern "C" void __cdecl begin_browser_output(const char *);
extern "C" int c1_test_browser_factory(void)
{
    static const unsigned empty_slots[] = { 1, 2, 3, 4, 6, 11, 13, 14, 16, 22 };
    static const unsigned argument_slots[] = { 7, 9, 12, 15 };
    const char filename[] = "owned-browser.sbr";
    unsigned owned_marker = 0, i;
    CompilerBackend *default_object;
    void **table;
    char *copy;
    c1_data.browser_mode = 17;
    c1_data.named_browser = (CompilerBackend *)&owned_marker;
    begin_browser_output(NULL);
    default_object = c1_data.initial_backend;
    if (!default_object || c1_data.browser_mode != 17 ||
        c1_data.named_browser != (CompilerBackend *)&owned_marker) return 0;
    table = *(void ***)default_object;
    if (!table || !table[0] || !table[5] || table[0] == table[5]) return 0;
    for (i = 0; i < sizeof(empty_slots) / sizeof(empty_slots[0]); ++i)
        if (table[empty_slots[i]] != table[0]) return 0;
    for (i = 0; i < sizeof(argument_slots) / sizeof(argument_slots[0]); ++i)
        if (table[argument_slots[i]] != table[5]) return 0;
    begin_browser_output(filename);
    if (!c1_data.named_browser || c1_data.browser_mode != 3 ||
        c1_data.initial_backend != default_object) return 0;
    /* Construction leaves the saved-position field unwritten. Give that
     * field a test-owned value before the ordinary non-reuse writer. */
    copy = *(char **)((BYTE *)c1_data.named_browser + 4);
    if (!copy || copy == filename) return 0;
    for (i = 0; i < sizeof(filename); ++i)
        if (copy[i] != filename[i]) return 0;
    *(long *)((BYTE *)c1_data.named_browser + 8) = 0x12345678;
    c1_data.browser_mode = 1;
    c1_data.named_browser->Slot5c();
    if (*(long *)((BYTE *)c1_data.named_browser + 8) != -1 ||
        *(char **)((BYTE *)c1_data.named_browser + 4) != copy) return 0;
    recon_free(copy); /* _strdup owns a CRT allocation, not an operator-new block. */
    recon_cpp_delete(c1_data.named_browser);
    recon_cpp_delete(default_object);
    return 1;
}

extern "C" int __cdecl check_browser_symbol_generation(CompilerBrowserRecord *, CompilerIrSymbol *);
extern "C" int c1_test_browser_generation_records(void)
{
    static const struct { BYTE kind, storage, category; WORD attributes; } rows[] = {
        { 3, 0, 17, 0x60 }, { 4, 0, 8, 0x20 },
        { 5, 0, 10, 0x20 }, { 7, 1, 1, 0x60 },
        { 10, 3, 4, 0x22 }, { 11, 4, 3, 0x20 }
    };
    CompilerNameEntry name;
    CompilerType type;
    CompilerIrSymbol symbol;
    CompilerBrowserRecord record, before;
    begin_browser_output("owned-generations.sbr");
    CompilerBackend *browser = c1_data.named_browser;
    char *filename = *(char **)((BYTE *)browser + 4);
    c1_data.backend = browser;
    c1_data.browser_generation = 7;
    c1_data.next_symbol_record_id = 0x1234;
    c1_bss.parameter_nesting = 0;
    memset(&record, 0xa5, sizeof(record));
    memset(&symbol, 0x96, sizeof(symbol));
    symbol.record_id = 0x07123456;
    before = record;
    if (check_browser_symbol_generation(&record, &symbol) ||
        memcmp(&record, &before, sizeof(record)) ||
        c1_data.next_symbol_record_id != 0x1234) return 0;
    memset(&name, 0x97, sizeof(name));
    name.name = (char *)"owned-declaration";
    name.flags = 0;
    memset(&type, 0x69, sizeof(type));
    type.flags = 0x402;
    for (unsigned i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        memset(&symbol, 0x96, sizeof(symbol));
        symbol.kind = rows[i].kind;
        symbol.flags = 0;
        symbol.storage_class = rows[i].storage;
        symbol.binding.name = &name;
        symbol.value_type = &type;
        symbol.record_id = 0;
        memset(&record, 0xa5, sizeof(record));
        record.kind = 4;
        record.category = 0;
        record.line = 17;
        record.symbol_id = 0;
        record.name = NULL;
        record.identifier = NULL;
        record.attributes = 0;
        if (check_browser_symbol_generation(&record, &symbol) != 1 ||
            record.kind != 3 || record.category != rows[i].category ||
            record.attributes != rows[i].attributes || record.line != 17 ||
            record.symbol_id != 0x07001234U + i ||
            symbol.record_id != record.symbol_id || record.name != name.name ||
            record.identifier != &name || record.unmodeled_02[0] != 0xa5 ||
            record.unmodeled_02[1] != 0xa5 || record.unmodeled_16[0] != 0xa5 ||
            record.unmodeled_16[1] != 0xa5 ||
            c1_data.next_symbol_record_id != 0x1235U + i) return 0;
    }
    /* Local kind12 is suppressed by its actual DWORD column cell. It must
     * not reach its unrecovered category byte or the ID/name/type consumers. */
    memset(&symbol, 0x96, sizeof(symbol));
    symbol.kind = 12;
    symbol.flags = 0;
    symbol.record_id = 0;
    CompilerIrSymbol symbol_before = symbol;
    memset(&record, 0xa5, sizeof(record));
    before = record;
    before.kind = 20;
    unsigned count_before = c1_data.next_symbol_record_id;
    if (check_browser_symbol_generation(&record, &symbol) != 1 ||
        memcmp(&record, &before, sizeof(record)) ||
        memcmp(&symbol, &symbol_before, sizeof(symbol)) ||
        c1_data.next_symbol_record_id != count_before) return 0;
    recon_free(filename);
    recon_cpp_delete(browser);
    return 1;
}

extern "C" int c1_test_browser_scope_markers(void)
{
    FILE stream, stream_before;
    BYTE output[16];
    begin_browser_output("owned-scopes.sbr");
    CompilerBackend *browser = c1_data.named_browser;
    char *filename = *(char **)((BYTE *)browser + 4);
    memset(&stream, 0x69, sizeof(stream));
    memset(output, 0xa5, sizeof(output));
    stream._ptr = (char *)output + 3;
    stream._cnt = 10;
    stream_before = stream;
    c1_bss.browser_stream = &stream;
    c1_bss.current_context.value = 17;
    c1_data.browser_scope_depth = 0;
    browser->Slot0c();
    if (c1_data.browser_scope_depth != 1 || output[3] != 8) return 0;
    browser->Slot08();
    if (c1_data.browser_scope_depth || output[4] != 9) return 0;
    browser->Slot08();
    c1_data.browser_scope_depth = -7;
    browser->Slot08();
    if (c1_data.browser_scope_depth || stream._ptr != (char *)output + 5 ||
        stream._cnt != 8 || c1_data.browser_output_stream != &stream ||
        c1_bss.browser_stream != &stream || c1_bss.current_context.value != 17) return 0;
    stream_before._ptr = stream._ptr;
    stream_before._cnt = stream._cnt;
    if (memcmp(&stream, &stream_before, sizeof(stream))) return 0;
    for (unsigned i = 0; i < sizeof(output); ++i)
        if ((i < 3 || i >= 5) && output[i] != 0xa5) return 0;
    recon_free(filename);
    recon_cpp_delete(browser);
    return 1;
}

extern "C" int c1_test_browser_symbol_ids(void)
{
    static const unsigned values[] = { 0, 0x0100abcdU, 0x07001234U, 0x55, 0x07000000U, 0xff000000U, 0x00fffffeU };
    static const unsigned expected[] = { 0x07001234U, 0x0700abcdU, 0x07001234U, 0x07000055U, 0x07000000U, 0x07001235U, 0x07fffffeU };
    begin_browser_output("owned-ids.sbr");
    CompilerBackend *browser = c1_data.named_browser;
    char *filename = *(char **)((BYTE *)browser + 4);
    BYTE before[12];
    *(long *)((BYTE *)browser + 8) = 0x12345678;
    memcpy(before, browser, sizeof(before));
    c1_data.browser_generation = 7;
    c1_data.next_symbol_record_id = 0x1234;
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        unsigned owned[3] = { 0xa5a5a5a5U, 0x96969696U, 0x5a5a5a5aU };
        unsigned *result = browser->SetSymbolId(owned + 1, values[i]);
        if (result != owned + 1 || owned[1] != expected[i] ||
            owned[0] != 0xa5a5a5a5U || owned[2] != 0x5a5a5a5aU ||
            c1_data.next_symbol_record_id != (i >= 5 ? 0x1236U : 0x1235U) ||
            memcmp(browser, before, sizeof(before))) return 0;
    }
    c1_data.browser_generation = 0;
    unsigned result;
    if (browser->SetSymbolId(&result, 0) != &result || result ||
        c1_data.next_symbol_record_id != 0x1236U) return 0;
    c1_data.browser_generation = 7;
    CompilerIrSymbol symbol, symbol_before;
    memset(&symbol, 0x96, sizeof(symbol));
    symbol.record_id = 0x07123456U;
    symbol_before = symbol;
    if (browser->SymbolId(&symbol) != 0x123456U ||
        memcmp(&symbol, &symbol_before, sizeof(symbol)) ||
        memcmp(browser, before, sizeof(before)) ||
        c1_data.next_symbol_record_id != 0x1236U) return 0;
    recon_free(filename);
    recon_cpp_delete(browser);
    return 1;
}

extern "C" CompilerBackend *__cdecl acquire_initial_backend(void);
extern "C" int c1_test_backend_getter(void)
{
    unsigned owned_marker = 0;
    CompilerBackend *first, *second;
    begin_browser_output(NULL);
    first = c1_data.initial_backend;
    if (!first || acquire_initial_backend() != first) return 0;
    c1_data.initial_backend = NULL;
    c1_data.browser_mode = 37;
    c1_data.named_browser = (CompilerBackend *)&owned_marker;
    second = acquire_initial_backend();
    if (!second || second == first || c1_data.initial_backend != second ||
        *(void **)second != *(void **)first || c1_data.browser_mode != 37 ||
        c1_data.named_browser != (CompilerBackend *)&owned_marker) return 0;
    recon_cpp_delete(first);
    recon_cpp_delete(second);
    return 1;
}

extern "C" void __cdecl intern_identifier(const char *);
extern "C" int c1_test_identifier_interning(void)
{
    CompilerNameEntry *buckets[2048] = { 0 };
    DWORD records[15] = { 0 };
    unsigned char names[64];
    CompilerNameEntry *first = (CompilerNameEntry *)records;
    CompilerNameEntry *second = (CompilerNameEntry *)((BYTE *)records + 20);
    memset(names, 0xa5, sizeof(names));
    c1_data.name_buckets = buckets;
    c1_bss.pools[6].cursor = (BYTE *)records;
    c1_bss.pools[6].remaining = sizeof(records);
    c1_bss.arenas[0].cursor = names;
    c1_bss.arenas[0].committed_limit = names + sizeof(names);
    first->flags = 0x9abc0000;
    ((CompilerKeywordEntry *)first)->token = 77;
    ((CompilerKeywordEntry *)first)->category = 88;
    intern_identifier("aa");
    if (c1_data.current_identifier != first || strcmp(first->name, "aa") ||
        first->flags != 0x9abc01eb || ((CompilerKeywordEntry *)first)->token != 77 || ((CompilerKeywordEntry *)first)->category != 88)
        return 0;
    intern_identifier("b]"); /* Same hash491, different spelling. */
    if (c1_data.current_identifier != second || second->next != first ||
        buckets[491] != second || strcmp(second->name, "b]")) return 0;
    intern_identifier("aa");
    return c1_data.current_identifier == first && buckets[491] == first &&
        first->next == second && !second->next &&
        c1_bss.pools[6].cursor == (BYTE *)records + 40 &&
        c1_bss.pools[6].remaining == 20 &&
        c1_bss.arenas[0].cursor == names + 8 && names[3] == 0xa5 &&
        names[7] == 0xa5 && first->flags == 0x9abc01eb;
}

extern "C" int c1_test_output_ensure(void)
{
    BYTE bytes[8];
    CompilerOutputBuffer buffer;
    buffer.data = bytes;
    buffer.cursor = bytes;
    buffer.capacity = sizeof(bytes);
    buffer.good = 0x5a;
    if (buffer.Ensure(-1) || !buffer.Ensure(8) || buffer.cursor != bytes ||
        buffer.good != 0x5a) return 0;
    buffer.cursor = bytes + 8;
    if (!buffer.Ensure(0) || buffer.cursor != bytes + 8 || buffer.good != 0x5a)
        return 0;
    buffer.cursor = bytes;
    buffer.good = 7;
    if (buffer.Reserve(7) != 1 || buffer.good != 1 || buffer.cursor != bytes)
        return 0;
    buffer.good = 6;
    if (buffer.Reserve(-1) != 0 || buffer.good != 0 || buffer.cursor != bytes)
        return 0;
    buffer.good = 7;
    if (buffer.Reserve(0x7fffffff) != 0 || buffer.good != 0 || buffer.cursor != bytes)
        return 0;
    buffer.good = 7;
    if (buffer.Reserve(-2) || buffer.good || buffer.cursor != bytes) return 0;
    memset(bytes, 0xa5, sizeof(bytes));
    buffer.cursor = bytes + 1;
    buffer.good = 6;
    if (buffer.WriteBytes("123456", 6) != 1 || buffer.cursor != bytes + 7 ||
        buffer.good != 6 || memcmp(bytes + 1, "123456", 6) ||
        bytes[0] != 0xa5 || bytes[7] != 0xa5) return 0;
    return buffer.WriteBytes("", 0) == 1 && buffer.cursor == bytes + 7 &&
        buffer.good == 6 && bytes[7] == 0xa5;
}

struct GrowthObservation {
    CompilerOutputBuffer *buffer;
    BYTE *old_data, *new_data;
    int calls, published;
};
static void __cdecl observe_output_growth(void *context, BYTE *old_data, BYTE *new_data)
{
    GrowthObservation *observation = (GrowthObservation *)context;
    ++observation->calls;
    observation->old_data = old_data;
    observation->new_data = new_data;
    observation->published = observation->buffer->data == new_data &&
        observation->buffer->cursor == new_data + 3;
}
extern "C" int c1_test_output_grow(void)
{
    CompilerOutputBuffer buffer;
    GrowthObservation observation;
    BYTE *first;
    unsigned i;
    int ok;
    memset(&buffer, 0, sizeof(buffer));
    if (!buffer.Grow(2) || buffer.capacity != 8192 || buffer.cursor != buffer.data)
        return 0;
    first = buffer.data;
    for (i = 0; i < 8192; ++i)
        if (first[i]) { ::operator delete(first); return 0; }
    first[0] = 'a'; first[1] = 'b'; first[2] = 'c'; first[4] = 0xff;
    buffer.cursor += 3;
    memset(&observation, 0, sizeof(observation));
    observation.buffer = &buffer;
    buffer.relocated = observe_output_growth;
    buffer.relocation_context = &observation;
    if (!buffer.Grow(10000)) { ::operator delete(buffer.data); return 0; }
    ok = buffer.capacity == 24576 && observation.calls == 1 && observation.published &&
        observation.old_data == first && observation.new_data == buffer.data &&
        buffer.data[0] == 'a' && buffer.data[1] == 'b' && buffer.data[2] == 'c';
    for (i = 3; i < (unsigned)buffer.capacity; ++i)
        if (buffer.data[i]) ok = 0;
    ::operator delete(buffer.data);
    return ok;
}

extern "C" int c1_test_output_flush_suppression(void)
{
    BYTE bytes[4] = { 0x51, 0x72, 0x93, 0xb4 };
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    unsigned mode;
    buffer->data = bytes;
    buffer->capacity = sizeof(bytes);
    buffer->good = 0;
    buffer->stream = NULL;
    for (mode = 0; mode != 4; ++mode) {
        c1_bss.error_count = mode == 0;
        c1_bss.options.warnings_are_errors = mode == 1 ? 2 : 0;
        c1_bss.options.preprocess_stdout = mode == 2;
        c1_bss.options.prototypes_only = mode == 3;
        buffer->cursor = bytes + 3;
        buffer->Flush();
        if (buffer->cursor != bytes || buffer->good ||
            bytes[0] != 0x51 || bytes[1] != 0x72 ||
            bytes[2] != 0x93 || bytes[3] != 0xb4) return 0;
    }
    c1_bss.options.prototypes_only = 0;
    buffer->Flush(); /* An empty buffer needs neither a stream nor good state. */
    return buffer->cursor == bytes;
}

extern "C" CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
extern "C" int c1_test_returning_interner(void)
{
    CompilerNameEntry *buckets[2048] = { 0 };
    DWORD records[10];
    BYTE names[16];
    CompilerNameEntry *first, *second;
    unsigned i;
    for (i = 0; i != 10; ++i) records[i] = 0xa5a5a5a5;
    memset(names, 0xa5, sizeof(names));
    c1_data.name_buckets = buckets;
    c1_bss.pools[4].cursor = (BYTE *)(records + 1);
    c1_bss.pools[4].remaining = 32;
    c1_bss.arenas[0].cursor = names;
    c1_bss.arenas[0].committed_limit = names + sizeof(names);
    first = find_or_create_identifier("aa");
    second = find_or_create_identifier("b]");
    if ((BYTE *)first != (BYTE *)(records + 1) ||
        (BYTE *)second != (BYTE *)(records + 5) || first == second ||
        second->next != first || buckets[491] != second ||
        first->flags != 0xa5a501eb || second->flags != 0xa5a501eb ||
        records[4] != 0xa5a5a5a5 || records[8] != 0xa5a5a5a5) return 0;
    return find_or_create_identifier("aa") == first && c1_data.current_identifier == first &&
        first->next == second && !second->next && buckets[491] == first &&
        c1_bss.pools[4].remaining == 0 &&
        c1_bss.pools[4].cursor == (BYTE *)(records + 9) &&
        c1_bss.arenas[0].cursor == names + 8 && !strcmp(first->name, "aa") &&
        !strcmp(second->name, "b]") && names[3] == 0xa5 && names[7] == 0xa5 &&
        records[0] == 0xa5a5a5a5 && records[9] == 0xa5a5a5a5;
}

extern "C" CompilerNameEntry *__cdecl intern_name_span(const char *, unsigned, unsigned, int *);
extern "C" int c1_test_name_span_interner(void)
{
    CompilerNameEntry *buckets[2048] = { 0 };
    DWORD records[10];
    BYTE names[16];
    CompilerNameEntry *first, *second;
    int created = 37;
    unsigned i;
    for (i = 0; i != 10; ++i) records[i] = 0xa5a5a5a5;
    memset(names, 0x96, sizeof(names));
    c1_data.name_buckets = buckets;
    c1_bss.pools[4].cursor = (BYTE *)(records + 1);
    c1_bss.pools[4].remaining = 32;
    c1_bss.arenas[0].cursor = names;
    c1_bss.arenas[0].committed_limit = names + sizeof(names);
    first = intern_name_span("aa-tail", 0x1eb, 2, &created);
    if (created != 1 || strcmp(first->name, "aa")) return 0;
    second = intern_name_span("b]-tail", 0x1eb, 2, &created);
    if (created != 1 || second->next != first || buckets[491] != second ||
        first->flags != 0xa5a501eb || second->flags != 0xa5a501eb ||
        records[4] != 0xa5a5a5a5 || records[8] != 0xa5a5a5a5) return 0;
    if (intern_name_span("aa-more", 0x1eb, 2, &created) != first || created != 0)
        return 0;
    return c1_data.current_identifier == first && first->next == second &&
        !second->next && buckets[491] == first &&
        c1_bss.pools[4].remaining == 0 &&
        c1_bss.pools[4].cursor == (BYTE *)(records + 9) &&
        c1_bss.arenas[0].cursor == names + 8 && names[3] == 0x96 && names[7] == 0x96 &&
        records[0] == 0xa5a5a5a5 && records[9] == 0xa5a5a5a5;
}

extern "C" int c1_test_source_file_write_extent(void)
{
    CompilerTranslationUnit object;
    CompilerSourceFile predecessor;
    CompilerNameEntry *buckets[2048] = { 0 };
    DWORD record[4];
    DWORD bytes[16];
    unsigned i;
    memset(&object, 0x96, sizeof(object));
    memset(&predecessor, 0x87, sizeof(predecessor));
    memset(record, 0xa5, sizeof(record));
    memset(bytes, 0x78, sizeof(bytes));
    object.source_files = &predecessor;
    c1_data.name_buckets = buckets;
    c1_bss.pools[4].cursor = (BYTE *)record;
    c1_bss.pools[4].remaining = sizeof(record);
    c1_bss.arenas[0].cursor = (BYTE *)bytes;
    c1_bss.arenas[0].committed_limit = (BYTE *)(bytes + 16);
    CompilerSourceFile *file = object.FindSourceFile("aa", 0x1eb);
    if ((void *)file != bytes + 1 || file->name != (void *)record || file->kind != 21 ||
        file->record_id != 0x78787878 || file->next != &predecessor ||
        object.source_files != file || c1_bss.arenas[0].cursor != (BYTE *)(bytes + 6))
        return 0;
    c1_data.c9_il = 1;
    if (file->OutputIndex() != 0x787878 || file->record_id != 0x78787878) return 0;
    for (i = 5; i < 12; ++i)
        if (((BYTE *)file)[i] != 0x78) return 0;
    for (i = 6; i < 16; ++i)
        if (bytes[i] != 0x78787878) return 0;
    return predecessor.record_id == 0x87878787 &&
        (unsigned)object.symbols == 0x96969696 && (unsigned)object.root == 0x96969696;
}

extern "C" int c1_test_translation_unit(void)
{
    CompilerTranslationUnit object;
    CompilerSymbolTable *root;
    DWORD storage[1032];
    unsigned i;
    memset(&object, 0xa5, sizeof(object));
    for (i = 0; i < 1032; ++i) storage[i] = 0x96969696;
    c1_bss.pools[7].cursor = (BYTE *)(storage + 1);
    c1_bss.pools[7].remaining = 4120;
    if (object.Initialize() != &object || !object.root ||
        object.current != object.root || object.scopes[0] != object.root ||
        object.depth || object.lookup_mode || object.retired_scopes ||
        object.symbols || object.source_files) return 0;
    root = object.root;
    if ((void *)root != storage + 1 || !*(void **)root || root->next || root->level ||
        root->outer || root->bucket_mask != 1023 || (void *)root->buckets != storage + 7 ||
        c1_bss.pools[7].cursor != (BYTE *)(storage + 1031) ||
        c1_bss.pools[7].remaining) return 0;
    for (i = 1; i < 128; ++i)
        if ((unsigned)object.scopes[i] != 0xa5a5a5a5) return 0;
    for (i = 7; i < 1032; ++i) if (storage[i] != 0x96969696) return 0;
    return storage[0] == 0x96969696;
}

extern "C" CompilerIrSymbol *__cdecl make_abstract_declarator(void);
extern "C" int c1_test_abstract_symbol_reuse(void)
{
    DWORD owned[15];
    BYTE payload[18];
    CompilerNameEntry name;
    CompilerSymbolBinding retained;
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)(owned + 1);
    unsigned i;
    for (i = 0; i < 15; ++i) owned[i] = 0xa5a5a5a5;
    memset(payload, 0x96, sizeof(payload));
    symbol->binding.next = &retained;
    c1_data.abstract_symbol_free = &symbol->binding;
    c1_data.builtin_names.abstract_declarator = &name;
    c1_data.record_sizes.known.abstract_symbol_bytes = 52;
    c1_data.abstract_symbol_cache_index = 9;
    c1_bss.declaration_nesting = 0;
    c1_bss.arenas[3].cursor = payload + 1;
    c1_bss.arenas[3].committed_limit = payload + 17;
    if (make_abstract_declarator() != symbol || symbol->kind != 13 ||
        symbol->binding.name != &name || symbol->binding.next ||
        c1_data.abstract_symbol_free != &retained ||
        (void *)symbol->abstract_type != payload + 1 ||
        c1_bss.arenas[3].cursor != payload + 17) return 0;
    for (i = 0; i < 52; ++i)
        if (i != 0 && !(i >= 8 && i < 12) && !(i >= 16 && i < 20) &&
            ((BYTE *)symbol)[i] != 0) return 0;
    for (i = 0; i < sizeof(payload); ++i) if (payload[i] != 0x96) return 0;
    return owned[0] == 0xa5a5a5a5 && owned[14] == 0xa5a5a5a5;
}

extern "C" int c1_test_symbol_constructor(void)
{
    CompilerIrSymbol symbol;
    CompilerNameEntry name;
    unsigned pass, i;
    for (pass = 0; pass != 2; ++pass) {
        unsigned kind = pass ? 4 : 13;
        memset(&symbol, 0xa5, sizeof(symbol));
        if (symbol.Initialize(kind, &name, 3) != &symbol || symbol.kind != kind ||
            symbol.binding.name != &name || symbol.binding.next) return 0;
        if (pass && symbol.kind4_context) return 0;
        for (i = 0; i < sizeof(symbol); ++i) {
            if (i == 0 || (i >= 8 && i < 16) || (pass && i >= 48)) continue;
            if (((BYTE *)&symbol)[i] != 0xa5) return 0;
        }
    }
    return 1;
}

extern "C" int c1_test_append_type_shape(void)
{
    CompilerDeclarator declarator;
    CompilerTypeShape previous, next;
    unsigned marker;
    memset(&declarator, 0, sizeof(declarator));
    memset(&previous, 0, sizeof(previous));
    memset(&next, 0, sizeof(next));
    next.flags = 0xffff;
    next.value = 37;
    declarator.shape_flags = 0x80;
    declarator.flags = 0x400;
    declarator.AppendShape(&next);
    if (declarator.shape_flags != 0x81 || declarator.shape || declarator.last_shape ||
        declarator.flags != 0x400) return 0;
    previous.flags = 1;
    previous.base = (CompilerTypeShape *)&marker;
    previous.value = 83;
    declarator.shape = &previous;
    declarator.last_shape = &previous;
    declarator.AppendShape(&next);
    return previous.flags == 0xc803 && previous.attributes == 0 &&
        previous.base == (CompilerTypeShape *)&marker && previous.value == 83 &&
        declarator.shape == &previous && declarator.last_shape == &previous &&
        declarator.shape_flags == 0x81 && declarator.flags == 0x400 &&
        next.flags == 0xffff && next.value == 37;
}

extern "C" int c1_test_arena_byte_allocation(void)
{
    BYTE storage[23];
    unsigned i;
    for (i = 0; i < sizeof(storage); ++i) storage[i] = 0xa5;
    CompilerArena *arena = &c1_bss.arenas[0];
    arena->cursor = storage + 1;
    arena->committed_limit = storage + 21;
    if (arena->AllocateBytes(7) != storage + 1 || arena->cursor != storage + 8 ||
        arena->AllocateBytes(13) != storage + 8 || arena->cursor != storage + 21)
        return 0;
    for (i = 0; i < sizeof(storage); ++i)
        if (storage[i] != 0xa5) return 0;
    return 1;
}

extern "C" int c1_test_scope_binding_chains(void)
{
    CompilerSymbolTable scope;
    CompilerSymbolBinding *buckets[8] = { 0 };
    CompilerIrSymbol first, second;
    CompilerNameEntry name1, name2;
    memset(&first, 0xa5, sizeof(first));
    memset(&second, 0x96, sizeof(second));
    name1.flags = 0x10003;
    name2.flags = 0x20003;
    first.binding.name = &name1;
    second.binding.name = &name2;
    scope.buckets = buckets;
    scope.bucket_mask = 7;
    scope.Link(&first);
    scope.Link(&second);
    if (buckets[3] != &second.binding || second.binding.next != &first.binding ||
        first.binding.next || first.binding.name != &name1 || second.binding.name != &name2 ||
        first.record_id != 0xa5a5a5a5 || second.record_id != 0x96969696)
        return 0;
    unsigned i;
    for (i = 0; i < 8; ++i)
        if (i != 3 && buckets[i]) return 0;
    return 1;
}

extern "C" void __cdecl define_preprocessor_macro(const char *, unsigned, int);
extern "C" int c1_test_macro_definition_extent(void)
{
    DWORD descriptors[4];
    BYTE body_bytes[16];
    CompilerNameEntry name;
    unsigned i;
    memset(descriptors, 0x96, sizeof(descriptors));
    memset(body_bytes, 0xa5, sizeof(body_bytes));
    memset(&name, 0, sizeof(name));
    name.name = (char *)"owned_macro";
    name.flags = 0x12040000;
    c1_data.current_identifier = &name;
    c1_data.source_include_depth = -1;
    c1_data.backend = acquire_initial_backend();
    c1_data.browser_mode = 0;
    c1_bss.pools[5].cursor = (BYTE *)(descriptors + 1);
    c1_bss.pools[5].remaining = 8;
    c1_bss.arenas[0].cursor = body_bytes + 1;
    c1_bss.arenas[0].committed_limit = body_bytes + 13;
    define_preprocessor_macro("abc", 3, -3);
    CompilerMacro *macro = (CompilerMacro *)(descriptors + 1);
    if (name.macro != macro || name.flags != (0x12040000 | 0x80000) ||
        macro->identifier != &name || (BYTE *)macro->body != body_bytes + 1 ||
        macro->body->kind != -3 || memcmp(macro->body->payload, "abc", 3) ||
        c1_bss.pools[5].cursor != (BYTE *)(descriptors + 3) ||
        c1_bss.pools[5].remaining || c1_bss.arenas[0].cursor != body_bytes + 13 ||
        descriptors[0] != 0x96969696 || descriptors[3] != 0x96969696) return 0;
    for (i = 0; i < sizeof(body_bytes); ++i)
        if ((i < 5 || i > 8) && body_bytes[i] != 0xa5) return 0;
    c1_data.browser_mode = 3;
    c1_data.backend->Slot28(macro);
    return name.flags == (0x12040000 | 0x80000 | 0x200000) &&
        macro->body->kind == -3 && !memcmp(macro->body->payload, "abc", 3);
}

extern "C" int c1_test_context_buffer_initialize(void)
{
    DWORD words[34];
    CompilerContextBuffer buffer;
    unsigned i;
    memset(words, 0xa5, sizeof(words));
    memset(&buffer, 0x96, sizeof(buffer));
    c1_bss.arenas[2].cursor = (BYTE *)(words + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(words + 33);
    if (buffer.Initialize(2) != &buffer || (void *)buffer.block != words + 1 ||
        buffer.block->next || buffer.position != 4 || buffer.arena_kind != 2 ||
        c1_bss.arenas[2].cursor != (BYTE *)(words + 33)) return 0;
    for (i = 0; i < 34; ++i)
        if (i != 1 && words[i] != 0xa5a5a5a5) return 0;
    return 1;
}

extern "C" int c1_test_context_push_alias(void)
{
    DWORD buffer_words[32], frame_words[8];
    CompilerContextFrame predecessor;
    unsigned old_object = 0x1234, new_object = 0x5678;
    unsigned i;
    memset(buffer_words, 0xa5, sizeof(buffer_words));
    memset(frame_words, 0x96, sizeof(frame_words));
    memset(&predecessor, 0x87, sizeof(predecessor));
    c1_bss.arenas[1].cursor = (BYTE *)buffer_words;
    c1_bss.arenas[1].committed_limit = (BYTE *)(buffer_words + 32);
    CompilerLexer lexer(&new_object, 19);
    lexer.arena_kind = 2;
    c1_bss.arenas[2].cursor = (BYTE *)(frame_words + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(frame_words + 7);
    c1_bss.current_context.object = &old_object;
    c1_bss.current_context.value = 83;
    c1_bss.current_context.saved_frame = &predecessor;
    c1_bss.current_context.Push(&lexer, 6, 37, 41, &c1_bss.current_context);
    CompilerContextFrame *frame = c1_bss.current_context.saved_frame;
    if ((void *)frame != frame_words + 1 || frame->stream != &lexer ||
        frame->next != &predecessor || frame->object != &old_object || frame->value != 83 ||
        frame->first != 37 || frame->second != 41 ||
        c1_bss.current_context.object != &new_object || c1_bss.current_context.value != 19 ||
        c1_bss.arenas[2].cursor != (BYTE *)(frame_words + 7)) return 0;
    for (i = 1; i < 32; ++i) if (buffer_words[i] != 0xa5a5a5a5) return 0;
    return frame_words[0] == 0x96969696 && frame_words[7] == 0x96969696 &&
        (unsigned)predecessor.stream == 0x87878787 && predecessor.second == 0x87878787;
}

extern "C" int __cdecl create_debug_module(struct PDB *, int, struct NameMap *,
    const char *, const char *, CompilerTextInput *, int, int, int, CompilerDebugModule **);
extern "C" int c1_test_null_debug_factory(void)
{
    /* Identity groups correspond to the actual physical methods, not to their
     * source spellings. No pending virtual body is invoked by this test. */
    static const BYTE group[58] = {
        0,1,2,2,0,3,4,0,5,5,0,0,6,5,6,7,0,0,0,0,0,8,2,2,9,0,0,0,9,
        5,5,10,0,10,5,6,7,6,0,6,6,0,6,6,0,0,0,0,11,6,10,12,0,0,13,10,6,0
    };
    CompilerDebugModule *module = NULL;
    unsigned i, j;
    if (!create_debug_module(NULL, 0, NULL, NULL, NULL, NULL, 0, 0, 0, &module) ||
        !module) return 0;
    void **table = *(void ***)module;
    for (i = 0; i < 58; ++i) {
        if (!table[i]) return 0;
        for (j = 0; j < 58; ++j)
            if ((table[i] == table[j]) != (group[i] == group[j])) return 0;
    }
    recon_cpp_delete(module);
    return 1;
}

extern "C" int c1_test_parser_reset_extent(void)
{
    DWORD owned[2218];
    unsigned i;
    memset(owned, 0xa5, sizeof(owned));
    CompilerParser *parser = (CompilerParser *)(owned + 1);
    if (parser->Initialize() != parser || parser->lookahead || parser->state ||
        parser->syntax_errors || parser->recovery_shifts ||
        parser->state_top != (short *)((BYTE *)parser + 0x1ff6) ||
        parser->value_top != (CompilerParserStackValue *)parser ||
        parser->unmodeled_0e != 0xa5a5) return 0;
    for (i = 24; i < 8864; ++i)
        if (((BYTE *)parser)[i] != 0xa5) return 0;
    return owned[0] == 0xa5a5a5a5 && owned[2217] == 0xa5a5a5a5;
}

extern "C" int c1_test_parameter_scope_layout(void)
{
    DWORD hash_storage[8], parameter_storage[7];
    unsigned i;
    for (i = 0; i < 8; ++i) hash_storage[i] = 0xa5a5a5a5;
    for (i = 0; i < 7; ++i) parameter_storage[i] = 0x5a5a5a5a;
    CompilerSymbolTable *hash = new(hash_storage + 1) CompilerSymbolTable;
    CompilerParameterScope *parameters = new(parameter_storage + 1) CompilerParameterScope(37);
    if (sizeof(*hash) != 24 || sizeof(*parameters) != 20 ||
        hash->next || hash->level || hash->outer || hash->bucket_mask != 1023 ||
        parameters->next || parameters->level != 1 || parameters->unmodeled_0c ||
        parameters->parameter_value != 37) return 0;
    /* The hash constructor leaves its bucket-pointer field unwritten. */
    if (hash_storage[5] != 0xa5a5a5a5 || hash_storage[0] != 0xa5a5a5a5 ||
        hash_storage[7] != 0xa5a5a5a5 || parameter_storage[0] != 0x5a5a5a5a ||
        parameter_storage[6] != 0x5a5a5a5a) return 0;
    void **hash_slots = *(void ***)hash;
    void **parameter_slots = *(void ***)parameters;
    if (hash_slots[0] != parameter_slots[0]) return 0;
    hash->buckets = (CompilerSymbolBinding **)(hash_storage + 1);
    if (hash->HasBuckets() != 1 || parameters->HasBuckets() != 1) return 0;
    hash->buckets = NULL;
    parameters->parameter_value = 0;
    if (hash->HasBuckets() != 0 || parameters->HasBuckets() != 0) return 0;
    /* The other nine genuine methods are distinct pending owners. */
    for (i = 1; i < 10; ++i) {
        if (!parameter_slots[i] || parameter_slots[i] == hash_slots[i]) return 0;
        for (unsigned j = 0; j < i; ++j)
            if (parameter_slots[i] == parameter_slots[j]) return 0;
    }
    return 1;
}

extern "C" int c1_test_output_small(void)
{
    BYTE storage[5] = {0xa5,0xa5,0xa5,0xa5,0xa5};
    CompilerOutputBuffer buffer;
    buffer.cursor = storage + 1;
    if (buffer.WriteSmall(128) || buffer.WriteSmall(0xffffffff) || buffer.cursor != storage + 1)
        return 0;
    for (unsigned i = 0; i < 5; ++i) if (storage[i] != 0xa5) return 0;
    if (buffer.WriteSmall(0) != 1 || buffer.WriteSmall(127) != 1 || buffer.cursor != storage + 3)
        return 0;
    return storage[0] == 0xa5 && storage[1] == 0 && storage[2] == 127 &&
        storage[3] == 0xa5 && storage[4] == 0xa5;
}

extern "C" int c1_test_output_index(void)
{
    BYTE storage[20];
    static const BYTE expected[10] = {0,0,0xff,0x7f,0,0x80,1,0,0xff,0xff};
    CompilerOutputBuffer buffer;
    memset(storage, 0xa5, sizeof(storage));
    buffer.data = buffer.cursor = storage + 1;
    buffer.capacity = 18;
    buffer.good = 1;
    buffer.WriteIndex(0);
    buffer.WriteIndex(0x7fff);
    buffer.WriteIndex(0x8000);
    buffer.WriteIndex(0xffffffff);
    if (buffer.cursor != storage + 11 || buffer.good != 1 || memcmp(storage + 1, expected, 10)) return 0;
    buffer.good = 0;
    buffer.WriteIndex(1);
    if (buffer.cursor != storage + 11 || buffer.good != 0 || storage[0] != 0xa5) return 0;
    for (unsigned i = 11; i < sizeof(storage); ++i) if (storage[i] != 0xa5) return 0;
    return 1;
}

extern "C" int c1_test_output_short(void)
{
    BYTE storage[12];
    static const BYTE expected[8] = {0,127,0x80,0xff,0xff,0x80,0x80,0};
    CompilerOutputBuffer buffer;
    memset(storage, 0xa5, sizeof(storage));
    buffer.cursor = storage + 1;
    buffer.WriteShort(0);
    buffer.WriteShort(127);
    buffer.WriteShort(-1);
    buffer.WriteShort(128);
    return buffer.cursor == storage + 9 && !memcmp(storage + 1, expected, 8) &&
        storage[0] == 0xa5 && storage[9] == 0xa5 && storage[10] == 0xa5 && storage[11] == 0xa5;
}

extern "C" CompilerParserNode *__cdecl make_typed_binary_expression(unsigned, CompilerParserNode *, CompilerParserNode *, CompilerType *);
extern "C" int c1_test_binary_node_extent(void)
{
    DWORD storage[10];
    CompilerParserNode children[2];
    CompilerType type;
    memset(storage, 0xa5, sizeof(storage));
    memset(children, 0x96, sizeof(children));
    c1_data.expression_arena_kind = 2;
    c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 10);
    CompilerParserNode *node = make_typed_binary_expression(0x16c, &children[0], &children[1], &type);
    if ((void *)node != storage + 1 || c1_bss.arenas[2].cursor != (BYTE *)(storage + 7) ||
        node->kind != 108 || node->flags != 2 || node->type != &type ||
        node->left != &children[0] || node->right != &children[1]) return 0;
    if (((BYTE *)node)[2] != 0xa5 || ((BYTE *)node)[3] != 0xa5) return 0;
    for (unsigned i = 8; i < 16; ++i) if (((BYTE *)node)[i] != 0xa5) return 0;
    for (unsigned j = 7; j < 10; ++j) if (storage[j] != 0xa5a5a5a5) return 0;
    for (unsigned k = 0; k < sizeof(children); ++k) if (((BYTE *)children)[k] != 0x96) return 0;
    return storage[0] == 0xa5a5a5a5;
}

extern "C" int c1_test_block_scope_extent(void)
{
    DWORD storage[9];
    CompilerSymbolBinding *owned_bucket = NULL;
    CompilerSymbolTable outer(1, NULL);
    CompilerTranslationUnit context;
    memset(storage, 0xa5, sizeof(storage));
    storage[5] = (DWORD)&owned_bucket; /* Constructor must retain this field. */
    context.current = &outer;
    context.depth = 0;
    context.scopes[0] = &outer;
    c1_bss.parameter_nesting = 2;
    c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 9);
    context.EnterBlockScope();
    CompilerSymbolTable *scope = (CompilerSymbolTable *)(storage + 1);
    if (context.current != scope || scope->next != &outer || scope->outer != &outer ||
        scope->level != 2 || scope->bucket_mask != 15 || scope->buckets != &owned_bucket ||
        context.depth != 1 || context.scopes[0] != &outer || context.scopes[1] != scope ||
        c1_bss.arenas[2].cursor != (BYTE *)(storage + 7)) return 0;
    if (*(void **)scope != *(void **)&outer) return 0;
    for (unsigned i = 9; i < 12; ++i) if (((BYTE *)scope)[i] != 0xa5) return 0;
    context.retired_scopes = NULL;
    context.FinishScope(scope);
    if (context.retired_scopes != scope || scope->next || scope->outer != &outer) return 0;
    context.FinishScope(&outer);
    if (context.retired_scopes != &outer || outer.next != scope || scope->next ||
        scope->outer != &outer || scope->buckets != &owned_bucket || scope->bucket_mask != 15)
        return 0;
    return storage[0] == 0xa5a5a5a5 && storage[7] == 0xa5a5a5a5 && storage[8] == 0xa5a5a5a5;
}

extern "C" CompilerType *__cdecl select_based_literal_type(unsigned __int64 *);
extern "C" int c1_test_based_literal_boundaries(void)
{
    CompilerType types[5];
    static const unsigned __int64 values[] = {
        0,0x7fffffffULL,0x80000000ULL,0xffffffffULL,
        0x100000000ULL,0x7fffffffffffffffULL,0x8000000000000000ULL,0xffffffffffffffffULL
    };
    c1_data.builtin_types.type_0063 = types;
    c1_data.builtin_types.type_4063_first = types + 1;
    c1_data.builtin_types.type_4044 = types + 2;
    c1_data.builtin_types.type_004c = types + 3;
    c1_data.builtin_types.type_404c = types + 4;
    for (unsigned mode = 0; mode < 3; ++mode) {
        c1_data.internal_zb_value = mode == 1 ? 8 : 4;
        c1_data.internal_bm_flags = mode == 2;
        for (unsigned i = 0; i < sizeof(values)/sizeof(values[0]); ++i) {
            unsigned __int64 value = values[i];
            unsigned expected = i < 2 ? 0 : i < 4 ? 1 : !mode ? 2 : i < 6 ? 3 : 4;
            if (select_based_literal_type(&value) != types + expected || value != values[i]) return 0;
        }
    }
    return 1;
}

extern "C" int c1_test_arena_clear_used(void)
{
    BYTE bytes[32];
    CompilerArena arena, expected;
    unsigned i;
    memset(bytes, 0xa5, sizeof(bytes));
    memset(&arena, 0x96, sizeof(arena));
    arena.base = bytes + 3;
    arena.cursor = bytes + 12;
    arena.committed_limit = bytes + 29;
    expected = arena;
    expected.cursor = expected.base;
    arena.ClearUsed();
    if (memcmp(&arena, &expected, sizeof(arena))) return 0;
    for (i = 0; i < sizeof(bytes); ++i)
        if (bytes[i] != (i >= 3 && i < 12 ? 0 : 0xa5)) return 0;
    arena.ClearUsed();
    return !memcmp(&arena, &expected, sizeof(arena));
}

extern "C" void __cdecl check_finished_function(CompilerIrSymbol *);
extern "C" int c1_test_function_directory(void)
{
    struct { DWORD before; CompilerFunctionDirectoryBlock block; DWORD after; } owned;
    BYTE empty[2][4], encoded[40];
    CompilerIrSymbol function;
    CompilerOutputBuffer buffer;
    static const BYTE expected[10] = {0,0,0xff,0x7f,0,0x80,1,0,0xff,0xff};
    unsigned i;
    memset(&owned, 0xa5, sizeof(owned));
    memset(&function, 0x96, sizeof(function));
    c1_data.c9_il = 1;
    c1_data.debug_module_kind = 2;
    c1_data.options.pch_use = c1_data.options.pch_auto = c1_data.saved_pch_mode = 0;
    c1_bss.options.prototypes_only = 0;
    c1_bss.parser_early_result = c1_bss.options.prototypes_only = 0;
    c1_data.function_directory_head = c1_data.function_directory_current = NULL;
    c1_data.function_directory_index = 19;
    c1_data.function_record_flags = 0x1000;
    c1_data.function_debug_handle = 0x1234;
    c1_data.emit_global_locations = 1;
    c1_data.ir_symbol_state_ready = 1;
    c1_data.ir_symbol_state_changed = 0;
    c1_data.disable_inline_records = 1;
    c1_bss.function_expression_offset = 31;
    c1_bss.function_symbol_offset = 47;
    c1_bss.ir_record_count = 61;
    c1_bss.parameter_symbol_count = 73;
    c1_bss.nonil_type_index = 89;
    c1_bss.pools[1].cursor = (BYTE *)&owned.block;
    c1_bss.pools[1].remaining = 644;
    for (i = 0; i < 2; ++i) {
        c1_bss.output_buffers[i*2+1].data = c1_bss.output_buffers[i*2+1].cursor = empty[i];
        c1_bss.output_buffers[i*2+1].capacity = 4;
    }
    function.record_id = 22;
    c1_bss.current_scope_symbol = &function;
    memset(encoded, 0xa5, sizeof(encoded));
    c1_bss.output_buffers[4].data = c1_bss.output_buffers[4].cursor = encoded + 1;
    c1_bss.output_buffers[4].capacity = 38;
    c1_bss.output_buffers[4].good = 7;
    check_finished_function(&function);
    if (c1_bss.output_buffers[4].cursor != encoded + 4 ||
        c1_bss.output_buffers[4].good != 1 || encoded[1] != 9 ||
        encoded[2] != 22 || encoded[3] != 0 || encoded[0] != 0xa5) return 0;
    for (i = 4; i < sizeof(encoded); ++i) if (encoded[i] != 0xa5) return 0;
    if (c1_bss.output_buffers[3].cursor != empty[1]) return 0;
    CompilerFunctionDirectoryRecord *record = &owned.block.records[0];
    if (c1_data.function_directory_head != &owned.block ||
        c1_data.function_directory_current != &owned.block || c1_data.function_directory_index ||
        c1_bss.pools[1].cursor != (BYTE *)&owned.block + 644 || c1_bss.pools[1].remaining ||
        c1_data.function_record_flags != 0 || c1_bss.ir_record_count || c1_data.emit_global_locations ||
        record->binding != &function.binding || record->expression_position != 31 ||
        record->symbol_position != 47 || record->debug_handle != 0x1234 ||
        record->ir_records != 61 || record->parameter_symbols != 73 || record->context_index != 89 ||
        record->flags != ((0xa5a5a5a5U & ~0x3fU) | 0x25)) return 0;
    if (owned.before != 0xa5a5a5a5 || owned.after != 0xa5a5a5a5) return 0;
    /* The link and nineteen later records are not initialized by this call. */
    for (i = 0; i < 4; ++i) if (((BYTE *)&owned.block)[i] != 0xa5) return 0;
    for (i = 36; i < 644; ++i) if (((BYTE *)&owned.block)[i] != 0xa5) return 0;
    memset(encoded, 0xa5, sizeof(encoded));
    memset(&buffer, 0, sizeof(buffer));
    buffer.data = buffer.cursor = encoded + 1;
    buffer.capacity = 38;
    buffer.good = 7;
    buffer.WriteFunctionIndex(0);
    buffer.WriteFunctionIndex(0x7fff);
    buffer.WriteFunctionIndex(0x8000);
    buffer.WriteFunctionIndex(0xffffffff);
    if (buffer.good != 1 || buffer.cursor != encoded + 11 || memcmp(encoded + 1, expected, 10)) return 0;
    c1_data.c9_il = 0;
    buffer.WriteFunctionIndex(0);
    buffer.WriteFunctionIndex(0x3fff);
    buffer.WriteFunctionIndex(0x4000);
    buffer.WriteFunctionIndex(0x3fff80);
    static const BYTE compact[8] = {0x80,0,0xbf,0xff,0xc0,0,1,0};
    if (buffer.cursor != encoded + 19 || memcmp(encoded + 11, compact, 8)) return 0;
    buffer.good = 0;
    buffer.WriteFunctionIndex(1);
    if (buffer.cursor != encoded + 19 || encoded[0] != 0xa5) return 0;
    for (i = 19; i < sizeof(encoded); ++i) if (encoded[i] != 0xa5) return 0;
    return 1;
}

extern "C" int c1_test_recorded_trace(void)
{
    DWORD objects[18], blocks[66];
    BYTE prior[64];
    memset(objects, 0x96, sizeof(objects));
    memset(blocks, 0xa5, sizeof(blocks));
    c1_bss.arenas[2].cursor = (BYTE *)(blocks + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(blocks + 65);
    CompilerRecordedLexer *lexer = new(objects + 1) CompilerRecordedLexer(NULL, 0, 2);
    memcpy(prior, lexer, sizeof(prior));
    for (unsigned i = 0; i < 28; ++i) lexer->Trace(0x12340000 + i);
    CompilerContextBlock *first = (CompilerContextBlock *)(blocks + 1);
    CompilerContextBlock *second = (CompilerContextBlock *)(blocks + 33);
    if (first->next != second || second->next || lexer->start_buffer.block != second ||
        lexer->start_buffer.position != 5 || c1_bss.arenas[2].cursor != (BYTE *)(blocks + 65)) return 0;
    for (unsigned j = 0; j < 27; ++j) if (first->tokens[j] != 0x12340000 + j) return 0;
    if (second->tokens[0] != 0x1234001b) return 0;
    for (unsigned k = 1; k < 27; ++k) if (second->tokens[k] != 0xa5a5a5a5) return 0;
    for (unsigned n = 0; n < 16; ++n)
        if (first->unmodeled_04[n] != 0xa5 || second->unmodeled_04[n] != 0xa5) return 0;
    for (unsigned m = 0; m < 64; ++m)
        if ((m < 44 || m >= 52) && ((BYTE *)lexer)[m] != prior[m]) return 0;
    /* Existing next-block reuse writes no new allocation or header bytes. */
    lexer->start_buffer.block = first;
    lexer->start_buffer.position = 31;
    lexer->Trace(0x56789abc);
    if (lexer->start_buffer.block != second || lexer->start_buffer.position != 5 ||
        second->tokens[0] != 0x56789abc || c1_bss.arenas[2].cursor != (BYTE *)(blocks + 65)) return 0;
    lexer->read_buffer.block = second;
    lexer->read_buffer.position = 19;
    lexer->read_buffer.arena_kind = 6;
    memcpy(prior, lexer, sizeof(prior));
    lexer->Reset();
    if (lexer->read_buffer.block != first || lexer->read_buffer.position != 4 ||
        lexer->read_buffer.arena_kind != 2) return 0;
    for (unsigned r = 0; r < 64; ++r)
        if ((r < 32 || r >= 44) && ((BYTE *)lexer)[r] != prior[r]) return 0;
    /* Read all defined words across a linked block, then keep the terminal
     * position/block unchanged when the chain ends. */
    for (unsigned t = 1; t < 27; ++t) second->tokens[t] = 0x87650000 + t;
    for (unsigned u = 0; u < 27; ++u)
        if (lexer->Read() != 0x12340000 + u) return 0;
    if (lexer->Read() != 0x56789abc) return 0;
    for (unsigned v = 1; v < 27; ++v)
        if (lexer->Read() != 0x87650000 + v) return 0;
    if (lexer->read_buffer.block != second || lexer->read_buffer.position != 31 ||
        lexer->Read() || lexer->read_buffer.block != second || lexer->read_buffer.position != 31) return 0;
    for (unsigned w = 0; w < 16; ++w)
        if (first->unmodeled_04[w] != 0xa5 || second->unmodeled_04[w] != 0xa5) return 0;
    CompilerParserToken old_token, replacement;
    old_token.kind = replacement.kind = 45;
    lexer->read_buffer.block = first;
    lexer->read_buffer.position = 5;
    first->tokens[0] = (unsigned)&old_token;
    lexer->Unread((unsigned)&replacement);
    if (lexer->read_buffer.position != 4 || first->tokens[0] != (unsigned)&replacement ||
        lexer->Read() != (unsigned)&replacement) return 0;
    lexer->Unread(0);
    if (lexer->read_buffer.position != 4 || first->tokens[0] || lexer->Read()) return 0;
    lexer->Unread((unsigned)&old_token);
    if (lexer->Read() != (unsigned)&old_token || first->tokens[1] != 0x12340001) return 0;
    return objects[0] == 0x96969696 && objects[17] == 0x96969696 &&
        blocks[0] == 0xa5a5a5a5 && blocks[65] == 0xa5a5a5a5;
}
extern "C" CompilerParserToken *__cdecl make_arena_token(unsigned);
extern "C" int c1_test_location_token(void)
{
    DWORD storage[10];
    unsigned identity = 37;
    memset(storage, 0xa5, sizeof(storage));
    memset(&c1_bss.literal, 0x96, sizeof(c1_bss.literal));
    c1_bss.current_context.object = &identity;
    c1_bss.current_context.value = 43;
    c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 9);
    CompilerParserToken *token = make_arena_token(2);
    if ((void *)token != storage + 1 || token->kind != 0xf0 ||
        token->value.context.object != &identity || token->value.context.value != 43 ||
        c1_bss.arenas[2].cursor != (BYTE *)(storage + 9) ||
        memcmp(&token->value, &c1_bss.literal, 24)) return 0;
    for (unsigned i = 1; i < 8; ++i) if (((BYTE *)token)[i] != 0xa5) return 0;
    for (unsigned j = 8; j < 24; ++j) if (((BYTE *)&c1_bss.literal)[j] != 0x96) return 0;
    return storage[0] == 0xa5a5a5a5 && storage[9] == 0xa5a5a5a5;
}

/* Destruction frees only the three consumed resources and clears capacity.
 * The debug interface gap and dangling source pointer representations survive. */
extern "C" int c1_test_output_destroy(void)
{
    CompilerOutputBuffer buffer;
    BYTE *data = (BYTE *)::operator new(16);
    char *name = (char *)malloc(8);
    FILE *stream = fopen("owned-output-destruction.tmp", "wb");
    unsigned i;
    if (!data || !name || !stream) return 0;
    memset(&buffer, 0xa5, sizeof(buffer));
    buffer.data = data;
    buffer.cursor = data + 3;
    buffer.capacity = 16;
    buffer.stream = stream;
    buffer.owned_debug_name = name;
    if (fwrite("abc", 1, 3, stream) != 3) return 0;
    buffer.Destroy();
    if (buffer.capacity || buffer.data != data || buffer.cursor != data + 3 ||
        buffer.stream != stream || buffer.owned_debug_name != name ||
        buffer.good != (int)0xa5a5a5a5) return 0;
    for (i = 0; i < sizeof(buffer.unmodeled_124); ++i)
        if (buffer.unmodeled_124[i] != 0xa5) return 0;
    stream = fopen("owned-output-destruction.tmp", "rb");
    if (!stream) return 0;
    if (fgetc(stream) != 'a' || fgetc(stream) != 'b' || fgetc(stream) != 'c' ||
        fgetc(stream) != EOF) { fclose(stream); return 0; }
    return !fclose(stream);
}

extern "C" int c1_test_include_alias_lookup(void)
{
    const char replacement[] = "selected.h";
    CompilerIncludeAlias second = { NULL, "language.h", replacement, 0, 1 };
    CompilerIncludeAlias first = { &second, "LANGUAGE.H", "wrong-kind.h", 1, 0 };
    CompilerIncludeAliases aliases = { &first };
    int kind = 0;
    if (aliases.Find("Language.H", &kind) != replacement || kind != 1) return 0;
    kind = 2;
    if (aliases.Find("language.h", &kind) || kind != 2) return 0;
    aliases.head = NULL;
    if (aliases.Find("language.h", &kind) || kind != 2) return 0;
    return second.replacement == replacement && first.next == &second;
}

extern "C" int c1_test_recorded_lexer_initialize(void)
{
    DWORD objects[18], blocks[34];
    unsigned marker = 0;
    memset(objects, 0xa5, sizeof(objects));
    memset(blocks, 0x96, sizeof(blocks));
    c1_bss.current_context.object = &marker;
    c1_bss.current_context.value = 73;
    c1_bss.arenas[2].cursor = (BYTE *)(blocks + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(blocks + 33);
    CompilerRecordedLexer *lexer = (CompilerRecordedLexer *)(objects + 1);
    if (lexer->Initialize(2, 1) != lexer || !*(void **)lexer ||
        lexer->initial_object != &marker || lexer->current_object != &marker ||
        lexer->initial_value != 73 || lexer->current_value != 73 ||
        lexer->arena_kind != 2 || !lexer->active || lexer->buffer.position != 4 ||
        lexer->buffer.arena_kind != 2 || lexer->buffer.block != (CompilerContextBlock *)(blocks + 1) ||
        lexer->buffer.block->next || c1_bss.arenas[2].cursor != (BYTE *)(blocks + 33)) return 0;
    if (memcmp(&lexer->buffer, &lexer->read_buffer, 12) ||
        memcmp(&lexer->buffer, &lexer->start_buffer, 12)) return 0;
    if ((((BYTE *)lexer)[60] & ~1U) != 0xa4) return 0;
    for (unsigned i = 61; i < 64; ++i) if (((BYTE *)lexer)[i] != 0xa5) return 0;
    for (unsigned j = 4; j < 128; ++j) if (((BYTE *)lexer->buffer.block)[j] != 0x96) return 0;
    return objects[0] == 0xa5a5a5a5 && objects[17] == 0xa5a5a5a5 &&
        blocks[0] == 0x96969696 && blocks[33] == 0x96969696;
}

extern "C" void __cdecl begin_record_definition(CompilerIrSymbol *, CompilerType *);
extern "C" int c1_test_record_definition_write_extent(void)
{
    DWORD objects[18], blocks[34], member_storage[8];
    CompilerContextFrame context;
    CompilerIrSymbol symbol;
    CompilerType type;
    CompilerRecordDefinition *frame = &c1_bss.record_definitions[0];
    memset(objects, 0xa5, sizeof(objects));
    memset(blocks, 0x96, sizeof(blocks));
    memset(member_storage, 0xa6, sizeof(member_storage));
    memset(&c1_bss.record_definitions[0], 0xa5, 2 * sizeof(*frame));
    memset(&type, 0, sizeof(type));
    type.flags = 0x402;
    c1_bss.arenas[2].cursor = (BYTE *)(blocks + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(blocks + 33);
    CompilerRecordedLexer *lexer = ((CompilerRecordedLexer *)(objects + 1))->Initialize(2, 0);
    context.stream = (CompilerLexer *)lexer;
    c1_bss.current_context.saved_frame = &context;
    c1_bss.browser_filename = NULL;
    c1_bss.pools[7].cursor = (BYTE *)(member_storage + 1);
    c1_bss.pools[7].remaining = 24;
    symbol.Initialize(4, NULL, 0);
    symbol.value_type = &type;
    c1_data.record_definition_depth = -1;
    begin_record_definition(&symbol, &type);
    CompilerRecordMembers *members = (CompilerRecordMembers *)(member_storage + 1);
    if (c1_data.record_definition_depth || frame->symbol != &symbol || frame->type != &type ||
        frame->initialized_04 || frame->byte_size || frame->bitfield_type_flags ||
        frame->alignment != 1 || frame->initialized_14 || frame->flags != 0xa1 ||
        frame->unmodeled_16[0] != 0xa5 || frame->unmodeled_16[1] != 0xa5 ||
        symbol.kind4_context != members || members->unmodeled_00 || members->first || members->last ||
        c1_bss.pools[7].cursor != (BYTE *)(member_storage + 4) || c1_bss.pools[7].remaining != 12)
        return 0;
    for (unsigned i = 0; i < sizeof(*frame); ++i)
        if (((BYTE *)&c1_bss.record_definitions[1])[i] != 0xa5) return 0;
    return member_storage[0] == 0xa6a6a6a6 && member_storage[4] == 0xa6a6a6a6;
}

extern "C" int c1_test_nested_record_definition_write_extent(void)
{
    DWORD objects[18], blocks[34], member_storage[8];
    CompilerContextFrame context;
    CompilerIrSymbol symbol;
    CompilerType type;
    CompilerRecordDefinition *frame = &c1_bss.record_definitions[0];
    memset(objects, 0xa5, sizeof(objects));
    memset(blocks, 0x96, sizeof(blocks));
    memset(member_storage, 0xa6, sizeof(member_storage));
    memset(&c1_bss.record_definitions[0], 0xa5, 3 * sizeof(*frame));
    memset(&type, 0, sizeof(type));
    type.flags = 0x402;
    c1_bss.arenas[2].cursor = (BYTE *)(blocks + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(blocks + 33);
    CompilerRecordedLexer *lexer = ((CompilerRecordedLexer *)(objects + 1))->Initialize(2, 0);
    context.stream = (CompilerLexer *)lexer;
    c1_bss.current_context.saved_frame = &context;
    c1_bss.browser_filename = NULL;
    c1_bss.pools[7].cursor = (BYTE *)(member_storage + 1);
    c1_bss.pools[7].remaining = 24;
    symbol.Initialize(4, NULL, 0);
    symbol.value_type = &type;
    c1_data.record_definition_depth = -1;
    begin_record_definition(&symbol, &type);
    CompilerRecordMembers *members = (CompilerRecordMembers *)(member_storage + 1);
    if (c1_data.record_definition_depth || frame->symbol != &symbol || frame->type != &type ||
        frame->initialized_04 || frame->byte_size || frame->bitfield_type_flags ||
        frame->alignment != 1 || frame->initialized_14 || frame->flags != 0xa1 ||
        frame->unmodeled_16[0] != 0xa5 || frame->unmodeled_16[1] != 0xa5 ||
        symbol.kind4_context != members || members->unmodeled_00 || members->first || members->last ||
        c1_bss.pools[7].cursor != (BYTE *)(member_storage + 4) || c1_bss.pools[7].remaining != 12)
        return 0;
    for (unsigned i = 0; i < sizeof(*frame); ++i)
        if (((BYTE *)&c1_bss.record_definitions[1])[i] != 0xa5) return 0;
    CompilerRecordDefinition parent = *frame;
    CompilerIrSymbol child;
    CompilerType child_type = type;
    child_type.flags = 0x404;
    child.Initialize(4, NULL, 0);
    child.value_type = &child_type;
    begin_record_definition(&child, &child_type);
    frame = &c1_bss.record_definitions[1];
    members = (CompilerRecordMembers *)(member_storage + 4);
    if (c1_data.record_definition_depth != 1 ||
        memcmp(&c1_bss.record_definitions[0], &parent, sizeof(parent)) ||
        frame->symbol != &child || frame->type != &child_type ||
        frame->initialized_04 || frame->byte_size || frame->bitfield_type_flags ||
        frame->alignment != 1 || frame->initialized_14 || frame->flags != 0xa0 ||
        frame->unmodeled_16[0] != 0xa5 || frame->unmodeled_16[1] != 0xa5 ||
        child.kind4_context != members || members->unmodeled_00 || members->first || members->last ||
        c1_bss.pools[7].cursor != (BYTE *)(member_storage + 7) || c1_bss.pools[7].remaining)
        return 0;
    for (unsigned i = 0; i < sizeof(*frame); ++i)
        if (((BYTE *)&c1_bss.record_definitions[2])[i] != 0xa5) return 0;
    return member_storage[0] == 0xa6a6a6a6 && member_storage[7] == 0xa6a6a6a6;
}

extern "C" int c1_test_parameter_link_write_extent(void)
{
    DWORD storage[8];
    CompilerIrSymbol symbol;
    BYTE before[sizeof(symbol)];
    CompilerParameterScope scope(0);
    memset(storage, 0xa5, sizeof(storage));
    memset(&symbol, 0xa6, sizeof(symbol));
    memcpy(before, &symbol, sizeof(symbol));
    c1_bss.parameter_nesting = 1;
    c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 7);
    scope.Link(&symbol);
    CompilerParameterBindings *list = (CompilerParameterBindings *)(storage + 1);
    if (scope.parameter_bindings != list || list->arena_kind != 2 ||
        list->first != &symbol.binding || list->last != &symbol.binding ||
        symbol.binding.next || symbol.previous_parameter ||
        c1_bss.arenas[2].cursor != (BYTE *)(storage + 4)) return 0;
    for (unsigned i = 0; i < sizeof(symbol); ++i)
        if (!(i >= 12 && i < 16) && !(i >= 32 && i < 36) && ((BYTE *)&symbol)[i] != before[i])
            return 0;
    return storage[0] == 0xa5a5a5a5 && storage[4] == 0xa5a5a5a5 &&
        storage[5] == 0xa5a5a5a5 && storage[6] == 0xa5a5a5a5 && storage[7] == 0xa5a5a5a5;
}

/* A byte transfer preserves opaque payload too; this checks extent, not a
 * semantic default or cross-process equality for those bytes. */
extern "C" int c1_test_initializer_copy_extent(void)
{
    struct OwnedSymbol { DWORD before; CompilerIrSymbol symbol; DWORD after; } destination;
    CompilerIrSymbol source;
    memset(&source, 0x6b, sizeof(source));
    memset(&destination, 0xa5, sizeof(destination));
    source.kind = 10;
    destination.symbol.CopyInitializerFrom(&source, 1);
    if (memcmp(&source, &destination.symbol, 52) ||
        destination.before != 0xa5a5a5a5U || destination.after != 0xa5a5a5a5U) return 0;
    source.kind = 3;
    destination.symbol.CopyInitializerFrom(&source, 1);
    if (memcmp(&source, &destination.symbol, 52)) return 0;
    source.kind = 2;
    destination.symbol.CopyInitializerFrom(&source, 1);
    if (memcmp(&source, &destination.symbol, 52) || destination.before != 0xa5a5a5a5U ||
        destination.after != 0xa5a5a5a5U) return 0;
    DWORD storage[8];
    CompilerRecordMembers original;
    CompilerSymbolBinding first, last;
    original.first = &first;
    original.last = &last;
    source.kind = 4;
    source.kind4_context = &original;
    memset(storage, 0xa5, sizeof(storage));
    c1_bss.arenas[1].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[1].committed_limit = (BYTE *)(storage + 8);
    destination.symbol.CopyInitializerFrom(&source, 1);
    CompilerRecordMembers *copy = (CompilerRecordMembers *)(storage + 1);
    if (destination.symbol.kind4_context != copy || copy->arena_kind != 1 ||
        copy->first != &first || copy->last != &first ||
        c1_bss.arenas[1].cursor != (BYTE *)(storage + 4)) return 0;
    for (unsigned i = 0; i < 48; ++i)
        if (((BYTE *)&source)[i] != ((BYTE *)&destination.symbol)[i]) return 0;
    for (unsigned i = 4; i < 8; ++i) if (storage[i] != 0xa5a5a5a5U) return 0;
    return storage[0] == 0xa5a5a5a5U && destination.before == 0xa5a5a5a5U &&
        destination.after == 0xa5a5a5a5U;
}

extern "C" CompilerParserNode *__cdecl make_typed_unary_operation(unsigned, CompilerType *, CompilerParserNode *);
extern "C" CompilerParserNode *__cdecl make_enumerator_expression(CompilerIrSymbol *);
extern "C" int c1_test_unary_node_extent(void)
{
    DWORD storage[10];
    CompilerParserNode child;
    CompilerType type;
    CompilerIrSymbol symbol;
    CompilerParserNode *value;
    memset(storage, 0xa5, sizeof(storage));
    memset(&child, 0x96, sizeof(child));
    c1_data.expression_arena_kind = 2;
    c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 10);
    value = make_typed_unary_operation(0x15a, &type, &child);
    if ((void *)value != storage + 1 || c1_bss.arenas[2].cursor != (BYTE *)(storage + 7) ||
        value->kind != 90 || value->flags != 1 || value->type != &type || value->left != &child)
        return 0;
    for (unsigned i = 2; i < 4; ++i) if (((BYTE *)value)[i] != 0xa5) return 0;
    for (unsigned j = 8; j < 16; ++j) if (((BYTE *)value)[j] != 0xa5) return 0;
    for (unsigned k = 20; k < 24; ++k) if (((BYTE *)value)[k] != 0xa5) return 0;
    for (unsigned l = 7; l < 10; ++l) if (storage[l] != 0xa5a5a5a5) return 0;
    if (storage[0] != 0xa5a5a5a5) return 0;
    for (unsigned m = 0; m < sizeof(child); ++m) if (((BYTE *)&child)[m] != 0x96) return 0;
    for (int negative = 0; negative < 2; ++negative) {
        memset(storage, 0xa5, sizeof(storage));
        c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
        symbol.integer_value = negative ? -17 : 0;
        c1_data.builtin_types.type_0063 = &type;
        value = make_enumerator_expression(&symbol);
        if ((void *)value != storage + 1 || c1_bss.arenas[2].cursor != (BYTE *)(storage + 7) ||
            value->kind != 101 || value->flags != 3 || value->type != &type ||
            value->low != (unsigned)symbol.integer_value || value->high != (negative ? ~0U : 0)) return 0;
        for (unsigned n = 2; n < 4; ++n) if (((BYTE *)value)[n] != 0xa5) return 0;
        for (unsigned o = 8; o < 16; ++o) if (((BYTE *)value)[o] != 0xa5) return 0;
        for (unsigned q = 7; q < 10; ++q) if (storage[q] != 0xa5a5a5a5) return 0;
        if (storage[0] != 0xa5a5a5a5) return 0;
    }
    return 1;
}

extern "C" CompilerParserNode *__cdecl fold_binary_expression(CompilerParserNode *, CompilerParserNode *, CompilerParserNode *);
extern "C" int c1_test_binary_comparison_signedness(void)
{
    struct Comparison { BYTE operation; int signed_result, unsigned_result; } comparisons[] = {
        { 59, 0, 0 }, { 76, 1, 1 }, { 68, 1, 0 },
        { 69, 1, 0 }, { 62, 0, 1 }, { 63, 0, 1 }
    };
    CompilerTypeEntry integer_entry;
    CompilerTypeEntry *buckets[2048];
    CompilerType &integer_type = integer_entry.type;
    CompilerType operand_type;
    memset(&integer_entry, 0, sizeof(integer_entry));
    memset(buckets, 0, sizeof(buckets));
    memset(&integer_type, 0, sizeof(integer_type));
    integer_type.flags = 0x63;
    integer_type.attributes = 0x100;
    buckets[0x163] = &integer_entry;
    c1_data.type_buckets = buckets;
    c1_bss.type_description_depth = 0;
    c1_data.builtin_types.type_0063 = &integer_type;
    c1_data.expression_arena_kind = 2;
    for (unsigned sign = 0; sign < 2; ++sign) {
        operand_type = integer_type;
        if (sign) operand_type.flags |= 0x4000;
        for (unsigned i = 0; i < sizeof(comparisons) / sizeof(comparisons[0]); ++i) {
            DWORD storage[10];
            CompilerParserNode left, right, node, saved_node;
            memset(storage, 0xa5, sizeof(storage));
            memset(&left, 0x6b, sizeof(left));
            memset(&right, 0x7c, sizeof(right));
            memset(&node, 0x8d, sizeof(node));
            left.kind = right.kind = 101;
            left.flags = right.flags = 3;
            left.type = right.type = &operand_type;
            left.low = ~0U;
            left.high = sign ? 0 : ~0U;
            right.low = 1;
            right.high = 0;
            node.kind = comparisons[i].operation;
            node.flags = 2;
            node.type = &integer_type;
            node.left = &left;
            node.right = &right;
            saved_node = node;
            c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
            c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 10);
            CompilerParserNode *value = fold_binary_expression(&node, &left, &right);
            unsigned expected = sign ? comparisons[i].unsigned_result : comparisons[i].signed_result;
            if ((void *)value != storage + 1 || value->kind != 101 ||
                value->type != &integer_type || value->low != expected || value->high ||
                value->flags != (expected ? 3 : 0x83) ||
                memcmp(&node, &saved_node, sizeof(node)) ||
                c1_bss.arenas[2].cursor != (BYTE *)(storage + 7)) return 0;
            for (unsigned j = 2; j < 4; ++j) if (((BYTE *)value)[j] != 0xa5) return 0;
            for (unsigned k = 8; k < 16; ++k) if (((BYTE *)value)[k] != 0xa5) return 0;
            if (storage[0] != 0xa5a5a5a5U) return 0;
            for (unsigned tail = 7; tail < 10; ++tail)
                if (storage[tail] != 0xa5a5a5a5U) return 0;
        }
    }
    return 1;
}

extern "C" int __cdecl integer_value_loses_bits(int, int, int, __int64);
extern "C" int c1_test_integer_conversion_ranges(void)
{
    struct Range { int width, destination_unsigned, source_unsigned; __int64 value; int loses; } ranges[] = {
        {1,0,0,-128,0}, {1,0,0,-129,1}, {1,0,0,127,0}, {1,0,0,128,1},
        {1,1,0,-255,0}, {1,1,0,-256,1}, {1,1,0,255,0}, {1,1,0,256,1},
        {1,0,1,255,0}, {1,0,1,256,1},
        {2,0,0,-32768,0}, {2,0,0,-32769,1}, {2,0,0,32767,0}, {2,0,0,32768,1},
        {4,0,0,-2147483648LL,0}, {4,0,0,-2147483649LL,1},
        {4,0,0,2147483647LL,0}, {4,0,0,2147483648LL,1},
        {4,1,0,-4294967295LL,0}, {4,1,0,-4294967296LL,1},
        {8,0,0,(-9223372036854775807LL-1),0}, {8,0,0,9223372036854775807LL,0},
        {8,1,0,(-9223372036854775807LL-1),0}, {8,0,1,-1,0}
    };
    for (unsigned i = 0; i < sizeof(ranges)/sizeof(ranges[0]); ++i)
        if (integer_value_loses_bits(ranges[i].width, ranges[i].destination_unsigned,
            ranges[i].source_unsigned, ranges[i].value) != ranges[i].loses) return 0;
    return 1;
}

struct ParserLabelStack;
extern "C" ParserLabelStack *__cdecl push_parser_label_link(ParserLabelStack **, unsigned);
extern "C" int c1_test_parser_label_link_extent(void)
{
    DWORD storage[6];
    memset(storage, 0xa5, sizeof(storage));
    ParserLabelStack *previous = (ParserLabelStack *)(storage + 5), *head = previous;
    c1_bss.arenas[4].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[4].committed_limit = (BYTE *)(storage + 5);
    ParserLabelStack *entry = push_parser_label_link(&head, 4);
    return entry == (ParserLabelStack *)(storage + 1) && head == entry &&
        storage[1] == (DWORD)previous && storage[2] == 0xa5a5a5a5U &&
        storage[0] == 0xa5a5a5a5U && storage[3] == 0xa5a5a5a5U &&
        storage[4] == 0xa5a5a5a5U && storage[5] == 0xa5a5a5a5U &&
        c1_bss.arenas[4].cursor == (BYTE *)(storage + 3);
}

extern "C" void __cdecl layout_bitfield_member(CompilerIrSymbol *, WORD);
extern "C" int c1_test_packed_bitfields(void)
{
    CompilerPackingFrame packing_frame = { NULL, 1, NULL };
    CompilerPacking packing = { &packing_frame, NULL };
    CompilerRecordDefinition *frame = &c1_bss.record_definitions[0];
    CompilerIrSymbol record, fields[3];
    CompilerRecordMembers members;
    struct CompilerNameEntry names[3];
    CompilerType type;
    unsigned i;
    memset(&record, 0, sizeof(record)); memset(&members, 0, sizeof(members));
    memset(&type, 0, sizeof(type)); memset(names, 0, sizeof(names));
    memset(fields, 0xa5, sizeof(fields)); memset(frame, 0, sizeof(*frame));
    type.flags = 0x4063;
    record.kind4_context = &members;
    frame->symbol = &record; frame->flags = 1; frame->alignment = 1;
    frame->unmodeled_16[0] = 0x5a; frame->unmodeled_16[1] = 0x3c;
    c1_constant_diagnostic_mode = 0;
    c1_data.options.packing_stack = &packing;
    for (i = 0; i < 3; ++i) {
        fields[i].binding.name = &names[i]; fields[i].value_type = &type;
        fields[i].bit_width = i == 0 ? 3 : i == 1 ? 5 : 8;
        layout_bitfield_member(&fields[i], 0x4063);
        if (fields[i].symbol_value || fields[i].unmodeled_26[0] != 0xa5 ||
            fields[i].unmodeled_26[1] != 0xa5) return 0;
    }
    return fields[0].bit_offset == 0 && fields[1].bit_offset == 3 &&
        fields[2].bit_offset == 8 && frame->initialized_14 == 16 &&
        frame->byte_size == 0 && frame->bitfield_type_flags == 0x4063 &&
        members.first == &fields[0].binding && members.last == &fields[2].binding &&
        fields[0].binding.next == &fields[1].binding &&
        fields[1].binding.next == &fields[2].binding && !fields[2].binding.next &&
        frame->unmodeled_16[0] == 0x5a && frame->unmodeled_16[1] == 0x3c;
}

extern "C" int __cdecl check_extended_range(void);
extern "C" int __cdecl classify_literal_bits(unsigned, const void *, const void *);
extern "C" void __cdecl store_extended_real(void);
extern "C" InitializerExtendedReal *__cdecl allocate_extended_constant(InitializerExtendedReal, unsigned);
extern "C" int c1_test_extended_range_boundaries(void)
{
    struct RangeCase { DWORD low, high; WORD exponent; int outside, negative; } cases[] = {
        {0,0xa0000000U,0x3fff,0,0}, {0,0xa0000000U,0xbfff,0,1},
        {0,0,0,0,0}, {0,0,0x8000,0,0},
        {~1025U,~0U,0x43fe,0,0}, {~1024U,~0U,0x43fe,0,0},
        {~1023U,~0U,0x43fe,1,0}, {~1023U,~0U,0xc3fe,1,1},
        {~0U,~0U,0x7ffe,1,0}
    };
    InitializerExtendedReal limit = {~1024U,~0U,0x43fe,0xa5a5};
    unsigned index;
    for (index = 0; index < sizeof(cases)/sizeof(cases[0]); ++index) {
        InitializerExtendedReal value, returned;
        WORD before, after_call, after_pop;
        int outside;
        value.significand_low = cases[index].low;
        value.significand_high = cases[index].high;
        value.exponent = cases[index].exponent;
        value.precision = 0xa5a5;
        memset(&returned, 0x5a, sizeof(returned));
        memset(&c1_real_range_scratch, 0xa5, sizeof(c1_real_range_scratch));
        __asm {
            fstsw before
            fld tbyte ptr value
            fld tbyte ptr limit
            call check_extended_range
            mov outside, eax
            fstsw after_call
            fstp tbyte ptr returned
            fstsw after_pop
        }
        if (outside != cases[index].outside ||
            returned.significand_low != value.significand_low ||
            returned.significand_high != value.significand_high ||
            returned.exponent != value.exponent || returned.precision != 0x5a5a ||
            c1_real_range_scratch.negative != (unsigned)cases[index].negative ||
            c1_real_range_scratch.unmodeled_tail[0] != 0xa5 ||
            c1_real_range_scratch.unmodeled_tail[1] != 0xa5 ||
            ((after_call >> 11) & 7) != (((before >> 11) - 1) & 7) ||
            ((after_pop ^ before) & 0x3800) ||
            !!(c1_real_range_scratch.status & 0x100) != cases[index].outside ||
            ((c1_real_range_scratch.status ^ after_call) & 0x7f00)) return 0;
    }
    {
        struct { DWORD before; InitializerExtendedReal value; DWORD after; } owned;
        InitializerExtendedReal value = {0,0xa0000000U,0xbfff,0xa5a5};
        InitializerExtendedReal *destination = &owned.value, *receiver;
        WORD before, after;
        memset(&owned, 0x5a, sizeof(owned));
        __asm {
            fstsw before
            fld tbyte ptr value
            mov esi, destination
            call store_extended_real
            mov receiver, esi
            fstsw after
        }
        if (receiver != destination || owned.before != 0x5a5a5a5aU ||
            owned.after != 0x5a5a5a5aU || owned.value.precision != 0x5a5a ||
            owned.value.significand_low != value.significand_low ||
            owned.value.significand_high != value.significand_high ||
            owned.value.exponent != value.exponent || ((after ^ before) & 0x3800)) return 0;
    }
    {
        DWORD storage[5];
        InitializerExtendedReal value = {0x12345678,0xa0000000U,0xbfff,0xa5a5};
        InitializerExtendedReal saved = value;
        InitializerExtendedReal *stored;
        memset(storage, 0x5a, sizeof(storage));
        c1_bss.arenas[4].cursor = (BYTE *)(storage + 1);
        c1_bss.arenas[4].committed_limit = (BYTE *)(storage + 4);
        stored = allocate_extended_constant(value, 4);
        if (stored != (InitializerExtendedReal *)(storage + 1) ||
            c1_bss.arenas[4].cursor != (BYTE *)(storage + 4) ||
            c1_bss.arenas[4].committed_limit != (BYTE *)(storage + 4) ||
            storage[0] != 0x5a5a5a5aU || storage[4] != 0x5a5a5a5aU ||
            memcmp(stored, &saved, 12) || memcmp(&value, &saved, 12)) return 0;
    }
    {
        InitializerExtendedReal values[] = {
            {0,0,0,4}, {0,0,0x8000,8},
            {0,0x80000000U,0x3fff,8}, {0,0xa0000000U,0x3fff,8},
            {0,0xa0000000U,0xbfff,8}
        };
        unsigned first, second, operation;
        /* Signed zeros compare equal; all other pairs have ordinary signed
         * numeric order. Check every relation-mask combination and stack TOP. */
        int rank[] = {0,0,1,2,-1};
        for (first = 0; first < 5; ++first) for (second = 0; second < 5; ++second) {
            InitializerExtendedReal left = values[first], right = values[second];
            unsigned relation = rank[first] == rank[second] ? 1 : rank[first] > rank[second] ? 2 : 4;
            for (operation = 0; operation < 8; ++operation) {
                WORD before, after;
                __asm { fstsw before }
                memset(&c1_real_range_scratch, 0xa5, sizeof(c1_real_range_scratch));
                int result = classify_literal_bits(operation, &left, &right);
                __asm { fstsw after }
                if (result != !!(operation & relation) || ((after ^ before) & 0x3800) ||
                    (c1_real_range_scratch.status & 0x4500) !=
                        (relation == 1 ? 0x4000 : relation == 4 ? 0x100 : 0) ||
                    c1_real_range_scratch.negative != 0xa5a5a5a5U ||
                    c1_real_range_scratch.unmodeled_tail[0] != 0xa5 ||
                    c1_real_range_scratch.unmodeled_tail[1] != 0xa5 ||
                    memcmp(&left, &values[first], 12) || memcmp(&right, &values[second], 12)) return 0;
            }
        }
    }
    return 1;
}

extern "C" CompilerIrSymbol *__cdecl make_statement_label(CompilerNameEntry *);
extern "C" CompilerIrSymbol *__cdecl find_statement_label(CompilerNameEntry *);
extern "C" CompilerParserNode *__cdecl make_symbol_node(BYTE, CompilerIrSymbol *, CompilerType *);
extern "C" int c1_test_statement_label_storage(void)
{
    DWORD storage[15];
    BYTE expected[sizeof(storage)];
    CompilerNameEntry name, other;
    CompilerType integer_type;
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)(storage + 1);
    memset(storage, 0xa5, sizeof(storage));
    memcpy(expected, storage, sizeof(storage));
    memset(&integer_type, 0, sizeof(integer_type)); integer_type.flags = 0x63;
    c1_data.builtin_types.type_0063 = &integer_type;
    c1_data.record_sizes.known.global_symbol_bytes = 52;
    c1_bss.arenas[2].cursor = (BYTE *)symbol;
    c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 14);
    if (make_statement_label(&name) != symbol) return 0;
    if (symbol->kind != 9 || symbol->binding.name != &name || symbol->binding.next ||
        symbol->value_type != &integer_type || symbol->symbol_value ||
        c1_bss.arenas[2].cursor != (BYTE *)(storage + 14)) return 0;
    ((CompilerIrSymbol *)(expected + 4))->kind = 9;
    ((CompilerIrSymbol *)(expected + 4))->binding.name = &name;
    ((CompilerIrSymbol *)(expected + 4))->binding.next = NULL;
    ((CompilerIrSymbol *)(expected + 4))->value_type = &integer_type;
    ((CompilerIrSymbol *)(expected + 4))->symbol_value = 0;
    if (memcmp(storage, expected, sizeof(storage))) return 0;
    {
        DWORD node_storage[8], node_expected[8];
        CompilerParserNode *node = (CompilerParserNode *)(node_storage + 1);
        CompilerParserNode *expected_node = (CompilerParserNode *)(node_expected + 1);
        memset(node_storage, 0x5a, sizeof(node_storage));
        memcpy(node_expected, node_storage, sizeof(node_storage));
        c1_data.expression_arena_kind = 4;
        c1_bss.arenas[4].cursor = (BYTE *)node;
        c1_bss.arenas[4].committed_limit = (BYTE *)(node_storage + 7);
        expected_node->kind = 6; expected_node->flags = 4;
        expected_node->symbol = symbol; expected_node->type = NULL;
        if (make_symbol_node(6, symbol, NULL) != node ||
            c1_bss.arenas[4].cursor != (BYTE *)(node_storage + 7) ||
            memcmp(node_storage, node_expected, sizeof(node_storage))) return 0;
    }
    c1_data.label_symbols = symbol;
    c1_bss.parser_early_result = 0;
    return find_statement_label(&name) == symbol && !find_statement_label(&other) &&
        !memcmp(storage, expected, sizeof(storage));
}

extern "C" void __cdecl write_string_literal_initializer(CompilerIrSymbol *, struct CompilerParserNode *);
extern "C" int c1_test_deferred_literal_buffer(void)
{
    BYTE ordinary[64], deferred[64], saved_ordinary[64];
    static const BYTE expected[] = { 0, 37, 0, 0, 3, 4, 'a', 'b', 'c', 0, 7 };
    CompilerOutputBuffer saved_buffer;
    CompilerIrSymbol symbol, pending;
    CompilerType type;
    struct CompilerTypeShape shape;
    struct CompilerParserNode value;
    unsigned i;
    memset(ordinary, 0xa5, sizeof(ordinary));
    memcpy(saved_ordinary, ordinary, sizeof(ordinary));
    memset(deferred, 0x5a, sizeof(deferred));
    memset(&symbol, 0, sizeof(symbol));
    memset(&type, 0, sizeof(type));
    memset(&shape, 0, sizeof(shape));
    memset(&value, 0, sizeof(value));
    c1_bss.output_buffers[4].data = ordinary;
    c1_bss.output_buffers[4].cursor = ordinary + 7;
    c1_bss.output_buffers[4].capacity = sizeof(ordinary);
    c1_bss.output_buffers[4].good = 1;
    saved_buffer = c1_bss.output_buffers[4];
    c1_bss.output_buffers[5].data = deferred;
    c1_bss.output_buffers[5].cursor = deferred + 1;
    c1_bss.output_buffers[5].capacity = sizeof(deferred);
    c1_bss.output_buffers[5].good = 1;
    c1_data.c9_il = 1;
    c1_data.function_record_flags = 0;
    c1_data.last_initializer_symbol = &pending;
    type.flags = 0x61;
    type.attributes = 0x100;
    type.shape = &shape;
    shape.flags = 2;
    shape.value = 4;
    symbol.kind = 10;
    symbol.qualifier_flags = 4;
    symbol.record_id = 37;
    symbol.value_type = &type;
    value.kind = 7;
    value.flags = 5;
    value.type = &type;
    value.text = "abc";
    value.string_length = 4;
    write_string_literal_initializer(&symbol, &value);
    if (memcmp(&saved_buffer, &c1_bss.output_buffers[4], sizeof(saved_buffer)) ||
        memcmp(ordinary, saved_ordinary, sizeof(ordinary)) ||
        c1_data.last_initializer_symbol != &pending ||
        c1_bss.output_buffers[5].cursor != deferred + 1 + sizeof(expected) ||
        memcmp(deferred + 1, expected, sizeof(expected)) || deferred[0] != 0x5a)
        return 0;
    for (i = 1 + sizeof(expected); i < sizeof(deferred); ++i)
        if (deferred[i] != 0x5a) return 0;
    return 1;
}

extern "C" int __cdecl initializer_real_to_float(const InitializerExtendedReal *, float *);
extern "C" int c1_test_real_to_float_boundaries(void)
{
    struct FloatCase { DWORD low, high; WORD exponent; DWORD bits; int outside; } cases[] = {
        {0,0xa0000000U,0x3fff,0x3fa00000,0},
        {0,0x80000000U,0xbffe,0xbf000000,0},
        {0,0,0,0,0}, {0,0,0x8000,0x80000000U,0},
        {0,0xffffff00U,0x407e,0x7f7fffff,0},
        {~1U,0xffffff7fU,0x407e,0x7f7fffff,0},
        {~0U,0xffffff7fU,0x407e,0x7f7fffff,0},
        {0,0xffffff80U,0x407e,0,1},
        {0,0xffffff80U,0xc07e,0,1}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        InitializerExtendedReal value = {cases[i].low,cases[i].high,cases[i].exponent,0x5a5a};
        InitializerExtendedReal saved = value, retained;
        DWORD owned[3] = {0xa5a5a5a5U,0xa5a5a5a5U,0xa5a5a5a5U};
        WORD before, after, popped;
        __asm { fstsw before }
        int outside = initializer_real_to_float(&value, (float *)(owned + 1));
        __asm { fstsw after }
        if (outside) {
            __asm {
                fstp tbyte ptr retained
                fstsw popped
            }
            if (memcmp(&retained, &saved, 10) || ((popped ^ before) & 0x3800) ||
                owned[1] != 0xa5a5a5a5U) return 0;
        } else if (owned[1] != cases[i].bits) return 0;
        if (outside != cases[i].outside || memcmp(&value, &saved, sizeof(value)) ||
            owned[0] != 0xa5a5a5a5U || owned[2] != 0xa5a5a5a5U ||
            ((after >> 11) & 7) != (((before >> 11) - outside) & 7)) return 0;
    }
    return 1;
}

extern "C" int __cdecl initializer_float_to_real(const float *, InitializerExtendedReal *);
extern "C" int c1_test_float_to_real_overlap(void)
{
    struct { DWORD before; InitializerExtendedReal number; DWORD after; } owned;
    unsigned pass;
    for (pass = 0; pass < 2; ++pass) {
        DWORD bits = 0x3fa00000;
        float *source = pass ? (float *)&owned.number.exponent : (float *)&bits;
        WORD before, after;
        memset(&owned, 0xa5, sizeof(owned));
        if (pass) memcpy(source, &bits, 4);
        __asm { fstsw before }
        int result = initializer_float_to_real(source, &owned.number);
        __asm { fstsw after }
        if (result || owned.before != 0xa5a5a5a5U || owned.after != 0xa5a5a5a5U ||
            owned.number.significand_low || owned.number.significand_high != 0xa0000000U ||
            owned.number.exponent != 0x3fff || owned.number.precision != 4 ||
            bits != 0x3fa00000 || ((after ^ before) & 0x3800)) return 0;
    }
    return 1;
}

extern "C" int __cdecl subtract_extended_constants(const InitializerExtendedReal *,
    const InitializerExtendedReal *, InitializerExtendedReal *);
extern "C" int c1_test_extended_subtraction_precision(void)
{
    struct { DWORD before; InitializerExtendedReal value; DWORD after; } owned;
    const WORD tags[] = {4,8,10};
    unsigned i, mode;
    for (i = 0; i < 3; ++i) for (mode = 0; mode < 2; ++mode) {
        InitializerExtendedReal left = {0,0xa0000000U,0x3fff,tags[i]};
        InitializerExtendedReal right = {0,0x80000000U,0x3ffe,4};
        InitializerExtendedReal saved_left = left, saved_right = right;
        WORD before, after;
        memset(&owned, 0xa5, sizeof(owned));
        c1_data.options.precise_float = mode;
        __asm { fstsw before }
        int result = subtract_extended_constants(&left, &right, &owned.value);
        __asm { fstsw after }
        if (result != (i == 2 ? 0 : (int)mode) || owned.before != 0xa5a5a5a5U ||
            owned.after != 0xa5a5a5a5U || owned.value.significand_low ||
            owned.value.significand_high != 0xc0000000U || owned.value.exponent != 0x3ffe ||
            owned.value.precision != tags[i] || memcmp(&left, &saved_left, 12) ||
            memcmp(&right, &saved_right, 12) || ((after ^ before) & 0x3800)) return 0;
    }
    /* Select extended arithmetic precision so a binary64 half-ulp survives
     * the subtraction itself. The -Op store/reload then ties to even. */
    for (i = 0; i < 2; ++i) for (mode = 0; mode < 2; ++mode) {
        InitializerExtendedReal left = {0,0x80000000U,0x3fff,tags[i]};
        InitializerExtendedReal right = {0,0x80000000U,(WORD)(i ? 0xbfca : 0xbfe7),tags[i]};
        WORD before, after, saved_control, extended_control;
        c1_data.options.precise_float = mode;
        memset(&owned, 0xa5, sizeof(owned));
        __asm { fnstcw saved_control }
        extended_control = (WORD)(saved_control | 0x0300);
        __asm { fldcw extended_control }
        __asm { fstsw before }
        int result = subtract_extended_constants(&left, &right, &owned.value);
        __asm { fstsw after }
        __asm { fldcw saved_control }
        if (result != (int)mode || owned.value.significand_low != (!mode && i ? 0x400U : 0) ||
            owned.value.significand_high != (!mode && !i ? 0x80000080U : 0x80000000U) ||
            owned.value.exponent != 0x3fff || owned.value.precision != tags[i] ||
            owned.before != 0xa5a5a5a5U || owned.after != 0xa5a5a5a5U ||
            ((after ^ before) & 0x3800)) return 0;
    }
    return 1;
}

extern "C" int __cdecl divide_extended_constants(const InitializerExtendedReal *,
    const InitializerExtendedReal *, InitializerExtendedReal *);
extern "C" int c1_test_extended_division_order(void)
{
    struct { DWORD before; InitializerExtendedReal value; DWORD after; } owned;
    const WORD tags[] = {4,8,10};
    for (unsigned i = 0; i < 3; ++i) {
        InitializerExtendedReal left = {0,0x80000000U,0x4001,tags[i]}; /* 4 */
        InitializerExtendedReal right = {0,0x80000000U,0x4000,4}; /* 2 */
        InitializerExtendedReal saved_left = left, saved_right = right;
        WORD before, after;
        c1_data.options.precise_float = 0;
        memset(&owned, 0xa5, sizeof(owned));
        __asm { fstsw before }
        int result = divide_extended_constants(&left, &right, &owned.value);
        __asm { fstsw after }
        if (result || owned.value.significand_low ||
            owned.value.significand_high != 0x80000000U || owned.value.exponent != 0x4000 ||
            owned.value.precision != tags[i] || owned.before != 0xa5a5a5a5U ||
            owned.after != 0xa5a5a5a5U || memcmp(&left, &saved_left, 12) ||
            memcmp(&right, &saved_right, 12) || ((after ^ before) & 0x3800)) return 0;
    }
    return 1;
}

extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
extern "C" int c1_test_suppressed_language_diagnostic(void)
{
    c1_bss.error_count = 17;
    if (c1_bss.parser_early_result != 17) return 0;
    c1_bss.parser_early_result = 23;
    if (c1_bss.error_count != 23) return 0;
    c1_data.parser_flags = 0;
    c1_data.options.warning_level = 1;
    c1_data.options.warning_levels[131] = 0;
    report_language_diagnostic(4, 131, (void *)0x13579);
    c1_data.options.warning_level = 4;
    c1_data.options.warning_levels[131] = 13;
    report_language_diagnostic(4, 131, (void *)0x13579);
    c1_data.options.warning_levels[131] = 255;
    report_language_diagnostic(4, 131, (void *)0x13579);
    c1_data.parser_flags = 0x20000;
    report_language_diagnostic(4, ~0U, (void *)0x13579);
    return c1_bss.error_count == 23 && c1_bss.parser_early_result == 23 &&
        c1_data.parser_flags == 0x20000 && c1_data.options.warning_level == 4 &&
        c1_data.options.warning_levels[131] == 255;
}

extern "C" int c1_test_symbol_copy_storage(void)
{
    struct { DWORD before; CompilerIrSymbol value; DWORD after; } source, destination;
    CompilerIrSymbol expected;
    if (sizeof(CompilerIrSymbol) != 52) return 0;
    for (unsigned kind_index = 0; kind_index < 2; ++kind_index) {
    memset(&source, 0x3d, sizeof(source));
    memset(&destination, 0xa7, sizeof(destination));
    source.value.kind = kind_index ? 3 : 10;
    source.value.binding.name = (CompilerNameEntry *)&source.before;
    source.value.binding.next = (CompilerSymbolBinding *)&source.after;
    memcpy(&expected, &source.value, 52);
    expected.binding.next = NULL;
    c1_data.record_sizes.known.static_symbol_bytes = 52;
    c1_data.record_sizes.known.member_symbol_bytes = 52;
    if (destination.value.CopyFrom(&source.value, 0) != &destination.value) return 0;
    if (memcmp(&destination.value, &expected, 52) ||
        destination.before != 0xa7a7a7a7U || destination.after != 0xa7a7a7a7U ||
        source.before != 0x3d3d3d3dU || source.after != 0x3d3d3d3dU ||
        source.value.binding.next != (CompilerSymbolBinding *)&source.after) return 0;
    }
    return 1;
}

extern "C" char *__cdecl encode_declaration_number(char *, unsigned);
extern "C" int c1_test_declaration_number_encoding(void)
{
    static const unsigned values[] = {0,1,10,11,15,16,256,0xffffffffU};
    static const char *expected[] = {"A@","0","9","L@","P@","BA@","BAA@","PPPPPPPP@"};
    char owned[24];
    for (unsigned i = 0; i < 8; ++i) {
        memset(owned, 0xa5, sizeof(owned));
        unsigned length = strlen(expected[i]);
        if (encode_declaration_number(owned + 4, values[i]) != owned + 4 + length ||
            memcmp(owned + 4, expected[i], length)) return 0;
        for (unsigned j = 0; j < sizeof(owned); ++j)
            if ((j < 4 || j >= 4 + length) && (BYTE)owned[j] != 0xa5) return 0;
    }
    return 1;
}

extern "C" unsigned __cdecl enum_symbol_bucket(CompilerSymbolMap *, void *);
extern "C" CompilerIrSymbol *__cdecl find_symbol_map_value(CompilerSymbolMap *, void *);
extern "C" int c1_test_static_symbol_map_lookup(void)
{
    char key_storage[512];
    CompilerIrSymbol payload[2];
    CompilerSymbolMapEntry entries[2], *buckets[31];
    CompilerSymbolMap map, saved;
    void *first = (void *)(((unsigned)key_storage + 127) & ~127U);
    void *second = (BYTE *)first + 124;
    memset(&map, 0xe7, sizeof(map));
    memset(buckets, 0, sizeof(buckets));
    map.arena_kind = 2; map.match_kind = 1; map.hash_kind = 2;
    map.capacity = 31; map.symbols = buckets; map.count = 2;
    entries[0].key = first; entries[0].symbol = payload; entries[0].next = entries + 1;
    entries[1].key = second; entries[1].symbol = payload + 1; entries[1].next = NULL;
    unsigned bucket = enum_symbol_bucket(&map, first);
    if (enum_symbol_bucket(&map, second) != bucket) return 0;
    buckets[bucket] = entries;
    memcpy(&saved, &map, sizeof(map));
    return find_symbol_map_value(NULL, first) == NULL &&
        find_symbol_map_value(&map, first) == payload &&
        find_symbol_map_value(&map, second) == payload + 1 &&
        find_symbol_map_value(&map, (BYTE *)first + 4) == NULL &&
        !memcmp(&map, &saved, sizeof(map)) && buckets[bucket] == entries &&
        entries[0].next == entries + 1 && entries[1].next == NULL;
}

extern "C" CompilerAssemblyOperand *__cdecl initialize_assembly_operand(unsigned);
extern "C" int c1_test_assembly_operand_storage(void)
{
    CompilerAssemblyOperand *operand;
    CompilerAssemblyResetState before;
    BYTE zeros[12] = {0};
    unsigned index, other;
    for (index = 0; index < 2; ++index) {
        memset(&c1_bss.assembly, 0xa5, sizeof(c1_bss.assembly));
        c1_bss.assembly.current_operand = &c1_bss.assembly.operands[2];
        before = c1_bss.assembly;
        operand = initialize_assembly_operand(index);
        if (operand != &c1_bss.assembly.operands[index] ||
            operand->value != &c1_bss.assembly.values[index] ||
            operand->form != 15 || operand->modifiers != 0 ||
            operand->unmodeled_06[0] != 0xa5 || operand->unmodeled_06[1] != 0xa5 ||
            operand->reset_08 || operand->reset_0c ||
            memcmp(operand->value, zeros, sizeof(zeros))) return 0;
        for (other = 0; other < 3; ++other)
            if (other != index &&
                (memcmp(&c1_bss.assembly.operands[other], &before.operands[other], sizeof(CompilerAssemblyOperand)) ||
                 memcmp(&c1_bss.assembly.values[other], &before.values[other], sizeof(CompilerAssemblyValue)))) return 0;
        if (c1_bss.assembly.current_operand != before.current_operand ||
            c1_bss.assembly.operand_index != before.operand_index ||
            c1_bss.assembly.reset_54 != before.reset_54 ||
            c1_bss.assembly.reset_5c != before.reset_5c ||
            c1_bss.assembly.reset_08 != before.reset_08 ||
            c1_bss.assembly.reset_60 != before.reset_60 ||
            c1_bss.assembly.reset_9fc4 != before.reset_9fc4 ||
            c1_bss.assembly.reset_9ffc != before.reset_9ffc ||
            c1_bss.assembly.first_form != before.first_form ||
            c1_bss.assembly.brace_depth != before.brace_depth ||
            c1_bss.assembly.reset_68 != before.reset_68 ||
            c1_bss.assembly.reset_04 != before.reset_04 ||
            c1_bss.assembly.reset_4c != 0) return 0;
    }
    return 1;
}

extern "C" int __cdecl compare_label_spelling(const char *, const char *);
extern "C" int c1_test_assembly_casefold(void)
{
    const char *same = "mov";
    return compare_label_spelling(NULL, NULL) == 0 &&
        compare_label_spelling(same, same) == 0 &&
        compare_label_spelling(NULL, same) == -1 &&
        compare_label_spelling(same, NULL) == -1 &&
        compare_label_spelling("mOv", "MOV") == 0 &&
        compare_label_spelling("A", "b") == -33 &&
        compare_label_spelling("a", "B") == 31 &&
        compare_label_spelling("ab", "a") == 98 &&
        compare_label_spelling("", "A") == -65;
}

extern "C" CompilerAssemblyExpression *__cdecl make_assembly_register(WORD);
extern "C" int c1_test_assembly_register_storage(void)
{
    union { unsigned alignment; BYTE bytes[32]; } memory;
    CompilerAssemblyValue value;
    CompilerAssemblyOperand operand;
    CompilerAssemblyExpression *expression;
    unsigned i;
    memset(memory.bytes, 0xa5, sizeof(memory.bytes));
    memset(&value, 0xa5, sizeof(value));
    memset(&operand, 0xa5, sizeof(operand));
    operand.value = &value;
    c1_bss.assembly.reset_9fc4 = 0;
    c1_bss.assembly.current_operand = &operand;
    c1_bss.arenas[3].cursor = memory.bytes;
    c1_bss.arenas[3].committed_limit = memory.bytes + sizeof(memory.bytes);
    expression = make_assembly_register(1);
    if ((BYTE *)expression != memory.bytes || c1_bss.arenas[3].cursor != memory.bytes + 12 ||
        expression->kind != 3 || expression->register_code != 1 || operand.value != &value ||
        operand.form != 2 || operand.modifiers != 0xa5 || operand.reset_08 != 0xa5a5a5a5U ||
        operand.reset_0c != 0xa5a5a5a5U) return 0;
    for (i = 0; i < sizeof(memory.bytes); ++i)
        if (i >= 2 && (i < 8 || i >= 12) && memory.bytes[i] != 0xa5) return 0;
    for (i = 0; i < sizeof(value); ++i)
        if (value.unmodeled[i] != (i == 8 ? 4 : i == 10 ? 1 : 0xa5)) return 0;
    return operand.unmodeled_06[0] == 0xa5 && operand.unmodeled_06[1] == 0xa5;
}

extern "C" int c1_test_assembly_reference_flags(void)
{
    CompilerIrSymbol symbol, expected;
    unsigned count, epoch;
    for (count = 0; count < 4; ++count) for (epoch = 0; epoch < 2; ++epoch) {
        memset(&symbol, 0xa5, sizeof(symbol));
        symbol.reference_flags = (0xa5a5a5a5U & ~7U) | count | ((epoch ^ 1U) << 2);
        memcpy(&expected, &symbol, sizeof(symbol));
        expected.reference_flags = (expected.reference_flags & ~7U) |
            (count < 3 ? count + 1 : 3) | (epoch << 2);
        c1_data.ir_reference_epoch = 0xf0U | epoch;
        symbol.MarkAssemblyReference();
        if (memcmp(&symbol, &expected, sizeof(symbol))) return 0;
    }
    return 1;
}

extern "C" CompilerAssemblyExpression *__cdecl make_assembly_register_expression(WORD);
extern "C" void __cdecl emit_assembly_immediate(unsigned);
extern "C" BYTE __cdecl assembly_width_code(unsigned);
extern "C" int c1_test_assembly_address_storage(void)
{
    union { unsigned alignment; BYTE bytes[32]; } memory;
    CompilerAssemblyValue value, expected;
    CompilerAssemblyOperand operand;
    CompilerAssemblyExpression *expression;
    unsigned pass, i;
    for (pass = 0; pass < 2; ++pass) {
        memset(memory.bytes, 0xa5, sizeof(memory.bytes));
        memset(&value, 0xa5, sizeof(value));
        memset(&operand, 0xa5, sizeof(operand));
        value.operand.register_code = pass ? 1 : 0;
        value.operand.index_register = 0;
        value.operand.displacement = 0xfffffffeU;
        memcpy(&expected, &value, sizeof(value));
        if (pass) expected.operand.index_register = 6;
        else expected.operand.register_code = 6;
        expected.operand.kind_flags = (expected.operand.kind_flags & 0xef) | 0x16;
        operand.value = &value;
        c1_bss.assembly.current_operand = &operand;
        c1_bss.arenas[3].cursor = memory.bytes;
        c1_bss.arenas[3].committed_limit = memory.bytes + sizeof(memory.bytes);
        expression = make_assembly_register_expression(6);
        if ((BYTE *)expression != memory.bytes || expression->kind != 1 ||
            expression->payload || c1_bss.arenas[3].cursor != memory.bytes + 12 ||
            memcmp(&value, &expected, sizeof(value))) return 0;
        for (i = 0; i < sizeof(memory.bytes); ++i)
            if ((i == 2 || i == 3 || i >= 8) && memory.bytes[i] != 0xa5) return 0;
        expected.operand.displacement = 1;
        emit_assembly_immediate(3);
        if (memcmp(&value, &expected, sizeof(value))) return 0;
    }
    return assembly_width_code(1) == 0 && assembly_width_code(2) == 1 &&
        assembly_width_code(4) == 2 && assembly_width_code(8) == 3 &&
        assembly_width_code(10) == 4 && assembly_width_code(6) == 5 &&
        assembly_width_code(0xffff) == 2 && assembly_width_code(0xfffe) == 5 &&
        assembly_width_code(0) == 15 && assembly_width_code(0xffffffffU) == 15;
}

extern "C" void __cdecl select_assembly_encoding(void);
extern "C" void __cdecl write_assembly_instruction(void);
extern "C" int c1_test_assembly_mov_record(void)
{
    unsigned i;
    static const BYTE expected[12] = {12, 1, 4, 1, 0, 4, 0x44, 0x33, 0x22, 0x11, 5, 0};
    memset(&c1_bss.assembly, 0xa5, sizeof(c1_bss.assembly));
    memset(&c1_bss.assembly_record, 0xa5, sizeof(c1_bss.assembly_record));
    c1_bss.assembly.first_form = 13;
    c1_bss.assembly.reset_54 = c1_bss.assembly.reset_5c = 1;
    c1_bss.assembly.operand_index = 2;
    for (i = 0; i < 2; ++i) {
        CompilerAssemblyOperand *operand = &c1_bss.assembly.operands[i];
        CompilerAssemblyOperandValue *value = &c1_bss.assembly.values[i].operand;
        operand->value = &c1_bss.assembly.values[i];
        operand->form = 2; operand->modifiers = 0;
        operand->reset_08 = operand->reset_0c = 0;
        value->kind_flags = i ? 22 : 4;
        value->register_code = i ? 6 : 1;
        value->index_register = value->segment_and_scale = 0;
        value->displacement = 0x11223344;
    }
    select_assembly_encoding();
    write_assembly_instruction();
    if (memcmp(&c1_bss.assembly_record, expected, sizeof(expected))) return 0;
    for (i = sizeof(expected); i < sizeof(c1_bss.assembly_record); ++i)
        if (((BYTE *)&c1_bss.assembly_record)[i] != 0xa5) return 0;
    for (i = 0; i < 2; ++i) {
        CompilerAssemblyOperandValue *value = &c1_bss.assembly.values[i].operand;
        if (value->segment_and_scale != 63 || value->index_register != 63 ||
            value->register_code != (i ? 5 : 0) || value->displacement != 0x11223344 ||
            value->symbol != (CompilerIrSymbol *)0xa5a5a5a5U ||
            c1_bss.assembly.operands[i].unmodeled_06[0] != 0xa5 ||
            c1_bss.assembly.operands[i].unmodeled_06[1] != 0xa5) return 0;
    }
    return 1;
}

extern "C" CompilerIrSymbol *__cdecl create_segment_symbol(CompilerNameEntry *, CompilerNameEntry *, unsigned, unsigned);
extern "C" int c1_test_segment_symbol_storage(void)
{
    DWORD owned[15];
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)(owned + 1), expected;
    CompilerNameEntry name, class_name;
    CompilerSymbolBinding active, free_next;
    CompilerTranslationUnit unit;
    CompilerType segment_type;
    unsigned recycled, i;
    c1_data.translation_unit = &unit;
    c1_data.builtin_types.type_0802 = &segment_type;
    c1_data.segment_symbol_cache_index = 9;
    c1_data.record_sizes.known.segment_symbol_bytes = 52;
    for (recycled = 0; recycled != 2; ++recycled) {
        unsigned identifier = recycled ? 5 : 23;
        for (i = 0; i < 15; ++i) owned[i] = 0xa5a5a5a5U;
        unit.symbols = &active;
        c1_bss.arenas[0].cursor = (BYTE *)symbol;
        c1_bss.arenas[0].committed_limit = (BYTE *)symbol + 52;
        if (recycled) {
            symbol->binding.next = &free_next;
            c1_data.abstract_symbol_free = &symbol->binding;
            /* This path has an authentic whole52-byte clear. */
            memset(&expected, 0, sizeof(expected));
        } else {
            c1_data.abstract_symbol_free = NULL;
            /* Fresh storage has no inferred zero/default fields. */
            memcpy(&expected, symbol, sizeof(expected));
        }
        expected.kind = 14;
        expected.binding.name = &name;
        expected.binding.next = &active;
        expected.record_id = (expected.record_id & 0xff000000U) | identifier;
        if (identifier < 16) expected.reference_flags = (expected.reference_flags & 7) | (identifier << 3);
        expected.segment_kind = 1;
        expected.value_type = &segment_type;
        expected.segment_class = &class_name;
        if (create_segment_symbol(&name, &class_name, identifier, 1) != symbol ||
            memcmp(symbol, &expected, sizeof(expected)) || unit.symbols != &symbol->binding ||
            c1_data.abstract_symbol_free != (recycled ? &free_next : NULL) ||
            c1_bss.arenas[0].cursor != (BYTE *)symbol + (recycled ? 0 : 52) ||
            owned[0] != 0xa5a5a5a5U || owned[14] != 0xa5a5a5a5U) return 0;
    }
    return 1;
}

extern "C" CompilerType *__cdecl compose_declaration_type(CompilerType *, CompilerType *, int, int, int);
extern "C" int c1_test_simple_function_type_composition(void)
{
    struct { DWORD before; CompilerTypeList value; DWORD after; } empty, single;
    CompilerType scalar, left, right, scalar_before, left_before, right_before;
    CompilerTypeShape left_shape, right_shape, left_shape_before, right_shape_before;
    BYTE empty_before[sizeof(empty)], single_before[sizeof(single)];
    memset(&empty, 0x96, sizeof(empty));
    memset(&single, 0xa7, sizeof(single));
    memset(&scalar, 0x69, sizeof(scalar));
    scalar.flags = 0x63; scalar.attributes = 0; scalar.shape = NULL; scalar.auxiliary = 0;
    empty.value.count = 0;
    single.value.count = 1;
    single.value.types[0] = &scalar;
    memset(&left_shape, 0x97, sizeof(left_shape));
    memset(&right_shape, 0x68, sizeof(right_shape));
    left_shape.flags = right_shape.flags = 0x404;
    left_shape.base = right_shape.base = NULL;
    left_shape.parameters = &empty.value; right_shape.parameters = &single.value;
    memset(&left, 0x59, sizeof(left)); memset(&right, 0x6a, sizeof(right));
    left.shape = &left_shape; right.shape = &right_shape;
    left_before = left; right_before = right; scalar_before = scalar;
    left_shape_before = left_shape; right_shape_before = right_shape;
    memcpy(empty_before, &empty, sizeof(empty)); memcpy(single_before, &single, sizeof(single));
    if (compose_declaration_type(&left, &right, 1, 1, 0) != &right ||
        compose_declaration_type(&right, &left, 1, 1, 0) != &right ||
        compose_declaration_type(&left, &left, 1, 1, 0) != &left)
        return 0;
    return !memcmp(&left, &left_before, sizeof(left)) &&
        !memcmp(&right, &right_before, sizeof(right)) &&
        !memcmp(&scalar, &scalar_before, sizeof(scalar)) &&
        !memcmp(&left_shape, &left_shape_before, sizeof(left_shape)) &&
        !memcmp(&right_shape, &right_shape_before, sizeof(right_shape)) &&
        !memcmp(&empty, empty_before, sizeof(empty)) &&
        !memcmp(&single, single_before, sizeof(single));
}

extern "C" CompilerIrSymbol *__cdecl find_assembly_member(CompilerIrSymbol *, CompilerNameEntry *);
extern "C" int c1_test_assembly_member_lookup(void)
{
    CompilerIrSymbol first, middle, last;
    CompilerNameEntry alignment, wanted, absent;
    CompilerNameEntry *saved = c1_data.builtin_names.alignment_member;
    int ok;
    memset(&first, 0xa5, sizeof(first));
    memset(&middle, 0x5a, sizeof(middle));
    memset(&last, 0x3c, sizeof(last));
    first.binding.name = &alignment; first.binding.next = &middle.binding;
    middle.binding.name = &wanted; middle.binding.next = &last.binding;
    last.binding.name = &wanted; last.binding.next = NULL;
    c1_data.builtin_names.alignment_member = &alignment;
    ok = !find_assembly_member(NULL, &wanted) &&
        find_assembly_member(&first, &wanted) == &middle &&
        !find_assembly_member(&first, &alignment) &&
        !find_assembly_member(&first, &absent) &&
        find_assembly_member(&last, &wanted) == &last &&
        first.binding.next == &middle.binding && middle.binding.next == &last.binding &&
        first.kind == 0xa5 && middle.kind == 0x5a && last.kind == 0x3c;
    c1_data.builtin_names.alignment_member = saved;
    return ok;
}

extern "C" CompilerAssemblyExpression *__cdecl make_assembly_current_location(CompilerAssemblyExpression *);
extern "C" int c1_test_assembly_offset_write(void)
{
    CompilerAssemblyOperand *saved = c1_bss.assembly.current_operand;
    CompilerAssemblyOperand operand;
    CompilerAssemblyValue value;
    CompilerAssemblyExpression expression;
    CompilerIrSymbol symbol;
    int ok;
    memset(&operand, 0xa5, sizeof(operand));
    memset(&value, 0x6b, sizeof(value));
    memset(&expression, 0x3c, sizeof(expression));
    operand.value = &value;
    value.operand.symbol = &symbol;
    value.operand.kind_flags = 0xd5;
    c1_bss.assembly.current_operand = &operand;
    ok = make_assembly_current_location(&expression) == &expression &&
        expression.kind == 0x3c3c && expression.payload == 0x3c3c3c3c &&
        value.operand.symbol == &symbol && value.operand.kind_flags == 0xc3 &&
        value.operand.displacement == 0x6b6b6b6b && value.operand.register_code == 0x6b &&
        operand.form == 2 && operand.modifiers == 0xa5 && operand.unmodeled_06[0] == 0xa5;
    c1_bss.assembly.current_operand = saved;
    return ok;
}

extern "C" void __cdecl select_assembly_encoding(void);
extern "C" int c1_test_assembly_imul_duplicate(void)
{
    CompilerAssemblyResetState saved = c1_bss.assembly;
    unsigned i;
    int ok;
    memset(&c1_bss.assembly, 0xa5, sizeof(c1_bss.assembly));
    c1_bss.assembly.first_form = 20;
    c1_bss.assembly.reset_54 = 163;
    c1_bss.assembly.reset_5c = 6;
    c1_bss.assembly.operand_index = 2;
    for (i = 0; i < 2; ++i) {
        c1_bss.assembly.operands[i].value = &c1_bss.assembly.values[i];
        c1_bss.assembly.operands[i].form = (BYTE)(i ? 15 : 2);
        c1_bss.assembly.operands[i].modifiers = 0;
        c1_bss.assembly.operands[i].source_symbol = NULL;
        c1_bss.assembly.operands[i].symbol = NULL;
        memset(&c1_bss.assembly.values[i], 0, sizeof(CompilerAssemblyValue));
    }
    c1_bss.assembly.values[0].operand.kind_flags = 4;
    c1_bss.assembly.values[0].operand.register_code = 2;
    c1_bss.assembly.values[1].operand.kind_flags = 2;
    c1_bss.assembly.values[1].operand.displacement = 4;
    c1_bss.assembly.current_operand = &c1_bss.assembly.operands[1];
    select_assembly_encoding();
    ok = c1_bss.assembly.operand_index == 3 &&
        c1_bss.assembly.encoded_opcode == 163 && c1_bss.assembly.encoding_flags == 2 &&
        c1_bss.assembly.operands[2].value == &c1_bss.assembly.values[2] &&
        c1_bss.assembly.operands[1].value == &c1_bss.assembly.values[1] &&
        !memcmp(&c1_bss.assembly.values[0], &c1_bss.assembly.values[1], 12) &&
        c1_bss.assembly.values[2].operand.displacement == 4 &&
        c1_bss.assembly.values[2].operand.kind_flags == 2 &&
        c1_bss.assembly.values[0].operand.register_code == 1 &&
        c1_bss.assembly.operands[2].unmodeled_06[0] == 0xa5 &&
        c1_bss.assembly.operands[1].unmodeled_06[1] == 0xa5;
    c1_bss.assembly = saved;
    return ok;
}

extern "C" int __cdecl find_intrinsic_number(CompilerNameEntry *);
extern "C" int c1_test_intrinsic_lookup_relations(void)
{
    static const struct { const char *name; int number; } rows[] = {
        {"memset",46}, {"memcpy",47}, {"strlen",54}, {"strcmp",56},
        {"abs",34}, {"labs",45}, {"_rotl",3}, {"_rotr",2},
        {"memCopy",-1}, {"Memcpy",-1}, {"unknown",-1}, {"",-1}
    };
    CompilerNameEntry identifier;
    unsigned i;
    for (i = 0; i < sizeof(rows)/sizeof(rows[0]); ++i) {
        identifier.name = (char *)rows[i].name;
        if (find_intrinsic_number(&identifier) != rows[i].number) return 0;
    }
    return 1;
}
