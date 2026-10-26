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
    c1_bss.parser_early_result = c1_bss.function_output_mode = 0;
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
