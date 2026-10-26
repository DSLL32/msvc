/* Narrow native policy checks. Synthetic unsupported inputs establish rejection,
 * not equivalence or public reachability. Historical proof: tagged 17c78f8c6. */
#include <sys/wait.h>
#include <unistd.h>
#include <io.h>
#include <fcntl.h>
#include "macro-expansion.h"
#include "member-access.h"
#include "function-finalization.h"
#include "type-diagnostics.h"
#include "lookahead.h"
#include "pragma.h"
#include "asm.h"
#include "c1xx.h"
#include <recon-tool.h>
#include "debug-types.h"
#include "debug-info.h"
#include "unary-expressions.h"
#include "class-members.h"
#include "binary-expressions.h"
#include "parser.h"
#include "lexer.h"
#include "browser.h"
#include "virtual-table-output.h"
#include "initializer-records.h"
#include "types.h"
#include "class-type-sets.h"
#include "source-input.h"
#include "member-pointers.h"
#include "numeric-conversion.h"
#include "pch-relocation.h"
#include "symbol-output.h"
#include "function-definitions.h"
#include "control-ir.h"
#include "operators.h"
#include <float.h>
#include "text-format.h"
#include "parser.h"
#include <recon.h>

static int traps(void (*invoke)(void))
{
    int child = fork(), status;
    if (child < 0) return 0;
    if (!child) { invoke(); _exit(99); }
    if (waitpid(child, &status, 0) != child) return 0;
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGTRAP;
}
static void single_null(void) { has_single_function(0); }
static void overload_null(void) { count_emitted_overloads(0); }
static void methods_null(void) { unsigned short flags = 0; count_debug_methods(0, &flags); }
static void method_list_null(void)
{
    static unsigned char bytes[4096];
    debug_record_buffer = bytes;
    emit_debug_method_list(0);
}
static void real_width(void) { primitive_debug_type(0x10b); }
static void integer_width(void) { primitive_debug_type(0x49); }
static void cached_lookahead(void) { parser_lookahead = 2; begin_implicit_statement_scope(0); }
static void multiply_width(void) { multiplication_overflows(1, ~0U, 0, 3); }
static void empty_browser_name(void)
{
    char result[16]; options.option_Fr = ""; format_browser_temporary_name(result);
}
static void unknown_browser_category(void)
{
    BrowserRecord record = {0}; CppSymbol symbol = {0};
    symbol.name = "plain"; symbol.kind = 12; symbol.declaration_flags = 0x200;
    fill_browser_declaration(&record, &symbol);
}
#define TRAP_TEST(name, body) static int name(void) { return traps(body); }
TRAP_TEST(check_single_null, single_null)
TRAP_TEST(check_overload_null, overload_null)
TRAP_TEST(check_methods_null, methods_null)
TRAP_TEST(check_method_list_null, method_list_null)
TRAP_TEST(check_real_width, real_width)
TRAP_TEST(check_integer_width, integer_width)
TRAP_TEST(check_cached_lookahead, cached_lookahead)
TRAP_TEST(check_multiply_width, multiply_width)
TRAP_TEST(check_empty_browser_name, empty_browser_name)
TRAP_TEST(check_unknown_browser_category, unknown_browser_category)

/* These ten returns were observed directly in original and native at the
 * existing multiply boundary. Keep concrete independent expectations. */
static int check_multiply_returns(void)
{
    static const unsigned values[][5] = {
        {0U,4U,0U,4U,0}, {255U,2U,1U,1U,1},
        {4294967168U,1U,0U,1U,0}, {4294967168U,4294967295U,0U,1U,1},
        {4294934528U,1U,0U,2U,0}, {4294934528U,4294967295U,0U,2U,1},
        {2147483648U,1U,0U,4U,0}, {2147483648U,4294967295U,0U,4U,1},
        {2147483647U,2U,0U,4U,1}, {4294967295U,2U,1U,4U,1}
    };
    unsigned i;
    for (i=0; i<RECON_COUNT(values); ++i)
        if (multiplication_overflows(values[i][0],values[i][1],(short)values[i][2],
                (unsigned short)values[i][3]) != (int)values[i][4]) return 0;
    return 1;
}
static int check_primitive_types(void)
{
    return primitive_debug_type(0x44)==0x12 && primitive_debug_type(0x64)==0x74 &&
        primitive_debug_type(0x80)==3 && primitive_debug_type(0)==0;
}
static int check_browser_filename(void)
{
    char result[16];
    options.option_Fr="a.sbr"; format_browser_temporary_name(result);
    if (strcmp(result,"a.sbt")) return 0;
    options.option_Fr="a.sbt"; format_browser_temporary_name(result);
    return strcmp(result,"a.sbu")==0;
}
static int check_single_function(void)
{
    CppSymbol first={0}, second={0};
    first.kind=6;
    if (has_single_function(&first)!=1) return 0;
    first.next=&second; second.kind=7;
    if (has_single_function(&first)!=0) return 0;
    second.kind=3;
    return has_single_function(&first)==1;
}
/* Unit-only qualification of dormant frontend support. This deliberately
   calls the identified helpers, without opening the original CLI predicate. */
CppSymbol *__stdcall find_memory_intrinsic(unsigned operation);
static int check_memory_intrinsic_lookup(void)
{
    CppSymbol *compare, *copy, *fill;
    TypeDescription integer = {0};
    initialize_preprocessor();
    initialize_type_table();
    initialize_global_scope();
    integer.flags = 0x64;
    type_0064 = intern_type(&integer);
    /* The helper consumes the named declaration identity only; it neither
       checks nor calls the declaration's function type. */
    compare = new_symbol(intern_identifier("memcmp"), 0);
    copy = new_symbol(intern_identifier("memcpy"), 0);
    fill = new_symbol(intern_identifier("memset"), 0);
    return find_memory_intrinsic(0xaa) == compare &&
        find_memory_intrinsic(0xac) == copy &&
        find_memory_intrinsic(0xad) == fill && find_memory_intrinsic(0) == 0;
}
static int check_auxiliary_real_encodings(void)
{
    static const unsigned char expected[12] = {
        0, 0, 0xa0, 0x3f, 0, 0, 0, 0, 0, 0, 0x0c, 0xc0
    };
    unsigned char actual[13];
    float single = 1.25f;
    double real = -3.5;
    FILE *saved = auxiliary_ir_stream;
    FILE *file = fopen("unit-auxiliary-real.bin", "w+b");
    int ok;
    if (!file) return 0;
    auxiliary_ir_stream = file;
    write_ir_float(&single);
    write_ir_double(&real);
    auxiliary_ir_stream = saved;
    ok = fseek(file, 0, SEEK_SET) == 0 &&
        fread(actual, 1, sizeof(actual), file) == sizeof(expected) &&
        !memcmp(actual, expected, sizeof(expected));
    if (fclose(file)) ok = 0;
    if (remove("unit-auxiliary-real.bin")) ok = 0;
    return ok;
}
/* The grammar's legacy segment/self family is unit-qualified: NT mode does
   not provide the historical source spelling used by this internal path. */
