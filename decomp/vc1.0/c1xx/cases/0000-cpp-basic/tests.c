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
    {"type recovery diagnostics (unit-only)",1,check_type_recovery_diagnostics}
};
