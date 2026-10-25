#include "input.h"
#include <new.h>
#include "keywords.h"
#include "frontend.h"
#include "global-record.h"
#include <recon.h>

extern "C" int __cdecl create_input(int, CompilerInput **, CompilerTextInput **);

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
    /* The only recovered named-browser data is its owned filename at +4.
     * Its final four allocation bytes are deliberately never inspected. */
    copy = *(char **)((BYTE *)c1_data.named_browser + 4);
    if (!copy || copy == filename) return 0;
    for (i = 0; i < sizeof(filename); ++i)
        if (copy[i] != filename[i]) return 0;
    recon_free(copy); /* _strdup owns a CRT allocation, not an operator-new block. */
    recon_cpp_delete(c1_data.named_browser);
    recon_cpp_delete(default_object);
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
    return buffer.Ensure(0) && buffer.cursor == bytes + 8 && buffer.good == 0x5a;
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