void __stdcall validate_based_self_source(IrToken *expression);
int __stdcall equal_based_expressions(IrToken *first, IrToken *second);
static int check_based_self_cast(void)
{
    return validate_based_self_cast(0x0802) == 8;
}
static int check_based_self_pointer_source(void)
{
    CType pointer_type = {0};
    TypeShape pointer_shape = {0};
    IrToken storage = {0}, cast = {0};
    CppSymbol object = {0};
    int before = error_count;
    pointer_shape.flags = 1;
    pointer_shape.attributes = 1;
    pointer_type.related = &pointer_shape;
    storage.kind = 2; storage.flags = 4; storage.type = &pointer_type;
    storage.value.pointer = &object;
    cast.kind = 0x6e; cast.flags = 1; cast.type = &pointer_type;
    cast.value.pointer = &storage;
    validate_based_self_source(&cast);
    return error_count == before && cast.value.pointer == &storage &&
        cast.type == &pointer_type;
}
static int check_based_expression_identity(void)
{
    CppSymbol one = {0}, two = {0};
    IrToken left = {0}, right = {0}, first = {0}, second = {0};
    if (!equal_based_expressions(0, 0) || equal_based_expressions(&left, 0)) return 0;
    left.kind = right.kind = 0x1b;
    left.value.pointer = right.value.pointer = &one;
    if (!equal_based_expressions(&left, &right)) return 0;
    right.value.pointer = &two;
    if (equal_based_expressions(&left, &right)) return 0;
    left.kind = right.kind = 0x89;
    if (!equal_based_expressions(&left, &right)) return 0;
    first.kind = second.kind = 2; first.flags = second.flags = 4;
    first.value.pointer = second.value.pointer = &one;
    left.kind = right.kind = 0x1a;
    left.value.pointer = &first; right.value.pointer = &second;
    if (!equal_based_expressions(&left, &right)) return 0;
    second.value.pointer = &two;
    if (equal_based_expressions(&left, &right)) return 0;
    second.value.pointer = &one;
    left.kind = right.kind = 0x46;
    if (!equal_based_expressions(&left, &right)) return 0;
    right.flags = 1;
    if (equal_based_expressions(&left, &right)) return 0;
    right.flags = 0;
    left.kind = right.kind = 0x6e;
    return !equal_based_expressions(&left, &right);
}
static int check_legacy_prefix_owners(void)
{
    CppSymbol symbol = {0}, base = {0}, other = {0};
    CType type = {0};
    TypeShape source = {0}, following = {0}, function = {0};
    TypeShape head = {0}, tail = {0}, *copy = 0;
    base.kind = 13; base.auxiliary.segment_kind = 8;
    source.attributes = 5; source.related = &following;
    source.data.based_symbol = &base;
    copy_shape_prefix(&symbol, &source, &copy, 0);
    if (!copy || copy == &source || copy->related || copy->flags != source.flags ||
        copy->attributes != 5 || copy->data.based_symbol != &base ||
        source.related != &following) return 0;
    function.flags = 4; type.related = &function; symbol.type = &type;
    symbol.attributes = 0x80;
    check_declarator_prefix(&symbol, copy);
    if (symbol.attributes != 0x82 || base.auxiliary.segment_kind != 0) return 0;
    if (merge_type_shape_prefix(&head, copy) != &head ||
        head.attributes != 5 || head.data.based_symbol != &base) return 0;
    tail.attributes = 5; tail.data.based_symbol = &other;
    if (merge_type_shape_prefix(&head, &tail) != &head ||
        head.attributes != 5 || head.data.based_symbol != &base) return 0;
    copy->data.based_symbol = 0;
    check_declarator_prefix(&symbol, copy);
    return symbol.attributes == 0x80;
}
/* Explicit unit-only qualification of application ownership helpers. No
   internal state here is presented as a captured public invocation. */
CppSymbol *__stdcall find_tag_declaration(CppSymbol *symbol);
int __stdcall temporary_cleanup_is_suppressed(IrToken *object, unsigned index);
void __stdcall mark_pch_virtual_thunk_key(VirtualThunkKey *key);
extern unsigned pch_encoded_pointer;
extern unsigned char *pch_reference_base;
extern unsigned short pch_reference_page;
static int check_frontend_owner_helpers(void)
{
    CppSymbol alias = {0}, declaration = {0};
    FrontendRecord record = {0}, *head, *opening, *tail;
    int saved = allow_incomplete_class;
    alias.kind = 0x80; alias.next = &declaration;
    if (find_tag_declaration(&alias) != &declaration) return 0;
    allow_incomplete_class = 0;
    if (temporary_cleanup_is_suppressed(0, 17)) return 0;
    allow_incomplete_class = 1;
    if (!temporary_cleanup_is_suppressed(0, 18)) return 0;
    allow_incomplete_class = saved;
    reset_frontend_records();
    record.kind = 12; append_frontend_record(&record, sizeof(record)); head = ir_record_tail;
    record.kind = 17; append_frontend_record(&record, sizeof(record)); opening = ir_record_tail;
    record.kind = 12; append_frontend_record(&record, sizeof(record)); tail = ir_record_tail;
    remove_matching_frontend_record(17);
    if (ir_record_head != head || ir_record_tail != tail || head->next != tail ||
        tail->previous != head || opening->next != 0) return 0;
    remove_matching_frontend_record(17);
    return ir_record_head == head && ir_record_tail == tail && head->next == tail;
}
static int check_thunk_key_relocation(void)
{
    VirtualThunkKey *key = allocate_permanent(sizeof(VirtualThunkKey));
    CppSymbol *function = allocate_permanent(sizeof(CppSymbol));
    unsigned expected;
    key->kind = 3; key->function = function;
    key->displacement = 8; key->virtual_displacement = 12;
    mark_pch_virtual_thunk_key(key);
    expected = ((unsigned)&key->function - (unsigned)pch_reference_base) |
        ((unsigned)pch_reference_page << 16);
    return pch_encoded_pointer == expected && key->function == function &&
        key->kind == 3 && key->displacement == 8 && key->virtual_displacement == 12;
}
/* Native C view of the nonvirtual12-byte class owners. The alias calls the
   genuine C++ method using its ECX/no-stack ABI; no application wrapper added. */
typedef struct TestDatabaseBlock {
    unsigned remaining;
    unsigned char *cursor;
    struct TestDatabaseBlock *next;
} TestDatabaseBlock;
typedef struct TestDatabasePool {
    TestDatabaseBlock *first, *current;
    unsigned used;
} TestDatabasePool;
typedef char TestDatabaseBlockSize[(sizeof(TestDatabaseBlock) == 12) ? 1 : -1];
typedef char TestDatabasePoolSize[(sizeof(TestDatabasePool) == 12) ? 1 : -1];
extern void __attribute__((thiscall)) test_release_database_pool(TestDatabasePool *pool)
    __asm__("_ZN12DatabasePool7releaseEv");
static int check_database_pool_release(void)
{
    TestDatabasePool pool = {0};
    TestDatabaseBlock *first, *second;
    test_release_database_pool(&pool);
    first = malloc(sizeof(*first)); second = malloc(sizeof(*second));
    if (!first || !second) { free(first); free(second); return 0; }
    first->remaining = second->remaining = 0;
    first->cursor = (unsigned char *)(first + 1); second->cursor = (unsigned char *)(second + 1);
    first->next = second; second->next = 0;
    pool.first = first; pool.current = second; pool.used = 27;
    test_release_database_pool(&pool);
    /* Compare retained field values only; never dereference released blocks. */
    return pool.first == first && pool.current == second && pool.used == 27;
}
/* Legacy descriptor algebra is unit-qualified, without claiming that this
   NT frontend accepts a public segmented-expression spelling. */
static int check_segment_descriptor_expression(void)
{
    CType segment_type = {0}, offset_type = {0};
    TypeShape based = {0};
    CppSymbol segment = {0}, offset = {0};
    IrToken *segment_value, *offset_value, *result;
    int depth = type_description_depth;
    initialize_type_table();
    segment_type.flags = 0x802; segment.kind = 13; segment.type = &segment_type;
    offset_type.flags = 0x64; offset_type.related = &based;
    based.flags = 1; based.attributes = 4;
    offset.kind = 2; offset.type = &offset_type;
    data_pointer_kind = 1;
    segment_value = new_declared_symbol_reference(0x1b, &segment);
    offset_value = new_declared_symbol_reference(2, &offset);
    if (segment_value->kind != 0x1b || segment_value->flags != 4 ||
        segment_value->value.pointer != &segment || segment_value->type != &segment_type) return 0;
    result = segment_offset_expression(segment_value, offset_value);
    if (!result || result->kind != 0x68 || result->flags != 2 ||
        result->value.pointer != offset_value || result->tail.right != segment_value ||
        !result->type || result->type->flags != 0x64 || !result->type->related ||
        result->type->related->flags != 1 || result->type->related->attributes != 1 ||
        type_description_depth != depth) return 0;
    offset_value->flags |= 0x80;
    result = segment_offset_expression(segment_value, offset_value);
    return result && result->flags == 0x82 && type_description_depth == depth &&
        segment_offset_expression(0, offset_value) == 0;
}
static int check_initializer_address_widths(void)
{
    CppSymbol symbol = {0}; CType type = {0}; TypeShape function = {0};
    unsigned i;
    static const unsigned data_widths[8] = {4,4,8,8,4,4,4,4};
    symbol.type = &type; symbol.kind = 2; type.flags = 0x64;
    data_pointer_bytes = code_pointer_bytes = 4;
    if (initializer_address_size(0) != 0) return 0;
    for (i=0; i<8; ++i) {
        type.attributes = (unsigned short)i;
        if (initializer_address_size(&symbol) != data_widths[i]) return 0;
    }
    type.attributes = 0; data_pointer_bytes = 8;
    if (initializer_address_size(&symbol) != 8) return 0;
    function.flags = 4; type.related = &function;
    symbol.kind = 6;
    for (i=0; i<8; ++i) {
        type.attributes = (unsigned short)i;
        if (initializer_address_size(&symbol) != 4) return 0;
    }
    symbol.kind = 13; type.flags = 0x802; type.related = 0;
    return initializer_address_size(&symbol) == 2;
}
/* Explicit unit-only legacy segment-debug ownership qualification. */
static int check_segment_debug_owners(void)
{
    CType segment_type = {0};
    CppSymbol segment = {0}, *debug_symbol;
    unsigned char leaf[32] = {0};
    DebugTypeFixup *first;
    initialize_preprocessor(); initialize_type_table(); initialize_global_scope();
    segment.kind = 13; segment.name = intern_identifier("DEVICE_SEG");
    segment_type.flags = 0x802; segment.type = &segment_type;
    options.configuration.debug_info = 0;
    ensure_debug_constant_symbol(&segment);
    if (segment.field28) return 0;
    options.configuration.debug_info = 1;
    next_symbol_index = 41; browser_symbol_limit = 0x7fffffffU;
    ensure_debug_constant_symbol(&segment);
    debug_symbol = (CppSymbol *)segment.field28;
    if (!debug_symbol || debug_symbol->kind != 9 || debug_symbol->type != &segment_type ||
        strcmp(debug_symbol->name, "__seg__DEVICE_SEG") || debug_symbol->index != 41 ||
        debug_symbol->references != 1 || debug_symbol->flags != 0x84 ||
        debug_symbol->declaration_flags != 0x200 || debug_symbol->storage[0] != 0x42 ||
        debug_symbol->storage[1] != 0x80 || debug_symbol->attributes != 0x8004 ||
        find_scope_symbol(global_scope, debug_symbol->name, 0) != debug_symbol) return 0;
    ensure_debug_constant_symbol(&segment);
    if ((CppSymbol *)segment.field28 != debug_symbol || next_symbol_index != 42) return 0;
    debug_record_buffer = leaf;
    queue_debug_type_fixup(&segment, leaf+6, 4);
    first = debug_type_fixups;
    queue_debug_type_fixup(&segment, leaf+14, 2);
    return first && first->symbol == &segment && first->offset == 6 && first->bytes == 4 &&
        first->next && first->next->symbol == &segment && first->next->offset == 14 &&
        first->next->bytes == 2 && !first->next->next;
}
/* Explicit unit-only resource fallback owners; no resource exhaustion or
   forced public control flow is claimed. */
static int check_input_handle_retirement(void)
{
    SourceFile input[3] = {{0}};
    int first, second, result;
    include_depth = 2; active_input = 0;
    if (release_input_handle() != 0) return 0;
    first = _open("handle-unit.tmp", _O_CREAT|_O_TRUNC|_O_RDWR|_O_BINARY, 0600);
    second = _open("handle-unit.tmp", _O_RDWR|_O_BINARY);
    if (first < 0 || second < 0) return 0;
    input[0].handle = first; input[0].next = &input[1];
    input[1].handle = second; input[1].next = &input[2];
    input[2].handle = -1;
    active_input = input; include_depth = 4;
    result = release_input_handle();
    if (result != 1 || input[0].handle != first || input[1].handle != -1 ||
        include_depth != 4) return 0;
    if (release_input_handle() != 1 || input[0].handle != -1) return 0;
    result = release_input_handle();
    _unlink("handle-unit.tmp");
    return result == 0;
}
static int check_browser_fallback_state(void)
{
    void *memory = retry_malloc(32);
    if (!memory) return 0;
    memset(memory, 0x5a, 32); free(memory);
    browser_record_head = browser_record_count = browser_records_suppressed = 0;
    source_line_number = 47;
    browser_end_macro_expansion();
    if (browser_record_count != 1 || first_browser_record()->kind != 7) return 0;
    options.option_FR = "unit.sbr"; browser_filename = options.option_FR;
    options.option_W = 0;
    browser_symbol_rollover();
    return !options.option_FR && !browser_filename && browser_record_count == 2 &&
        first_browser_record()->kind == 7 && second_browser_record()->kind == 14;
}
/* Explicit unit-only legacy expression/type-owner qualification. */
static int check_untyped_operand_owner(void)
{
    TypeShape function_shape = {0};
    CType function_type = {0};
    CppSymbol function = {0};
    OverloadSet overload = {0};
    IrToken designator = {0}, other = {0}, *operand = &designator;
    initialize_preprocessor(); initialize_type_table();
    function_shape.flags = 3;
    function_type.flags = 0x140; function_type.related = &function_shape;
    function.kind = 7; function.type = &function_type;
    overload.kind = 16; overload.functions = &function;
    designator.kind = 2; designator.value.pointer = &overload;
    other.type = &function_type;
    resolve_untyped_binary_operand(0x51, &operand, &other);
    return operand == &designator && operand->value.pointer == &function &&
        operand->type && operand->type->related &&
        operand->type->related->flags == 0xb;
}
static int check_materialized_address_owner(void)
{
    IrToken operand = {0}, address = {0}, *result, *again, *assignment, *left, *right;
    initialize_preprocessor(); initialize_type_table();
    reset_materialized_value_index(); next_symbol_index = 101;
    browser_symbol_limit = 0x7fffffffU;
    operand.kind = 7; operand.type = type_0000;
    address.value.pointer = &operand;
    result = make_address_temporary(&address);
    if (!result || result->kind != 0x40 || result->type != operand.type) return 0;
    assignment = result->value.pointer; right = result->tail.right;
    if (!assignment || assignment->kind != 0x3d || assignment->tail.right != &operand) return 0;
    left = assignment->value.pointer;
    if (!left || !right || left == right || left->kind != 0xa7 || right->kind != 0xa7 ||
        left->value.integer != 101 || right->value.integer != 101 ||
        left->type != operand.type || right->type != operand.type) return 0;
    again = make_address_temporary(&address);
    return next_symbol_index == 102 && again->tail.right->value.integer == 101;
}
static int check_virtual_access_owners(void)
{
    CppSymbol base = {0}, via = {0}, derived = {0};
    ClassInfo via_info = {0};
    BaseClassLink direct = {0}, indirect = {0}, suppressed = {0};
    VirtualTableGroup group = {0};
    int base_index, via_index;
    initialize_preprocessor(); initialize_cpp_context_stack();
    cpp_context_stack = add_inline_pointer(cpp_context_stack, &base);
    cpp_context_stack = add_inline_pointer(cpp_context_stack, &via);
    if (!locate_inline_pointer(cpp_context_stack, &base, &base_index) ||
        !locate_inline_pointer(cpp_context_stack, &via, &via_index)) return 0;
    via.detail.class_info = &via_info;
    via_info.virtual_table_groups = new_vector(2, 0);
    group.tag = &base; group.flags = 2;
    append_vector(via_info.virtual_table_groups, &group);
    direct.kind = indirect.kind = suppressed.kind = 15;
    direct.flags = 1; direct.context_index = (short)base_index; direct.next = (CppSymbol *)&indirect;
    indirect.flags = 3; indirect.context_index = (short)via_index; indirect.next = (CppSymbol *)&suppressed;
    suppressed.flags = 0x83; suppressed.context_index = (short)base_index;
    derived.auxiliary.scope_members = (CppSymbol *)&direct;
    if (virtual_base_access_level(&base, &derived) != 2) return 0;
    group.flags = 3;
    if (virtual_base_access_level(&base, &derived) != 3) return 0;
    derived.auxiliary.scope_members = 0;
    return virtual_base_access_level(&base, &derived) == 0;
}
static int check_type_recovery_diagnostics(void)
{
    CppSymbol symbol = {0};
    initialize_preprocessor(); initialize_type_table();
    symbol.name = "distance_object"; symbol.flags = 0x80;
    options.option_W = 0; warning_levels[40] = 14; error_count = 0;
    warn_ignored_distance(&symbol);
    if (error_count) return 0;
    symbol.flags = 0;
    warn_ignored_distance(&symbol);
    if (error_count != 1 || symbol.flags) return 0;
    error_count = 0;
    report_incompatible_type_shapes(0x3d, type_0000, type_0080);
    return error_count == 1 && type_diagnostic_depth == 0;
}
/* Unit-only callbacks inspect child exit bytes; the complete0x8000 literal
   and cdecl callbacks are independently checked in the original whole body. */
static int check_fatal_callbacks(void)
{
    int child, status, mode;
    for (mode = 0; mode < 2; ++mode) {
        child = fork();
        if (child < 0) return 0;
        if (!child) {
            active_input = 0; browser_filename = 0; error_count = 0;
            if (!mode) interrupt_compile(SIGINT);
            else {
                if (_open("missing-unit-pch-file", _O_RDONLY|_O_BINARY) != -1) _exit(98);
                report_error(82, 0, 0, "unit-source", 23);
            }
            _exit(99);
        }
        if (waitpid(child, &status, 0) != child || !WIFEXITED(status) ||
            WEXITSTATUS(status) != (mode ? 1 : 0)) return 0;
    }
    return 1;
}
static int check_asm_recovery_defined_words(void)
{
    ParserValue value;
    int token;
    initialize_preprocessor(); initialize_type_table();
    for (token = 39; token <= 49; ++token) {
        RECON_ASSIGN_RETURN8(value, asm_recovery_value(token));
        if (token == 43) {
            if (value.words.high != (unsigned)type_0000) return 0;
        } else if (value.words.low != (unsigned)(token == 46 ? token : 0)) return 0;
    }
    RECON_ASSIGN_RETURN8(value, asm_recovery_value(18));
    if (value.words.low != 18) return 0;
    error_count = 0;
    report_asm_expected_token(0, 29);
    if (error_count != 1) return 0;
    asm_current_token = 44; current_token_value.name = "bad_operand";
    asm_operand_mode = 0;
    report_asm_expected_token(44, 29);
    return error_count == 2;
}
static void push_unit_token(unsigned char token, ParserValue value)
{
    current_token_code = token; current_token_value = value;
    unget_token(token);
}
static int check_recursive_lookahead_frame(void)
{
    ParserValue empty = {{0}}, *value_top;
    short *state_top;
    int expected, result;
    initialize_preprocessor(); initialize_type_table();
    initialize_lookahead_parser();
    push_unit_token(0, empty);
    expected = lookahead_parse();
    lookahead_token = 133; lookahead_error_count = 7;
    lookahead_recovery_count = 2; lookahead_state = 5;
    state_top = lookahead_state_stack+3; value_top = lookahead_value_stack+3;
    *state_top = 5; value_top->words.low = 123; value_top->words.high = 456;
    lookahead_state_top = state_top; lookahead_value_top = value_top;
    push_unit_token(0, empty);
    result = recursive_lookahead_parse();
    return result == expected && lookahead_token == 133 && lookahead_error_count == 7 &&
        lookahead_recovery_count == 2 && lookahead_state == 5 &&
        lookahead_state_top == state_top && lookahead_value_top == value_top &&
        *state_top == 5 && value_top->words.low == 123 && value_top->words.high == 456;
}
static int check_pch_pragma_name_owner(void)
{
    ParserValue empty = {{0}}, name = {{0}};
    char text[] = "  unit sdk.pch  ", *first;
    initialize_preprocessor(); initialize_type_table();
    name.string.text = text; name.string.length = sizeof(text);
    push_unit_token(0x88, empty);
    push_unit_token(3, name);
    push_unit_token(0x80, empty);
    options.option_Fp = 0;
    synchronize_pch_pragma();
    first = options.option_Fp;
    if (!first || first == text || strcmp(first, "unit sdk.pch") ||
        (token_allocation_flags & 0x4000)) return 0;
    push_unit_token(0x88, empty);
    push_unit_token(3, name);
    push_unit_token(0x80, empty);
    synchronize_pch_pragma();
    if (options.option_Fp != first) return 0;
    push_unit_token(1, empty);
    synchronize_pch_pragma();
    return options.option_Fp == first;
}
/* The same nonvirtual116B type-server object as the genuine C++ methods.
   These C test views expose no application wrapper or extra graph vertex. */
typedef struct TestDatabaseHeader {
    char signature[44];
    unsigned magic, timestamp, age;
    unsigned short first_type, next_type;
    unsigned type_bytes;
} TestDatabaseHeader;
typedef struct TestDebugDatabase {
    TestDatabaseHeader header;
    unsigned char **type_offsets;
    void *type_lookup;
    TestDatabasePool records, lookup_nodes;
    TestDatabaseBlock *scan_block;
    int file, writable, want_lookup, want_offsets;
} TestDebugDatabase;
typedef char TestDebugDatabaseSize[(sizeof(TestDebugDatabase) == 116) ? 1 : -1];
extern int __attribute__((thiscall)) test_database_open(TestDebugDatabase *, char *, char *, unsigned)
    __asm__("_ZN13DebugDatabase4openEPcS0_j");
extern int __attribute__((thiscall)) test_database_read_header(TestDebugDatabase *)
    __asm__("_ZN13DebugDatabase11read_headerEv");
extern unsigned char *__attribute__((thiscall)) test_database_record(TestDebugDatabase *, unsigned short)
    __asm__("_ZN13DebugDatabase6recordEt");
extern unsigned __attribute__((thiscall)) test_database_copy_record(TestDebugDatabase *, unsigned short, unsigned char *, unsigned *)
    __asm__("_ZN13DebugDatabase11copy_recordEtPhPj");
extern int __attribute__((thiscall)) test_database_close_file(TestDebugDatabase *)
    __asm__("_ZN13DebugDatabase10close_fileEv");
extern int __cdecl debug_database_close(DebugDatabase *);
extern unsigned char *__cdecl reserve_database_record(unsigned, DebugDatabase *, unsigned char *);
extern unsigned char *__cdecl debug_database_record(DebugDatabase *, unsigned short);
extern unsigned __cdecl debug_database_copy_record(DebugDatabase *, unsigned short, unsigned char *, unsigned *);
extern unsigned __cdecl debug_database_magic(void);
extern unsigned short __cdecl debug_database_first_type(DebugDatabase *);
extern unsigned short __cdecl debug_database_next_type(DebugDatabase *);
extern unsigned __cdecl debug_database_record_bytes(DebugDatabase *);
static TestDebugDatabase *new_test_database(void)
{
    TestDebugDatabase *database = malloc(sizeof(*database));
    TestDatabaseBlock *block;
    if (!database) return 0;
    memset(database, 0, sizeof(*database));
    block = malloc(sizeof(*block) + 128);
    if (!block) { free(database); return 0; }
    block->remaining = 128; block->cursor = (unsigned char *)(block+1); block->next = 0;
    database->records.first = database->records.current = block;
    block = malloc(sizeof(*block));
    if (!block) { free(database->records.first); free(database); return 0; }
    block->remaining = 0; block->cursor = (unsigned char *)(block+1); block->next = 0;
    database->lookup_nodes.first = database->lookup_nodes.current = block;
    database->file = -1;
    return database;
}
static int check_database_service_owners(void)
{
    TestDatabaseHeader header = {{0}};
    unsigned char leaf[6] = {0x23,1,2,0,0x10,0}, output[8], *stored, *reserved;
    TestDebugDatabase *database;
    unsigned bytes;
    int file;
    memcpy(header.signature, "Microsoft C/C++ program database 1.00\r\n\032JG",
        sizeof("Microsoft C/C++ program database 1.00\r\n\032JG"));
    header.magic = 0xe0d5c; header.timestamp = 0x1234; header.age = 7;
    header.first_type = 0x1000; header.next_type = 0x1001; header.type_bytes = 6;
    file = _open("database-unit.pdb", _O_CREAT|_O_TRUNC|_O_RDWR|_O_BINARY, 0600);
    if (file < 0 || _write(file, &header, sizeof(header)) != sizeof(header) ||
        _write(file, leaf, sizeof(leaf)) != sizeof(leaf) || _close(file)) return 0;
    database = new_test_database();
    if (!database || !test_database_open(database, "database-unit.pdb", "rc", 0) ||
        database->file != -1 || !database->want_offsets || database->want_lookup) return 0;
    if (!debug_database_close((DebugDatabase *)database)) return 0;
    database = new_test_database();
    if (!database) return 0;
    database->want_offsets = 1;
    database->file = _open("database-unit.pdb", _O_RDONLY|_O_BINARY);
    if (database->file < 0 || !test_database_read_header(database)) return 0;
    stored = database->type_offsets[0];
    if (test_database_record(database, 0x1000) != stored ||
        debug_database_record((DebugDatabase *)database, 0x1000) != stored+2 ||
        debug_database_first_type((DebugDatabase *)database) != 0x1000 ||
        debug_database_next_type((DebugDatabase *)database) != 0x1001 ||
        debug_database_magic() != 0xe0d5c ||
        debug_database_record_bytes((DebugDatabase *)database) != 4) return 0;
    memset(output, 0x5a, sizeof(output)); bytes = 3;
    if (test_database_copy_record(database, 0x1000, output, &bytes) != 4 ||
        bytes != 4 || memcmp(output, leaf+2, 3) || output[3] != 0x5a) return 0;
    memset(output, 0x5a, sizeof(output)); bytes = sizeof(output);
    if (debug_database_copy_record((DebugDatabase *)database, 0x1000, output, &bytes) != 4 ||
        bytes != 4 || memcmp(output, leaf+2, 4) || output[4] != 0x5a) return 0;
    reserved = reserve_database_record(2, (DebugDatabase *)database, leaf+2);
    if (!reserved || database->records.used != 12 ||
        database->records.current == database->records.first ||
        database->records.current->remaining != 8192-6 ||
        database->records.current->cursor != reserved+6) return 0;
    if (reserve_database_record(0, (DebugDatabase *)database, leaf+2) != reserved+6 ||
        database->records.used != 16 ||
        debug_database_record_bytes((DebugDatabase *)database) != 14) return 0;
    if (!test_database_close_file(database) || database->file != -1 ||
        !test_database_close_file(database)) return 0;
    if (!debug_database_close((DebugDatabase *)database)) return 0;
    return _unlink("database-unit.pdb") == 0;
}
/* Whole-body/static qualification only: no original CLI encounter is claimed. */
static int check_container_owners(void)
{
    static unsigned char owner[12288];
    struct { short count, capacity; void *items[4]; } vector;
    InlineVector *set = (InlineVector *)&vector;
    TypeSetIterator iterator;
    CType replacement;
    unsigned char *base = (unsigned char *)(((unsigned)owner + 4095) & ~4095U);
    vector.count = 3; vector.capacity = 4;
    vector.items[0] = base+8; vector.items[1] = base+4096+8;
    vector.items[2] = base+16;
    if (!valid_inline_vector_header(set) || !validate_inline_vector(set)) return 0;
    vector.items[1] = vector.items[0];
    if (validate_inline_vector(set)) return 0;
    vector.items[1] = base+4;
    if (validate_inline_vector(set)) return 0;
    vector.count = -1;
    if (valid_inline_vector_header(set) || validate_inline_vector(set)) return 0;
    vector.count = 3; vector.capacity = 2;
    if (valid_inline_vector_header(set) || validate_inline_vector(set)) return 0;
    vector.capacity = 4; vector.items[1] = base+12;
    begin_type_set(&iterator, set); iterator.type = type_0000;
    iterator.reserved = 0x4321;
    if (!type_set_cursor_in_range(&iterator) || !valid_type_set_iterator(&iterator)) return 0;
    iterator.index = 1;
    replace_type_set_item(&iterator, &replacement);
    if (vector.items[1] != &replacement || iterator.index != 1 ||
        iterator.type != type_0000 || iterator.reserved != 0x4321) return 0;
    iterator.index = 3;
    if (!type_set_cursor_in_range(&iterator) || !valid_type_set_iterator(&iterator)) return 0;
    iterator.index = 4;
    if (type_set_cursor_in_range(&iterator) || valid_type_set_iterator(&iterator)) return 0;
    iterator.index = -1;
    if (!type_set_cursor_in_range(&iterator) || !valid_type_set_iterator(&iterator)) return 0;
    iterator.set = 0;
    return !valid_type_set_iterator(&iterator) && !validate_inline_vector(0);
}
static int check_bitset_owners(void)
{
    BitSet *set = new_bitset(17), *other = new_bitset(17), *copy;
    if (!set || !other) return 0;
    set->bits[0] = 0xf3; set->bits[1] = 0xaa; set->bits[2] = 0xf0;
    other->bits[0] = 0x30; other->bits[1] = 0x0f; other->bits[2] = 0xc0;
    copy = copy_bitset(set);
    if (!copy || copy == set || copy->bit_count != 17 ||
        memcmp(copy->bits, set->bits, 4)) return 0;
    subtract_bitset(copy, other);
    if (copy->bits[0] != 0xc3 || copy->bits[1] != 0xa0 ||
        copy->bits[2] != 0x30 || copy->bits[3] != 0xff ||
        set->bits[0] != 0xf3) return 0;
    release_bitset(copy); release_bitset(other); release_bitset(set);
    release_bitset(0);
    return 1;
}
static int check_dos_calendar(void)
{
    /* Sunday=0. The2100 result follows the original four-year cycle,
       rather than the Gregorian century correction. */
    return dos_date_weekday((1U<<21)|(1U<<16)) == 2 &&
        dos_date_weekday((20U<<25)|(2U<<21)|(29U<<16)) == 2 &&
        dos_date_weekday((44U<<25)|(2U<<21)|(29U<<16)) == 4 &&
        dos_date_weekday((43U<<25)|(3U<<21)|(1U<<16)) == 3 &&
        dos_date_weekday((120U<<25)|(3U<<21)|(1U<<16)) == 2 &&
        dos_date_weekday((127U<<25)|(12U<<21)|(31U<<16)) == 0;
}
static int check_declaration_support_owners(void)
{
    CppSymbol tag = {0}, member = {0}, pointer = {0}, same_name = {0};
    ClassInfo info = {0};
    VirtualTable table = {0};
    Vector tables;
    void *table_items[1];
    struct { short count, capacity; void *items[1]; } contexts;
    TypeShape pointer_shape = {0}, function_shape = {0}, leading_shape = {0};
    TypeShape this_shape = {0}, member_shape = {0};
    CType pointer_type = {0}, member_type = {0}, this_type = {0}, class_type = {0};
    FunctionParameters parameters = {0};
    BaseClassLink first = {0}, second = {0}, terminator = {0};
    QualifierBinding path[3];
    char name[] = "Owner", other_name[] = "Owner";
    tag.name = name; same_name.name = name; tag.detail.class_info = &info;
    table_items[0] = &table; tables.count = tables.capacity = 1;
    tables.items = table_items; tables.arena = 0; info.virtual_tables = &tables;
    table.pointer = &pointer; pointer.type = &pointer_type;
    pointer_type.related = &pointer_shape; pointer_shape.attributes = 2;
    contexts.count = contexts.capacity = 1; contexts.items[0] = &tag;
    cpp_context_stack = (InlineVector *)&contexts;
    member.auxiliary.virtual_function.table_class = 0;
    if (primary_vtable_pointer_attributes(&tag) != 2 ||
        !virtual_member_uses_far_table(&member)) return 0;
    pointer_shape.attributes = 1;
    if (virtual_member_uses_far_table(&member)) return 0;
    member.type = &member_type; member_type.related = &function_shape;
    function_shape.flags = 4; function_shape.attributes = 2;
    function_shape.data.parameters = &parameters;
    parameters.associated_type = &this_type; this_type.related = &this_shape;
    this_shape.attributes = 2;
    if (!member_uses_far_code(&member) || !member_uses_far_this(&member)) return 0;
    leading_shape.related = &function_shape; member_type.related = &leading_shape;
    function_shape.attributes = 1; this_shape.attributes = 1;
    if (member_uses_far_code(&member) || member_uses_far_this(&member)) return 0;
    current_class_context = -1;
    if (is_current_class_tag(&same_name)) return 0;
    current_class_context = 0; class_contexts[0].symbol = &tag;
    if (!is_current_class_tag(&same_name)) return 0;
    same_name.name = other_name;
    if (is_current_class_tag(&same_name)) return 0;
    first.kind = second.kind = 15; terminator.kind = 4;
    first.displacement = 2; second.displacement = 6; terminator.displacement = 100;
    path[0].target = (ClassReference *)&first; path[0].next = &path[1];
    path[1].target = (ClassReference *)&second; path[1].next = &path[2];
    path[2].target = (ClassReference *)&terminator; path[2].next = 0;
    if (base_qualifier_displacement(path) != 8 || base_qualifier_displacement(0)) return 0;
    class_type.related = &member_shape; member_shape.data.tag_symbol = &tag;
    if (member_pointer_has_zero_base_offsets(&class_type)) return 0;
    member_shape.flags = 0x20;
    if (!member_pointer_has_zero_base_offsets(&class_type)) return 0;
    info.flags = 0x10;
    return !member_pointer_has_zero_base_offsets(&class_type);
}
static int check_text_support_owners(void)
{
    unsigned char buffer[32];
    char prefix[261], readback[32], *name;
    OverloadSet *set;
    FILE *stream;
    unsigned i;
    memset(buffer, 0x5a, sizeof(buffer));
    if (copy_lookahead_spelling(buffer, "alpha") != buffer+5 ||
        memcmp(buffer,"alpha",6) || buffer[6] != 0x5a ||
        copy_parser_spelling(buffer+5, " beta") != buffer+10 ||
        memcmp(buffer,"alpha beta",11) || buffer[11] != 0x5a) return 0;
    name = append_symbol_suffix("Owner", "_member");
    if (strcmp(name,"Owner_member")) return 0;
    for (i = 0; i < 260; ++i) prefix[i] = 'p';
    prefix[260] = 0; name = append_symbol_suffix(prefix,".tail");
    if (strlen(name) != 246 || memcmp(name,prefix,241) || strcmp(name+241,".tail")) return 0;
    set = new_named_overload_set(name);
    if (set->kind != 17 || set->name != name || set->functions ||
        (set->declaration_flags & 0x300)) return 0;
    stream = fopen("formatter-unit.txt", "w+b");
    if (!stream || write_formatted_text(stream,"%s:%d:%x:%c", "node",7,0x2a,'!') != 1 ||
        fflush(stream) || fseek(stream,0,SEEK_SET)) return 0;
    memset(readback,0x5a,sizeof(readback));
    if (fread(readback,1,11,stream) != 11 || memcmp(readback,"node:7:2a:!",11) ||
        readback[11] != 0x5a || fclose(stream)) return 0;
    return _unlink("formatter-unit.txt") == 0;
}
static int check_extended_support_owners(void)
{
    Extended value, copy;
    ExtendedLimit raw;
    double real = 1.5;
    unsigned long maximum = 0xffffffffUL, converted = 0;
    long negative = -1, rounded = 0;
    unsigned control = _control87(0,0);
    unsigned short before, after;
    __asm { fstsw before }
    if (double_to_extended(&real,&value) || extended_format(&value) != 8 ||
        copy_extended(&value,&copy) || memcmp(&value,&copy,sizeof(value))) return 0;
    copy.format = 0xffff;
    if (extended_format(&copy) != -1 || extended_to_long_double(&value,&raw) ||
        memcmp(&raw,&value,sizeof(raw)) || long_double_to_extended(&raw,&copy) ||
        copy.format != 10 || memcmp(&copy,&value,sizeof(raw))) return 0;
    if (round_extended_to_integer(&value,&rounded) || rounded != 2) return 0;
    real = -1.5;
    if (double_to_extended(&real,&value) ||
        round_extended_to_integer(&value,&rounded) || rounded != -2) return 0;
    if (unsigned_to_extended(&maximum,&value) || extended_to_unsigned(&value,&converted) ||
        converted != maximum) return 0;
    if (integer_to_extended(&negative,&value) ||
        extended_to_unsigned(&value,&converted) != 1 || converted != 0xffffffffUL) return 0;
    __asm { fstsw after }
    return _control87(0,0) == control && !((before ^ after) & 0x3800);
}
void __stdcall encode_pch_pointer(void *pointer);
static int check_pch_value_vector_owners(void)
{
    void *items[2], *first = allocate_permanent(4), *second = allocate_permanent(4);
    Vector vector;
    unsigned references[2], final;
    vector.count = vector.capacity = 2; vector.arena = 0; vector.items = items;
    items[0] = first; items[1] = second;
    encode_pch_pointer(first); references[0] = pch_encoded_pointer;
    encode_pch_pointer(second); references[1] = pch_encoded_pointer; final = pch_encoded_pointer;
    if (references[0] == 0xffffffff || final == 0xffffffff) return 0;
    if (encode_pch_vector_values(&vector) != &vector) return 0;
    if (pch_encoded_pointer != final || items[0] != first || items[1] != second) return 0;
    items[0] = (void *)references[0]; items[1] = (void *)references[1];
    if (decode_pch_vector_values(&vector) != &vector) return 0;
    return (unsigned)items[0] == references[0] && (unsigned)items[1] == references[1] &&
        pch_encoded_pointer == final && vector.count == 2;
}
static int check_outer_pointer_type_owner(void)
{
    CType type = {0}, *result;
    TypeShape shape = {0};
    initialize_type_table();
    type.flags = 0x64; type.related = &shape;
    shape.flags = 2; shape.attributes = 2; shape.data.array_bound = 8;
    result = normalize_outer_pointer_type(&type);
    return result && result->flags == 0x64 && result->related &&
        result->related->flags == 1 && (result->related->attributes & 15) == 2 &&
        type.related == &shape && shape.flags == 2 && shape.data.array_bound == 8;
}
unsigned __cdecl codeview_leaf_offset(unsigned short *leaf);
static int check_remaining_frontend_owners(void)
{
    unsigned short leaf;
    IrToken outer = {0}, second = {0}, third = {0}, base = {0};
    if (take_unnamed_entity_index() != 65535 || take_unnamed_entity_index() != 65534) return 0;
    leaf = 1; if (codeview_leaf_offset(&leaf) != 0xffffffffU) return 0;
    leaf = 2; if (codeview_leaf_offset(&leaf)) return 0;
    leaf = 0x800b; if (codeview_leaf_offset(&leaf) != 0x8009) return 0;
    leaf = 0; if (codeview_leaf_offset(&leaf) != 0xfffffffeU) return 0;
    leaf = 0xffff; if (codeview_leaf_offset(&leaf) != 0xfffd) return 0;
    outer.kind = OP_ADD; outer.flags = 0x71; outer.field02 = 0x1234;
    outer.value.pointer = &second; second.value.pointer = &third; third.value.pointer = &base;
    return subtraction_expression_base(&outer) == &base && outer.kind == OP_SUBTRACT &&
        outer.flags == 0x71 && outer.field02 == 0x1234 &&
        outer.value.pointer == &second && !subtraction_expression_base(0);
}
static int check_asm_segment_prefix_owner(void)
{
    static const unsigned char expected[] = {
        0x4f,22,5,5,0xaf,0,1,12, 0x4f,22,5,5,0xaf,0,1,13,
        0x4f,22,5,5,0xaf,0,1,12
    };
    unsigned char actual[sizeof(expected)+1];
    FILE *stream = fopen("asm-prefix-unit.bin","w+b");
    if (!stream) return 0;
    expression_file = stream; options.prototypes_only = 0;
    reset_asm_operand(0); reset_asm_operand(1); asm_operand_count = 1;
    write_asm_segment_prefix();
    if (ftell(stream)) return 0;
    asm_addresses[0].flags = 29;
    write_asm_segment_prefix();
    if (control_counted_data[4] != 12 || control_counted_data[5]) return 0;
    asm_addresses[0].flags = 0; asm_addresses[1].flags = 30; asm_operand_count = 2;
    write_asm_segment_prefix();
    asm_addresses[0].flags = 29;
    write_asm_segment_prefix();
    if (fflush(stream) || fseek(stream,0,SEEK_SET)) return 0;
    memset(actual,0x5a,sizeof(actual));
    if (fread(actual,1,sizeof(actual),stream) != sizeof(expected) ||
        memcmp(actual,expected,sizeof(expected)) || actual[sizeof(expected)] != 0x5a ||
        fclose(stream)) return 0;
    return _unlink("asm-prefix-unit.bin") == 0;
}
const ReconTest c1xx_tests[] = {
    {"NULL single-function guard",1,check_single_null},
    {"NULL overload-count guard",1,check_overload_null},
    {"NULL debug-method-count guard",1,check_methods_null},
    {"NULL debug-method-list guard",1,check_method_list_null},
    {"unknown real width guard",1,check_real_width},
    {"unknown integer width guard",1,check_integer_width},
    {"cached parser lookahead guard",1,check_cached_lookahead},
    {"unsupported signed multiply width guard",1,check_multiply_width},
    {"empty browser filename guard",1,check_empty_browser_name},
    {"unmodeled browser category guard",1,check_unknown_browser_category},
    {"ten original-backed multiply returns",1,check_multiply_returns},
    {"primitive CodeView types",1,check_primitive_types},
    {"browser filename suffixes",1,check_browser_filename},
    {"single-function list controls",1,check_single_function},
    {"dormant memory-intrinsic lookup (unit-only)",1,check_memory_intrinsic_lookup},
    {"auxiliary real encodings (unit-only)",1,check_auxiliary_real_encodings},
    {"legacy based-self cast (unit-only)",1,check_based_self_cast},
    {"legacy based-self pointer source (unit-only)",1,check_based_self_pointer_source},
    {"based-expression identity (unit-only)",1,check_based_expression_identity},
    {"legacy declarator prefix owners (unit-only)",1,check_legacy_prefix_owners},
    {"frontend owner helpers (unit-only)",1,check_frontend_owner_helpers},
    {"thunk-key relocation slot (unit-only)",1,check_thunk_key_relocation},
    {"database pool release (unit-only)",1,check_database_pool_release},
    {"segmented descriptor expression (unit-only)",1,check_segment_descriptor_expression},
    {"initializer address widths (unit-only)",1,check_initializer_address_widths},
    {"segment debug owners (unit-only)",1,check_segment_debug_owners},
    {"input handle retirement (unit-only)",1,check_input_handle_retirement},
    {"browser fallback state (unit-only)",1,check_browser_fallback_state},
    {"untyped operand owner (unit-only)",1,check_untyped_operand_owner},
    {"materialized address owner (unit-only)",1,check_materialized_address_owner},
    {"virtual access owners (unit-only)",1,check_virtual_access_owners},
    {"type recovery diagnostics (unit-only)",1,check_type_recovery_diagnostics},
    {"fatal callbacks (unit-only)",1,check_fatal_callbacks},
    {"ASM recovery defined words (unit-only)",1,check_asm_recovery_defined_words},
    {"recursive lookahead frame (unit-only)",1,check_recursive_lookahead_frame},
    {"PCH pragma name owner (unit-only)",1,check_pch_pragma_name_owner},
    {"database service owners (unit-only)",1,check_database_service_owners},
    {"container owners (unit-only)",1,check_container_owners},
    {"bitset ownership (unit-only)",1,check_bitset_owners},
    {"DOS calendar (unit-only)",1,check_dos_calendar},
    {"declaration support owners (unit-only)",1,check_declaration_support_owners},
    {"text support owners (unit-only)",1,check_text_support_owners},
    {"extended support owners (unit-only)",1,check_extended_support_owners},
    {"PCH value vector owners (unit-only)",1,check_pch_value_vector_owners},
    {"outer pointer type owner (unit-only)",1,check_outer_pointer_type_owner},
    {"remaining frontend owners (unit-only)",1,check_remaining_frontend_owners},
    {"ASM segment prefix owner (unit-only)",1,check_asm_segment_prefix_owner}
};
